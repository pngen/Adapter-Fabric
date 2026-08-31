// Adapter Fabric — migration FSM tests.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#include "test.hpp"
#include "adapter_fabric/migration.hpp"

using namespace adapter_fabric;

AF_TEST_CASE(migration_valid_sequence) {
  MigrationStateMachine m;
  AF_CHECK(m.phase() == MigrationPhase::not_started);
  AF_CHECK(m.advance(MigrationPhase::source_validated));
  AF_CHECK(m.advance(MigrationPhase::destination_reserved));
  AF_CHECK(m.advance(MigrationPhase::transferring));
  AF_CHECK(m.advance(MigrationPhase::verifying));
  AF_CHECK(m.advance(MigrationPhase::destination_ready));
  AF_CHECK(m.advance(MigrationPhase::authority_promoted));
  AF_CHECK(m.advance(MigrationPhase::source_retired));
  AF_CHECK(m.advance(MigrationPhase::source_released));
  // terminal
  AF_CHECK(!m.advance(MigrationPhase::not_started));
}

AF_TEST_CASE(migration_skipping_dest_ready_blocked) {
  MigrationStateMachine m;  // non-destructive
  m.advance(MigrationPhase::source_validated);
  m.advance(MigrationPhase::destination_reserved);
  m.advance(MigrationPhase::transferring);
  m.advance(MigrationPhase::verifying);
  AF_CHECK(!m.advance(MigrationPhase::authority_promoted));  // must pass destination_ready
  AF_CHECK(m.advance(MigrationPhase::destination_ready));
  AF_CHECK(m.advance(MigrationPhase::authority_promoted));
}

AF_TEST_CASE(migration_destructive_skips_dest_ready) {
  MigrationStateMachine m(true);  // destructive
  m.advance(MigrationPhase::source_validated);
  m.advance(MigrationPhase::destination_reserved);
  m.advance(MigrationPhase::transferring);
  m.advance(MigrationPhase::verifying);
  AF_CHECK(m.advance(MigrationPhase::authority_promoted));  // allowed
}

AF_TEST_CASE(migration_abort_allowed_early) {
  MigrationStateMachine m;
  AF_CHECK(m.advance(MigrationPhase::source_validated));
  AF_CHECK(m.advance(MigrationPhase::aborted));
  AF_CHECK(!m.advance(MigrationPhase::destination_reserved));
}

AF_TEST_MAIN
