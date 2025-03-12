#include <iostream>

#include <koinos/chain/chain.pb.h>
#include <koinos/chain/system_call_ids.pb.h>
#include <koinos/chain/value.pb.h>
#include <koinos/contracts/governance/governance.pb.h>
#include <koinos/contracts/name_service/name_service.pb.h>
#include <koinos/crypto/merkle_tree.hpp>
#include <koinos/crypto/multihash.hpp>
#include <koinos/log.hpp>
#include <koinos/protocol/protocol.pb.h>
#include <koinos/util/base58.hpp>
#include <koinos/util/base64.hpp>
#include <koinos/util/hex.hpp>

using namespace koinos;
using namespace std::string_literals;

int main( int argc, char** argv, char** envp )
{
  initialize_logging( "koinos_governance_proposal", {}, "info" );

  const auto chain_id = util::from_base64< std::string >( "EiBZK_GGVP0H_fXVAM3j6EAuz3-B-l3ejxRSewi7qIBfSA=="s );

  const auto old_koin_address     = util::from_base58< std::string >( "15DJN4a8SgrbGhhGksSBASiSYjGnMU8dGL"s );
  const auto old_vhp_address      = util::from_base58< std::string >( "18tWNU7E4yuQzz7hMVpceb9ixmaWLVyQsr"s );
  const auto new_koin_address     = util::from_base58< std::string >( "19GYjDBVXU7keLbYvMLazsGQn3GTWHjHkK"s );
  const auto new_vhp_address      = util::from_base58< std::string >( "12Y5vW6gk8GceH53YfRkRre2Rrcsgw7Naq"s );
  const auto name_service_address = util::from_base58< std::string >( "19WxDJ9Kcvx4VqQFkpwVmwVEy1hMuwXtQE"s );
  const auto nickname_address     = util::from_base58< std::string >( "1KD9Es7LBBjA1FY3ViCgQJ7e6WH1ipKbhz"s );

  const auto payee      = util::from_base58< std::string >( "1Nx58cnvGdkVvz9h6VQytqNyZiGQTQD8EH"s );
  const auto payer      = util::from_base58< std::string >( "162GhJwsciDiKsgwzj2t6VoFHt3RMzGKdG"s );
  const auto governance = util::from_base58< std::string >( "19qj51eTbSFJYU7ZagudkpxPgNSzPMfdPX"s );

  contracts::governance::submit_proposal_arguments proposal;

  // Set new Koin contract as system contract
  auto op                  = proposal.add_operations();
  auto set_system_contract = op->mutable_set_system_contract();
  set_system_contract->set_contract_id( new_koin_address );
  set_system_contract->set_system_contract( true );

  // Set get_account_rc to new Koin contract
  op                   = proposal.add_operations();
  auto set_system_call = op->mutable_set_system_call();
  set_system_call->set_call_id( chain::system_call_id::get_account_rc );
  auto call_bundle = set_system_call->mutable_target()->mutable_system_call_bundle();
  call_bundle->set_contract_id( new_koin_address );
  call_bundle->set_entry_point( 0x2d464aab );

  // Set consume_account_rc to new Koin contract
  op              = proposal.add_operations();
  set_system_call = op->mutable_set_system_call();
  set_system_call->set_call_id( chain::system_call_id::consume_account_rc );
  call_bundle = set_system_call->mutable_target()->mutable_system_call_bundle();
  call_bundle->set_contract_id( new_koin_address );
  call_bundle->set_entry_point( 0x80e3f5c9 );

  // Set name_service record for 'koin' to new Koin contract
  op                 = proposal.add_operations();
  auto call_contract = op->mutable_call_contract();
  call_contract->set_contract_id( name_service_address );
  call_contract->set_entry_point( 0xe248c73a );
  auto set_record = contracts::name_service::set_record_arguments();
  set_record.set_name( "koin" );
  set_record.set_address( new_koin_address );
  call_contract->set_args( util::converter::as< std::string >( set_record ) );

  // Transfer '@koin' nickname to new Koin contract
  op            = proposal.add_operations();
  call_contract = op->mutable_call_contract();
  call_contract->set_contract_id( nickname_address );
  call_contract->set_entry_point( 1'560'272'691 );
  call_contract->set_args( util::from_base64< std::string >( "CgRrb2luEhkAWrG7DutiUIS3gYyjz82aGDNnN76l-vLAGAE=" ) );

  // Set old Koin contract as not system contract
  op                  = proposal.add_operations();
  set_system_contract = op->mutable_set_system_contract();
  set_system_contract->set_contract_id( old_koin_address );
  set_system_contract->set_system_contract( false );

  // Set new VHP contract as system contract
  op                  = proposal.add_operations();
  set_system_contract = op->mutable_set_system_contract();
  set_system_contract->set_contract_id( new_vhp_address );
  set_system_contract->set_system_contract( true );

  // Set name_service record for 'vhp' to new Vhp contract
  op            = proposal.add_operations();
  call_contract = op->mutable_call_contract();
  call_contract->set_contract_id( name_service_address );
  call_contract->set_entry_point( 0xe248c73a );
  set_record.set_name( "vhp" );
  set_record.set_address( new_vhp_address );
  call_contract->set_args( util::converter::as< std::string >( set_record ) );

  // Transfer '@vhp' nickname to new Koin contract
  op            = proposal.add_operations();
  call_contract = op->mutable_call_contract();
  call_contract->set_contract_id( nickname_address );
  call_contract->set_entry_point( 1'560'272'691 );
  call_contract->set_args( util::from_base64< std::string >( "CgN2aHASGQAQ2SEwlwdS_2h6vwi4JliA2LerasI16k4YAQ==" ) );

  // Set old VHP contract as not system contract
  op                  = proposal.add_operations();
  set_system_contract = op->mutable_set_system_contract();
  set_system_contract->set_contract_id( old_vhp_address );
  set_system_contract->set_system_contract( false );

  proposal.set_fee( 60ull * 100'000'000ull ); // 60 KOIN

  // Calculate operation merkle root
  std::vector< crypto::multihash > operations;
  operations.reserve( proposal.operations().size() );

  for( const auto& op: proposal.operations() )
  {
    operations.emplace_back( crypto::hash( crypto::multicodec::sha2_256, op ) );
  }

  auto operation_merkle_tree = crypto::merkle_tree( crypto::multicodec::sha2_256, operations );
  proposal.set_operation_merkle_root( util::converter::as< std::string >( operation_merkle_tree.root()->hash() ) );

  LOG( info ) << "Proposal ID: " << util::to_hex( proposal.operation_merkle_root() );

  protocol::transaction trx;
  op            = trx.add_operations();
  call_contract = op->mutable_call_contract();
  call_contract->set_contract_id( governance );
  call_contract->set_entry_point( 0xe74b785c );
  call_contract->set_args( util::converter::as< std::string >( proposal ) );

  chain::value_type nonce_value;
  nonce_value.set_uint64_value( 1 );

  auto header = trx.mutable_header();
  header->set_nonce( util::converter::as< std::string >( nonce_value ) );
  header->set_rc_limit( 100'000'000 ); // 10 Mana
  header->set_chain_id( chain_id );
  header->set_payee( payee );
  header->set_payer( payer );

  operations.clear();
  operations.reserve( trx.operations().size() );

  for( const auto& op: trx.operations() )
  {
    operations.emplace_back( crypto::hash( crypto::multicodec::sha2_256, op ) );
  }

  operation_merkle_tree = crypto::merkle_tree( crypto::multicodec::sha2_256, operations );
  header->set_operation_merkle_root( util::converter::as< std::string >( operation_merkle_tree.root()->hash() ) );
  trx.set_id( util::converter::as< std::string >( crypto::hash( crypto::multicodec::sha2_256, *header ) ) );

  LOG( info ) << "Unsigned Transaction: " << util::to_base64( trx );

  LOG( info ) << "Transaction: " << trx;

  return EXIT_SUCCESS;
}
