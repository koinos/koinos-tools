add_executable(koinos_random_proof_generator main.cpp)
target_link_libraries(koinos_random_proof_generator Koinos::exception Koinos::crypto Koinos::proto Koinos::log Boost::program_options)
install(TARGETS
   koinos_random_proof_generator
   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
