#include <iostream>
#include <fstream>
#include <filesystem>

#include <boost/program_options.hpp>

#include <koinos/conversion.hpp>
#include <koinos/exception.hpp>
#include <koinos/log.hpp>
#include <koinos/crypto/elliptic.hpp>
#include <koinos/crypto/multihash.hpp>
#include <koinos/mq/client.hpp>
#include <koinos/util.hpp>

#include <koinos/protocol/protocol.pb.h>
#include <koinos/rpc/chain/chain_rpc.pb.h>

#define HELP_OPTION               "help"
#define PRIVATE_KEY_FILE_OPTION   "private-key-file"
#define PRIVATE_KEY_FILE_DEFAULT  "private.key"
#define AMQP_OPTION               "amqp"
#define AMQP_DEFAULT              "amqp://guest:guest@localhost:5672/"
#define CONTRACT_OPTION           "contract"
#define CALL_ID_OPTION            "call-id"
#define ENTRY_POINT_OPTION        "entry-point"
#define CONTRACT_ID_OPTION        "contract-id"
#define UPLOAD_OPTION             "upload"
#define OVERRIDE_OPTION           "override"

using namespace boost;
using namespace koinos;

uint64_t get_next_nonce( std::shared_ptr< mq::client > client, const std::string& account );
void submit_transaction( std::shared_ptr< mq::client > client, const protocol::transaction& t );

int main( int argc, char** argv )
{
   try
   {
      program_options::options_description options( "Options" );
      options.add_options()
         (HELP_OPTION              ",h", "Print usage message")
         (AMQP_OPTION              ",a", program_options::value< std::string >()->default_value( AMQP_DEFAULT ), "AMQP server URL")
         (PRIVATE_KEY_FILE_OPTION  ",p", program_options::value< std::string >()->default_value( PRIVATE_KEY_FILE_DEFAULT ), "The private key file")

         // --upload arguments
         (UPLOAD_OPTION                , "Run in upload mode")
         (CONTRACT_OPTION          ",c", program_options::value< std::string >(), "The wasm contract")

         // --override arguments
         (OVERRIDE_OPTION              , "Run in override mode")
         (CALL_ID_OPTION           ",o", program_options::value< uint32_t >()   , "The system call ID to override")
         (ENTRY_POINT_OPTION       ",e", program_options::value< uint32_t >()   , "The contract entry point for override mode")
         (CONTRACT_ID_OPTION       ",i", program_options::value< std::string >(), "The contract ID for override mode")
      ;

      program_options::variables_map args;
      program_options::store( program_options::parse_command_line( argc, argv, options ), args );

      koinos::initialize_logging( "koinos_contract_uploader", {}, "info" );

      if ( args.count( HELP_OPTION ) )
      {
         std::cout << options << std::endl;
         return EXIT_SUCCESS;
      }

      auto client = std::make_shared< mq::client >();

      auto ec = client->connect( args[ AMQP_OPTION ].as< std::string >(), mq::retry_policy::none );

      if ( ec != mq::error_code::success )
         KOINOS_THROW( koinos::exception, "Unable to connect to AMQP server " );

      std::filesystem::path private_key_file( args[ PRIVATE_KEY_FILE_OPTION ].as< std::string >() );

      KOINOS_ASSERT(
         std::filesystem::exists( private_key_file ),
         koinos::exception,
         "Unable to find private key file at: ${loc}", ("loc", private_key_file.string())
      );

      std::ifstream ifs( private_key_file );
      std::string private_key_wif( ( std::istreambuf_iterator< char >( ifs ) ), ( std::istreambuf_iterator< char >() ) );

      crypto::private_key signing_key = crypto::private_key::from_wif( private_key_wif );
      std::string public_address      = signing_key.get_public_key().to_address_bytes();

      protocol::transaction transaction;
      protocol::active_transaction_data active_data;

      if ( args.count( UPLOAD_OPTION ) )
      {
         std::filesystem::path contract_file = args[ CONTRACT_OPTION ].as< std::string >();

         KOINOS_ASSERT(
            std::filesystem::exists( contract_file ),
            koinos::exception,
            "Unable to find contract file at: ${loc}", ("loc", contract_file.string())
         );

         std::ifstream contract( contract_file, std::ios::binary );

         std::vector< char > bytecode( (std::istreambuf_iterator< char >( contract )), std::istreambuf_iterator< char >() );

         auto op = active_data.add_operations();
         auto upload_contract = op->mutable_upload_contract();

         auto contract_id = koinos::crypto::hash( crypto::multicodec::ripemd_160, signing_key.get_public_key().to_address_bytes() );
         upload_contract->set_contract_id( converter::as< std::string >( contract_id ) );
         upload_contract->set_bytecode( converter::as< std::string >( bytecode ) );

         LOG(info) << "Attempting to upload contract with ID: " << to_hex( upload_contract->contract_id() );
      }
      else if ( args.count( OVERRIDE_OPTION ) )
      {
         KOINOS_ASSERT(
            args.count( CALL_ID_OPTION ),
            koinos::exception,
            "The call ID option is required"
         );

         KOINOS_ASSERT(
            args.count( ENTRY_POINT_OPTION ),
            koinos::exception,
            "The entry point is required"
         );

         KOINOS_ASSERT(
            args.count( CONTRACT_ID_OPTION ),
            koinos::exception,
            "The contract ID is required"
         );

         auto op = active_data.add_operations();
         auto set_system_call = op->mutable_set_system_call();
         set_system_call->set_call_id( args[ CALL_ID_OPTION ].as< uint32_t >() );
         auto system_call_bundle = set_system_call->mutable_target()->mutable_system_call_bundle();
         system_call_bundle->set_contract_id( from_hex( args[ CONTRACT_ID_OPTION ].as< std::string >() ) );
         system_call_bundle->set_entry_point( args[ ENTRY_POINT_OPTION ].as< uint32_t >() );

         LOG(info) << "Attempting to apply the system call override";
      }
      else
      {
         KOINOS_THROW( koinos::exception, "Use --upload or --override when invoking the tool, see --help for more information" );
      }

      active_data.set_resource_limit( 10'000'000 );
      active_data.set_nonce( get_next_nonce( client, public_address ) );

      transaction.set_active( converter::as< std::string >( active_data ) );

      auto trx_id = crypto::hash( crypto::multicodec::sha2_256, transaction.active() );
      transaction.set_id( converter::as< std::string >( trx_id ) );
      transaction.set_signature_data( converter::as< std::string >( signing_key.sign_compact( trx_id ) ) );

      submit_transaction( client, transaction );

      LOG(info) << "Transaction successfully submitted";

      return EXIT_SUCCESS;
   }
   catch ( const std::exception& e )
   {
      LOG(fatal) << e.what() << std::endl;
   }
   catch ( const boost::exception& e )
   {
      LOG(fatal) << boost::diagnostic_information( e ) << std::endl;
   }
   catch ( ... )
   {
      LOG(fatal) << "unknown exception" << std::endl;
   }

   return EXIT_FAILURE;
}

uint64_t get_next_nonce( std::shared_ptr< mq::client > client, const std::string& account )
{
   uint64_t nonce = 0;

   rpc::chain::chain_request req;
   auto get_account_nonce = req.mutable_get_account_nonce();
   get_account_nonce->set_account( account );

   auto future = client->rpc( service::chain, converter::as< std::string >( req ), 750 /* ms */, mq::retry_policy::none );
   auto resp = converter::to< rpc::chain::chain_response >( future.get() );

   KOINOS_ASSERT( !resp.has_error(), koinos::exception, "received error response from chain: ${e}", ("e", resp.error()) );
   KOINOS_ASSERT( resp.has_get_account_nonce(), koinos::exception, "unexpected response from chain" );

   return resp.get_account_nonce().nonce();
}

void submit_transaction( std::shared_ptr< mq::client > client, const protocol::transaction& t )
{
   rpc::chain::chain_request req;
   auto submit_transaction = req.mutable_submit_transaction();
   submit_transaction->mutable_transaction()->CopyFrom( t );
   submit_transaction->set_verify_passive_data( true );
   submit_transaction->set_verify_transaction_signature( true );

   auto future = client->rpc( service::chain, converter::as< std::string >( req ), 750 /* ms */, mq::retry_policy::none );
   auto resp = converter::to< rpc::chain::chain_response >( future.get() );

   KOINOS_ASSERT( !resp.has_error(), koinos::exception, "received error response from chain: ${e}", ("e", resp.error()) );
   KOINOS_ASSERT( resp.has_submit_transaction(), koinos::exception, "unexpected response from chain" );
}
