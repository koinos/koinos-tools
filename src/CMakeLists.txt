add_executable(kcs4_governance_proposal kcs4_governance_proposal.cpp)
target_link_libraries(
  kcs4_governance_proposal
    PRIVATE
      Koinos::crypto
      Koinos::exception
      Koinos::log
      Koinos::proto
      Koinos::util)

koinos_add_format(TARGET kcs4_governance_proposal)

add_executable(koinos_genesis_tool koinos_genesis_tool.cpp)
target_link_libraries(
  koinos_genesis_tool
    PRIVATE
      Koinos::crypto
      Koinos::exception
      Koinos::log
      Koinos::proto
      Koinos::util)

koinos_add_format(TARGET koinos_genesis_tool)

add_executable(koinos_get_dev_key koinos_get_dev_key.cpp)
target_link_libraries(
  koinos_get_dev_key
    PRIVATE
      Koinos::crypto
      Koinos::exception
      Koinos::log
      Koinos::proto
      Koinos::util)

koinos_add_format(TARGET koinos_get_dev_key)

add_executable(koinos_random_proof_generator koinos_random_proof_generator.cpp)
target_link_libraries(
  koinos_random_proof_generator
    PRIVATE
      Koinos::crypto
      Koinos::exception
      Koinos::log
      Koinos::proto
      Koinos::util)

koinos_add_format(TARGET koinos_random_proof_generator)

add_executable(koinos_transaction_signer koinos_transaction_signer.cpp)
target_link_libraries(
  koinos_transaction_signer
    PRIVATE
      Koinos::crypto
      Koinos::exception
      Koinos::log
      Koinos::proto
      Koinos::util)

koinos_add_format(TARGET koinos_transaction_signer)

koinos_install(
  TARGETS
    kcs4_governance_proposal
    koinos_genesis_tool
    koinos_get_dev_key
    koinos_random_proof_generator
)
