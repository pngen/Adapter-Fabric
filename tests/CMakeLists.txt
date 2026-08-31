# Adapter Fabric — test suite.
set(AF_TESTS
  test_identity
  test_generation
  test_lifecycle
  test_compatibility
  test_composition
  test_authority
  test_migration
  test_invalidation
  test_capacity
  test_property
  test_persistence
  test_adversarial
  test_concurrency
  test_cuda
)
foreach(t ${AF_TESTS})
  add_executable(${t} ${t}.cpp)
  target_link_libraries(${t} PRIVATE adapter_fabric)
  af_enable_strict_warnings(${t})
  add_test(NAME ${t} COMMAND ${t})
endforeach()

# Real OS-process multiprocess proof. Requires the coordinator/worker executables.
if(TARGET af_coordinator AND TARGET af_worker)
  add_executable(test_multiprocess test_multiprocess.cpp)
  target_link_libraries(test_multiprocess PRIVATE adapter_fabric)
  af_enable_strict_warnings(test_multiprocess)
  add_test(NAME test_multiprocess COMMAND test_multiprocess)
  # Locate the built coordinator/worker executables at build time.
  target_compile_definitions(test_multiprocess PRIVATE
    AF_COORDINATOR_EXE="$<TARGET_FILE:af_coordinator>"
    AF_WORKER_EXE="$<TARGET_FILE:af_worker>")
endif()
