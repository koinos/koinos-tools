#include <iostream>
#include <fstream>

#include <boost/program_options.hpp>

#include <koinos/exception.hpp>
#include <koinos/log.hpp>
#include <koinos/crypto/elliptic.hpp>
#include <koinos/util/conversion.hpp>
#include <koinos/util/base64.hpp>

// Command line option definitions
#define HELP_OPTION "help"
#define HELP_FLAG "h"

#define PRIVATE_KEY_OPTION "private-key"
#define PRIVATE_KEY_FLAG "p"

#define INPUT_OPTION "input"
#define INPUT_FLAG "i"

using namespace koinos;

// Read a base58 WIF private key from the given file
crypto::private_key read_keyfile(std::string key_filename)
{
   // Read base58 wif string from given file
   std::string key_string;
   std::ifstream instream;
   instream.open(key_filename);
   std::getline(instream, key_string);
   instream.close();

   // Create and return the key from the wif
   auto key = crypto::private_key::from_wif(key_string);
   return key;
}

int main(int argc, char **argv)
{
   try
   {
      // Setup command line options
      boost::program_options::options_description options("Options");
      options.add_options()
      (HELP_OPTION "," HELP_FLAG, "print usage message")
      (PRIVATE_KEY_OPTION "," PRIVATE_KEY_FLAG, boost::program_options::value<std::string>()->default_value("private.key"), "private key file")
      (INPUT_OPTION "," INPUT_FLAG, boost::program_options::value< std::string >()->default_value( "" ), "input to use to generate the random proof (base64 encoded)");

      // Parse command-line options
      boost::program_options::variables_map vm;
      boost::program_options::store(boost::program_options::parse_command_line(argc, argv, options), vm);

      // Handle help message
      if (vm.count(HELP_OPTION))
      {
         std::cout << "Koinos Random Proof Generator" << std::endl;
         std::cout << "Accepts an input to use to generate the random proof (base64 encoded)" << std::endl;
         std::cout << "Returns the random proof and its hash (base64 encoded and in a JSON format) via STDOUT" << std::endl
                   << std::endl;
         std::cout << options << std::endl;
         return EXIT_SUCCESS;
      }

      // Read options into variables
      std::string key_filename = vm[PRIVATE_KEY_OPTION].as<std::string>();

      // Read the keyfile
      auto private_key = read_keyfile(key_filename);

      std::string input = vm[INPUT_OPTION].as<std::string>();

      auto [ proof, proof_hash ] = private_key.generate_random_proof( util::from_base64< std::string >( input ) );

      // output proof and its hash base64 encoded and in a JSON format
      std::string json_str = "{ \"proof\": \"" + util::to_base64< std::string >( proof ) + "\", \"proof_hash\": \"" + util::to_base64< std::string >( util::converter::as< std::string >( proof_hash ) ) + "\" }";

      std::cout << json_str << std::endl;

      return EXIT_SUCCESS;
   }
   catch (const boost::exception &e)
   {
      LOG(fatal) << boost::diagnostic_information(e) << std::endl;
   }
   catch (const std::exception &e)
   {
      LOG(fatal) << e.what() << std::endl;
   }
   catch (...)
   {
      LOG(fatal) << "unknown exception" << std::endl;
   }

   return EXIT_FAILURE;
}
