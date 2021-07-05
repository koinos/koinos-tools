add_executable(koinos_obj_serializer main.cpp)
target_link_libraries(koinos_obj_serializer Koinos::exception Koinos::crypto Koinos::types Koinos::util Boost::program_options)
install(TARGETS
   koinos_obj_serializer
   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
