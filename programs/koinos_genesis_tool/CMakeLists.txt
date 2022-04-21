add_executable(koinos_genesis_tool main.cpp)
target_link_libraries(koinos_genesis_tool Koinos::exception Koinos::mq Koinos::crypto Koinos::proto Koinos::util Boost::program_options)
install(TARGETS
   koinos_genesis_tool
   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
