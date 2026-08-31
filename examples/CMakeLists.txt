set(AF_EXAMPLES
  ex_register
  ex_load_activate
  ex_compose
  ex_migrate
  ex_invalidate
  ex_persist
  ex_restart
  ex_cuda
  ex_compat
)
foreach(e ${AF_EXAMPLES})
  add_executable(${e} ${e}.cpp)
  target_link_libraries(${e} PRIVATE adapter_fabric)
  af_enable_strict_warnings(${e})
endforeach()
