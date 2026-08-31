// Adapter Fabric — coordinator implementation.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "adapter_fabric/coordinator.hpp"
#include "adapter_fabric/checksum.hpp"
#include "adapter_fabric/adapter.hpp"
#include "adapter_fabric/invalidation.hpp"
#include "adapter_fabric/persistence.hpp"
#include "adapter_fabric/explain.hpp"
#include <sstream>
#include <variant>

namespace adapter_fabric {

namespace {
template <typename IdT>
IdT must_parse(std::string_view s) {
  auto v = IdT::parse(s);
  if (auto* id = std::get_if<IdT>(&v)) return *id;
  throw Error(ErrorCode::invalid_identity, std::get<std::string>(v));
}
}  // namespace

CoordinatorServer::CoordinatorServer(std::uint16_t port, std::map<ResidencyLocation, std::uint64_t> limits)
    : port_(port), fabric_(std::move(limits)) {}

CoordinatorServer::~CoordinatorServer() { stop(); }

bool CoordinatorServer::start() {
  tcp_global_init();
  if (!accept_listener_.bind_listen(port_)) return false;
  running_ = true;
  accept_thread_ = std::thread(&CoordinatorServer::accept_loop, this);
  return true;
}

void CoordinatorServer::stop() {
  shutdown_ = true;
  running_ = false;
  accept_listener_.close();
  if (accept_thread_.joinable()) { try { accept_thread_.join(); } catch (...) {} }
  for (auto& th : conn_threads_) if (th.joinable()) { try { th.join(); } catch (...) {} }
  conn_threads_.clear();
}

void CoordinatorServer::accept_loop() {
  while (!shutdown_) {
    TcpSocket conn = accept_listener_.accept();
    if (!conn.valid()) break;
    auto sp = std::make_shared<TcpSocket>(std::move(conn));
    conn_threads_.emplace_back(&CoordinatorServer::handle_connection, this, sp);
  }
}

void CoordinatorServer::handle_connection(std::shared_ptr<TcpSocket> conn) {
  std::vector<std::uint8_t> buf;
  bool is_worker = false;
  std::shared_ptr<WorkerSlot> slot;
  while (!shutdown_ && conn->recv_frame(buf)) {
    Message msg;
    try { msg = decode_message(buf); } catch (...) { return; }
    if (msg.type == MsgType::wk_register) {
      slot = std::make_shared<WorkerSlot>();
      slot->sock = conn;
      slot->id = must_parse<WorkerId>(msg.text);
      slot->boot = must_parse<WorkerBootId>(msg.text2);
      slot->address = msg.text3;
      slot->connected = true;
      { std::lock_guard<std::mutex> lk(workers_mc_); workers_[msg.text] = slot; }
      { std::lock_guard<std::mutex> lk(state_mc_);
        WorkerRecord rec; rec.id = slot->id; rec.boot = slot->boot; rec.address = slot->address;
        rec.connected = true; rec.epoch = fabric_.epoch(); if (!msg.text4.empty()) rec.node = must_parse<NodeId>(msg.text4);
        fabric_.register_worker(rec);
        fabric_.on_worker_restart(slot->id, slot->boot);
      }
      Message ack; ack.type = MsgType::wk_register_ack; ack.ok = true; ack.a = fabric_.epoch().value();
      ack.text = std::to_string(fabric_.epoch().value());
      conn->send_frame(encode_message(ack));
      is_worker = true;
      continue;
    }
    if (is_worker) {
      if (msg.req_id != 0) {
        std::shared_ptr<Pending> pend;
        { std::lock_guard<std::mutex> lk(pending_mc_); auto it = pending_.find(msg.req_id); if (it != pending_.end()) pend = it->second; }
        if (pend) { std::lock_guard<std::mutex> lk(pend->m); pend->result = msg; pend->done = true; pend->cv.notify_one(); }
      }
      continue;
    }
    Message out = process_control(msg);
    if (!conn->send_frame(encode_message(out))) return;
  }
  if (slot) {
    { std::lock_guard<std::mutex> lk(workers_mc_); slot->connected = false; }
    abort_pending_for_worker(slot->id.str());
    { std::lock_guard<std::mutex> lk(state_mc_); fabric_.on_worker_down(slot->id); }
  }
}

void CoordinatorServer::abort_pending_for_worker(const std::string& worker_id) {
  (void)worker_id;
  std::vector<std::shared_ptr<Pending>> failed;
  { std::lock_guard<std::mutex> lk(pending_mc_);
    for (auto& kv : pending_) { auto& pd = kv.second; std::lock_guard<std::mutex> pk(pd->m); if (!pd->done) { pd->done = true; pd->result.type = MsgType::wk_error; pd->result.ok = false; pd->result.text = "worker disconnected"; failed.push_back(pd); } }
  }
  for (auto& pd : failed) pd->cv.notify_all();
}

Message CoordinatorServer::forward_to_worker(std::uint64_t req_id, const std::string& worker_id, const Message& fwd) {
  std::shared_ptr<WorkerSlot> slot;
  { std::lock_guard<std::mutex> lk(workers_mc_); auto it = workers_.find(worker_id); if (it != workers_.end()) slot = it->second; }
  Message fail; fail.type = MsgType::wk_error; fail.ok = false; fail.text = "unknown or disconnected worker";
  if (!slot || !slot->connected || !slot->sock) return fail;
  auto pend = std::make_shared<Pending>();
  { std::lock_guard<std::mutex> lk(pending_mc_); pending_[req_id] = pend; }
  Message f = fwd; f.req_id = req_id;
  if (!slot->sock->send_frame(encode_message(f))) {
    std::lock_guard<std::mutex> lk(pending_mc_); pending_.erase(req_id);
    fail.text = "send to worker failed"; return fail;
  }
  std::unique_lock<std::mutex> lk(pend->m);
  pend->cv.wait(lk, [&]{ return pend->done; });
  Message result = std::move(pend->result);
  lk.unlock();
  { std::lock_guard<std::mutex> lk2(pending_mc_); pending_.erase(req_id); }
  return result;
}

Message CoordinatorServer::process_control(const Message& req) {
  Message out;
  out.type = MsgType::ctl_result;
  auto err = [&](const std::string& m) { out.ok = false; out.text = m; return out; };
  try {
    switch (req.type) {
      case MsgType::ctl_register_adapter: {
        AdapterDescriptor d = deserialize_adapter(req.payload);
        AdapterDescriptor reg;
        { std::lock_guard<std::mutex> lk(state_mc_); reg = fabric_.register_adapter(std::move(d)); }
        out.ok = true; out.text = reg.id.str(); out.a = reg.generation.value();
        out.payload = serialize_adapter(reg);
        return out;
      }
      case MsgType::ctl_validate: {
        CompatibilityTarget t;
        t.base_model = must_parse<BaseModelId>(req.text2);
        t.base_model_revision = must_parse<ModelRevisionId>(req.text3);
        t.policy_generation = PolicyGeneration{1};
        t.runtime_capabilities["fp16"] = "supported";
        t.runtime_capabilities["sm_120"] = "supported";
        CompatibilityReport rep;
        { std::lock_guard<std::mutex> lk(state_mc_); rep = fabric_.validate(must_parse<AdapterId>(req.text), t); }
        out.ok = rep.compatible; out.text = rep.summary;
        const std::string js = to_json(rep); out.payload.assign(js.begin(), js.end());
        return out;
      }
      case MsgType::ctl_load: {
        AuthorityFence fence = decode_fence(req.payload);
        { std::lock_guard<std::mutex> lk(state_mc_); auto pre = fabric_.check_fence(fence); if (!pre.authorized()) { out.ok = false; out.text = pre.reason; return out; } }
        std::string worker_id = req.text2;
        std::uint64_t bytes = 0;
        AdapterInstanceId inst;
        { std::lock_guard<std::mutex> lk(state_mc_); const AdapterDescriptor* ad = fabric_.adapter(fence.adapter); if (ad) bytes = ad->memory_bytes; inst = fabric_.create_instance(fence.adapter, fence.worker, fence.boot, fence.device, bytes); }
        Message fwd; fwd.type = MsgType::wk_load; fwd.text = fence.adapter.str(); fwd.text2 = inst.str(); fwd.payload = encode_fence(fence);
        Message ack = forward_to_worker(next_req_++, worker_id, fwd);
        if (ack.ok) {
          { std::lock_guard<std::mutex> lk(state_mc_); auto* rec = fabric_.residency(inst); if (rec) { rec->ready = true; rec->bytes = ack.a; } try { fabric_.capacity().reserve(ResidencyLocation::device_memory, ack.a, inst.str()); } catch (...) {} }
        }
        out.ok = ack.ok; out.text = ack.ok ? "loaded" : ack.text; out.text2 = inst.str();
        if (out.ok) { std::lock_guard<std::mutex> lk(state_mc_); auto* ar = const_cast<AdapterAuthorityRecord*>(fabric_.authority_record(fence.adapter)); if (ar) ar->lifecycle.set_state(LifecycleState::ready); }
        return out;
      }
      case MsgType::ctl_activate: {
        AuthorityFence fence = decode_fence(req.payload);
        { std::lock_guard<std::mutex> lk(state_mc_); auto pre = fabric_.check_fence(fence); if (!pre.authorized()) { out.ok = false; out.text = pre.reason; return out; } }
        std::string worker_id = req.text2;
        Message fwd; fwd.type = MsgType::wk_activate; fwd.text = fence.adapter.str(); fwd.payload = encode_fence(fence);
        Message ack = forward_to_worker(next_req_++, worker_id, fwd);
        AuthorityCheck chk;
        if (ack.ok) { std::lock_guard<std::mutex> lk(state_mc_); chk = fabric_.bind_authority(fence.adapter, fence); }
        out.ok = ack.ok && chk.authorized();
        out.text = ack.ok ? chk.reason : ack.text;
        return out;
      }
      case MsgType::ctl_deactivate: {
        AdapterId id = must_parse<AdapterId>(req.text);
        Message fwd; fwd.type = MsgType::wk_deactivate; fwd.text = id.str();
        Message ack = forward_to_worker(next_req_++, req.text2, fwd);
        { std::lock_guard<std::mutex> lk(state_mc_); auto* ar = const_cast<AdapterAuthorityRecord*>(fabric_.authority_record(id)); if (ar) ar->lifecycle.set_state(LifecycleState::ready); }
        out.ok = ack.ok; out.text = ack.ok ? "deactivated" : ack.text;
        return out;
      }
      case MsgType::ctl_migrate: {
        AuthorityFence fence = decode_fence(req.payload);
        Message fwd; fwd.type = MsgType::wk_migrate; fwd.text = fence.adapter.str(); fwd.text2 = req.text2; fwd.payload = encode_fence(fence);
        Message ack = forward_to_worker(next_req_++, req.text2, fwd);
        out.ok = ack.ok; out.text = ack.ok ? "migrated" : ack.text;
        return out;
      }
      case MsgType::ctl_invalidate: {
        AdapterId id = must_parse<AdapterId>(req.text);
        InvalidationRecord rec;
        { std::lock_guard<std::mutex> lk(state_mc_); rec = fabric_.invalidate(id, InvalidationTrigger::manual, req.text4.empty() ? "manual invalidation" : req.text4); }
        Message fwd; fwd.type = MsgType::wk_invalidate; fwd.text = id.str();
        forward_to_worker(next_req_++, req.text2, fwd);
        out.ok = true; out.text = "invalidated: " + rec.reason;
        return out;
      }
      case MsgType::ctl_evict: {
        AdapterInstanceId inst = must_parse<AdapterInstanceId>(req.text);
        std::string reason; bool ok; std::uint64_t bytes = 0;
        { std::lock_guard<std::mutex> lk(state_mc_); if (auto* rec = fabric_.residency(inst)) bytes = rec->bytes; ok = fabric_.evict(inst, reason); if (ok) { try { fabric_.capacity().release(ResidencyLocation::device_memory, bytes, inst.str()); } catch (...) {} } }
        Message fwd; fwd.type = MsgType::wk_deactivate; fwd.text = inst.str();
        forward_to_worker(next_req_++, req.text2, fwd);
        out.ok = ok; out.text = reason;
        return out;
      }
      case MsgType::ctl_pin: { std::lock_guard<std::mutex> lk(state_mc_); fabric_.pin(must_parse<AdapterInstanceId>(req.text)); out.ok = true; out.text = "pinned"; return out; }
      case MsgType::ctl_unpin: { std::lock_guard<std::mutex> lk(state_mc_); fabric_.unpin(must_parse<AdapterInstanceId>(req.text)); out.ok = true; out.text = "unpinned"; return out; }
      case MsgType::ctl_snapshot: { std::vector<std::uint8_t> snap; { std::lock_guard<std::mutex> lk(state_mc_); snap = serialize_snapshot(fabric_.snapshot()); } out.payload = snap; out.ok = true; out.text = "snapshot"; return out; }
      case MsgType::ctl_list: { std::ostringstream os; { std::lock_guard<std::mutex> lk(state_mc_); auto ads = fabric_.adapters(); for (auto& a : ads) os << a.id.str() << " gen=" << a.generation.value() << " " << a.name << "\n"; } out.ok = true; out.text = os.str(); return out; }
      case MsgType::ctl_compose: {
        std::vector<AdapterId> ids; const std::string payload_str(req.payload.begin(), req.payload.end());
        std::istringstream is(payload_str); std::string s; while (std::getline(is, s, '\n')) if (!s.empty()) ids.push_back(must_parse<AdapterId>(s));
        Composition c; { std::lock_guard<std::mutex> lk(state_mc_); c = fabric_.publish_composition(ids, req.text4.empty() ? "default" : req.text4); }
        out.ok = c.is_valid; out.text = c.digest; out.text2 = c.id.str(); out.a = c.generation.value();
        return out;
      }
      case MsgType::ctl_epoch: { std::uint64_t e; { std::lock_guard<std::mutex> lk(state_mc_); e = fabric_.advance_epoch().value(); } out.ok = true; out.a = e; out.text = std::to_string(e); return out; }
      case MsgType::ctl_capacity: { std::ostringstream os; { std::lock_guard<std::mutex> lk(state_mc_); auto usage = fabric_.capacity().usage(); os << "device_memory=" << usage[ResidencyLocation::device_memory] << " pinned_host=" << usage[ResidencyLocation::pinned_host] << " pageable_host=" << usage[ResidencyLocation::pageable_host] << " process_local=" << usage[ResidencyLocation::process_local] << " node_local=" << usage[ResidencyLocation::node_local] << " persistent_storage=" << usage[ResidencyLocation::persistent_storage]; } out.ok = true; out.text = os.str(); return out; }
      case MsgType::ctl_shutdown: { out.ok = true; out.text = "shutdown"; shutdown_ = true; running_ = false; return out; }
      case MsgType::ping: { out.ok = true; out.type = MsgType::pong; return out; }
      default: return err("unhandled control message");
    }
  } catch (const Error& e) {
    out.ok = false; out.text = std::string(to_string(e.code())) + ": " + e.what();
    return out;
  } catch (const std::exception& e) {
    out.ok = false; out.text = std::string("exception: ") + e.what();
    return out;
  }
}

}  // namespace adapter_fabric
