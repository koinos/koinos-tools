#include <iostream>
#include <fstream>

#include <boost/program_options.hpp>

#include <google/protobuf/util/json_util.h>

#include <koinos/conversion.hpp>
#include <koinos/exception.hpp>
#include <koinos/log.hpp>
#include <koinos/crypto/elliptic.hpp>
#include <koinos/crypto/multihash.hpp>

#include <koinos/rpc/chain/chain_rpc.pb.h>
#include <koinos/protocol/protocol.pb.h>

// Command line option definitions
#define HELP_OPTION        "help"
#define HELP_FLAG          "h"

#define PRIVATE_KEY_OPTION "private-key"
#define PRIVATE_KEY_FLAG   "p"

#define WRAP_OPTION        "wrap"
#define WRAP_FLAG          "w"

using namespace koinos;

// Sign the given transaction
void sign_transaction( protocol::transaction& transaction, crypto::private_key& transaction_signing_key )
{
   // Signature is on the hash of the active data
   auto trx_id = crypto::hash( crypto::multicodec::sha2_256, transaction.active() );
   transaction.set_id( converter::as< std::string >( trx_id ) );
   transaction.set_signature_data( converter::as< std::string >( transaction_signing_key.sign_compact( trx_id ) ) );
}

// Wrap the given transaction in a request
rpc::chain::chain_request wrap_transaction( protocol::transaction& transaction )
{
   rpc::chain::chain_request req;
   auto submit_transaction = req.mutable_submit_transaction();
   submit_transaction->mutable_transaction()->CopyFrom( transaction );
   submit_transaction->set_verify_passive_data( true );
   submit_transaction->set_verify_transaction_signature( true );

   return req;
}

// Read a base58 WIF private key from the given file
crypto::private_key read_keyfile( std::string key_filename )
{
   // Read base58 wif string from given file
   std::string key_string;
   std::ifstream instream;
   instream.open( key_filename );
   std::getline( instream, key_string );
   instream.close();

   // Create and return the key from the wif
   auto key = crypto::private_key::from_wif( key_string );
   return key;
}

int main( int argc, char** argv )
{
   try
   {
      // Setup command line options
      boost::program_options::options_description options( "Options" );
      options.add_options()
      ( HELP_OPTION "," HELP_FLAG,               "print usage message" )
      ( PRIVATE_KEY_OPTION "," PRIVATE_KEY_FLAG, boost::program_options::value< std::string >()->default_value( "private.key" ), "private key file" )
      ( WRAP_OPTION "," WRAP_FLAG,               "wrap signed transaction in a request" )
      ;

      // Parse command-line options
      boost::program_options::variables_map vm;
      boost::program_options::store( boost::program_options::parse_command_line( argc, argv, options ), vm );

      // Handle help message
      if ( vm.count( HELP_OPTION ) )
      {
         std::cout << "Koinos Transaction Signing Tool" << std::endl;
         std::cout << "Accepts a json transaction to sign via STDIN" << std::endl;
         std::cout << "Returns the signed transaction via STDOUT" << std::endl << std::endl;
         std::cout << options << std::endl;
         return EXIT_SUCCESS;
      }

      // Read options into variables
      std::string key_filename = vm[ PRIVATE_KEY_OPTION ].as< std::string >();
      bool wrap                = vm.count( WRAP_OPTION );

      // Read the keyfile
      auto private_key = read_keyfile( key_filename );

      // Read STDIN to a string
      std::string transaction_json;
      std::getline( std::cin, transaction_json );

      // Parse and deserialize the json to a transaction
      google::protobuf::util::JsonParseOptions json_opts;
      json_opts.ignore_unknown_fields = true;
      json_opts.case_insensitive_enum_parsing = true;

      protocol::transaction transaction;
      google::protobuf::util::JsonStringToMessage( transaction_json, &transaction, json_opts );

      // Sign the transaction
      sign_transaction( transaction, private_key );

      if (wrap) // Wrap the transaction if requested
      {
         auto request = wrap_transaction( transaction );
         std::cout << request << std::endl;
      }
      else // Else simply output the signed transaction
      {
         std::cout << transaction << std::endl;
      }

      return EXIT_SUCCESS;
   }
   catch ( const boost::exception& e )
   {
      LOG(fatal) << boost::diagnostic_information( e ) << std::endl;
   }
   catch ( const std::exception& e )
   {
      LOG(fatal) << e.what() << std::endl;
   }
   catch ( ... )
   {
      LOG(fatal) << "unknown exception" << std::endl;
   }

   return EXIT_FAILURE;
}
