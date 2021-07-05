
#include <koinos/pack/rt/json_fwd.hpp>
#include <koinos/pack/rt/reflect.hpp>
#include <koinos/pack/rt/json.hpp>
#include <koinos/pack/classes.hpp>
#include <koinos/log.hpp>

#include <boost/exception/all.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <variant>

namespace koinos::pow {

struct pow_signature_data
{
   uint256_t        nonce;
   fixed_blob< 65 > recoverable_signature;
};

}

KOINOS_REFLECT( koinos::pow::pow_signature_data, (nonce)(recoverable_signature) )

namespace koinos::pow {

struct difficulty_metadata
{
   uint256_t      difficulty_target = 0;
   timestamp_type last_block_time = timestamp_type( 0 );
   timestamp_type block_window_time = timestamp_type( 0 );
   uint32_t       averaging_window = 0;
};

}

KOINOS_REFLECT( koinos::pow::difficulty_metadata,
   (difficulty_target)
   (last_block_time)
   (block_window_time)
   (averaging_window)
)

namespace koinos::koin {

struct transfer_args
{
   protocol::account_type from;
   protocol::account_type to;
   uint64_t               value;
};

}

KOINOS_REFLECT( koinos::koin::transfer_args, (from)(to)(value) );

namespace koinos::koin {

struct mint_args
{
   protocol::account_type to;
   uint64_t               value;
};

}

KOINOS_REFLECT( koinos::koin::mint_args, (to)(value) );

typedef std::variant<
   koinos::pow::pow_signature_data,
   koinos::pow::difficulty_metadata,
   koinos::koin::transfer_args,
   koinos::koin::mint_args
   > any_object;

void serialize_loop(bool binary)
{
   while(true)
   {
      std::string line;
      std::getline(std::cin, line);
      if( !std::cin )
         break;
      koinos::pack::json j = koinos::pack::json::parse(line);
      any_object obj;
      koinos::pack::from_json( j, obj );
      koinos::variable_blob vb = koinos::pack::to_variable_blob( obj );
      if( binary )
      {
         std::cout.put('\0');
         std::cout.write(vb.data(), vb.size());
         std::cout.flush();
      }
      else
      {
         std::vector<char> b64;
         koinos::pack::util::encode_multibase(vb.data(), vb.size(), b64, 'M');
         std::cout.write(b64.data(), b64.size());
         std::cout << std::endl;
         std::cout.flush();
      }
   }
}

void deserialize_loop()
{
   while(true)
   {
      if( !std::cin )
         break;
      char c = std::cin.get();
      if( !std::cin )
         break;

      if( c == '\0' )
      {
         /*
         koinos::variable_blob vb;
         from_binary( std::cin, vb );
         if( !std::cin )
            break;
         any_object obj;
         */
         // TODO finish binary decoding
         throw koinos::pack::base_decode_error( "Binary decoding not supported yet" );
      }
      else
      {
         std::string line;
         std::getline(std::cin, line);
         if( !std::cin )
            break;
         std::string mb_line = std::string(1, c)+line;

         std::vector<char> temp;
         koinos::pack::util::decode_multibase(mb_line.c_str(), mb_line.size(), temp);

         any_object obj;
         koinos::pack::from_variable_blob( temp, obj );

         koinos::pack::json j;
         koinos::pack::to_json( j, obj );

         std::cout << j.dump() << std::endl;
         std::cout.flush();
      }
   }
}

#define HELP_OPTION              "help"
#define SERIALIZE_OPTION         "serialize"
#define DESERIALIZE_OPTION       "deserialize"
#define BINARY_OPTION            "binary"

int main( int argc, char** argv, char** envp )
{
   try
   {
      boost::program_options::options_description options;
      options.add_options()
         (HELP_OPTION            ",h", "Print this help message and exit.")
         (SERIALIZE_OPTION       ",s", "Serialize to binary form")
         (DESERIALIZE_OPTION     ",d", "Deserialize from binary form")
         (BINARY_OPTION          ",b", "Don't base64-encode binary")
         ;

      boost::program_options::variables_map args;
      boost::program_options::store( boost::program_options::parse_command_line( argc, argv, options ), args );

      if( args.count( HELP_OPTION ) )
      {
         std::cout << options << std::endl;
         return EXIT_SUCCESS;
      }

      bool binary = false;

      if( args.count( BINARY_OPTION ) )
         binary = true;

      if( args.count( DESERIALIZE_OPTION ) )
      {
         if( args.count( SERIALIZE_OPTION ) )
         {
            std::cerr << "Cannot specify both -s and -d" << std::endl;
            return EXIT_FAILURE;
         }
         deserialize_loop();
      }
      else
      {
         serialize_loop(binary);
      }

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
      LOG(fatal) << "Unknown exception" << std::endl;
   }

   return EXIT_FAILURE;
}
