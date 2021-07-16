
#include <koinos/pack/rt/json_fwd.hpp>
#include <koinos/pack/rt/reflect.hpp>
#include <koinos/pack/rt/json.hpp>
#include <koinos/pack/classes.hpp>
#include <koinos/log.hpp>

#include <boost/container/flat_map.hpp>
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
   uint64_t               value = 0;
};

}

KOINOS_REFLECT( koinos::koin::transfer_args, (from)(to)(value) );

namespace koinos::koin {

struct mint_args
{
   protocol::account_type to;
   uint64_t               value = 0;
};

}

KOINOS_REFLECT( koinos::koin::mint_args, (to)(value) );

namespace koinos::koin {

struct balance_of_args
{
   protocol::account_type owner;
};

}

KOINOS_REFLECT( koinos::koin::balance_of_args, (owner) );

namespace koinos::koin {

struct balance_of_result
{
   uint64_t balance = 0;
};

}

KOINOS_REFLECT( koinos::koin::balance_of_result, (balance) );

// TODO:  Reflect all result types

/**
 * Create a method by deserializing bytes from a JSON-ish type selector.
 *
 * That is, we process methods of the form:
 * {"type" : ..., "bytes" : ...}
 */

template< typename... Ts >
inline void from_json_bytes( const nlohmann::json& j, std::variant<Ts... >&v, uint32_t depth )
{
   static std::map< std::string, int64_t > to_tag = []()
   {
      std::map< std::string, int64_t > name_map;
      for( size_t i = 0; i < sizeof...(Ts); ++i )
      {
         std::string n;
         koinos::pack::util::variant_helper< Ts... >::get_typename_at( i, n );
         name_map[n] = i;
      }
      return name_map;
   }();

   depth++;
   if( !(depth <= KOINOS_PACK_MAX_RECURSION_DEPTH) ) throw koinos::pack::depth_violation( "Unpack depth exceeded" );
   if( !(j.is_object()) ) throw koinos::pack::json_type_mismatch( "Unexpected JSON type: object expected" );
   if( !(j.size() == 2) ) throw koinos::pack::json_type_mismatch( "Variant JSON type must only contain two fields" );
   if( !(j.contains( "type" )) ) throw koinos::pack::json_type_mismatch( "Variant JSON type must contain field 'type'" );
   if( !(j.contains( "bytes" )) ) throw koinos::pack::json_type_mismatch( "Variant JSON type must contain field 'bytes'" );

   auto bytes_json = j[ "bytes" ];
   if( !(bytes_json.is_string()) ) throw koinos::pack::json_type_mismatch( "Unexpected JSON type: string expected" );
   std::string bytes_str = bytes_json.get< std::string >();
   std::vector< char > bytes;
   koinos::pack::util::decode_multibase( bytes_str.c_str(), bytes_str.size(), bytes );

   auto type = j[ "type" ];
   int64_t index = -1;

   if( type.is_number_integer() )
   {
      index = type.get< int64_t >();
   }
   else if( type.is_string() )
   {
      auto itr = to_tag.find( type.get< std::string >() );
      if( !(itr != to_tag.end()) ) throw koinos::pack::json_type_mismatch( "Invalid type name in JSON variant" );
      index = itr->second;
   }
   else
   {
      if( !(false) ) throw koinos::pack::json_type_mismatch( "Variant JSON 'type' must be an unsigned integer or string" );
   }

   koinos::pack::util::variant_helper< Ts... >::init_variant( v, index );
   std::visit( [&]( auto& arg ){ koinos::pack::from_variable_blob( bytes, arg ); }, v );
}

template< typename... Ts >
boost::container::flat_map< std::string, size_t > create__map( std::variant< Ts... >* v = nullptr )
{
   boost::container::flat_map< std::string, size_t > result;

   for( size_t n=0; n<sizeof...(Ts); n++ )
   {
      std::string name;
      koinos::pack::util::variant_helper< Ts... >::get_typename_at( n, name );
      result[name] = n;
      std::cout << name << std::endl;
   }
   return result;
}

typedef std::variant<
   koinos::pow::pow_signature_data,
   koinos::pow::difficulty_metadata,
   koinos::koin::transfer_args,
   koinos::koin::mint_args,
   koinos::koin::balance_of_args,
   koinos::koin::balance_of_result
   > any_object;

size_t get_varint_size( const koinos::variable_blob& vb )
{
   size_t i;
   for( i=0; i<vb.size(); i++ )
   {
      if( (vb[i] & 0x80) == 0 )
         return (i+1);
   }
   return i;
}

void serialize_loop(char base)
{
   while(true)
   {
      std::string line;
      std::getline(std::cin, line);
      if( !std::cin )
         break;
      koinos::pack::json j = koinos::pack::json::parse(line);
      koinos::variable_blob vb;
      if( j["type"] == "bytes" )
      {
         std::string value = j["value"].get<std::string>();
         koinos::pack::util::decode_multibase( value.c_str(), value.size(), vb );
      }
      else
      {
         any_object obj;
         koinos::pack::from_json( j, obj );
         koinos::pack::to_variable_blob( vb, obj, false );
         // We need to strip the variable_blob selector from the output
         size_t n = get_varint_size(vb);
         vb.erase( vb.begin(), vb.begin()+n );
      }

      if( base == '\0' )
      {
         std::cout.put('\0');
         std::cout.write(vb.data(), vb.size());
         std::cout.flush();
      }
      else
      {
         std::vector<char> b64;
         koinos::pack::util::encode_multibase(vb.data(), vb.size(), b64, base);
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

      std::string line;
      std::getline(std::cin, line);
      if( !std::cin )
         break;
      koinos::pack::json j = koinos::pack::json::parse(line);
      any_object obj;
      from_json_bytes( j, obj, 0 );

      koinos::pack::json j2;
      koinos::pack::to_json( j2, obj );

      std::cout << j2.dump() << std::endl;
      std::cout.flush();
   }
}

#define HELP_OPTION              "help"
#define SERIALIZE_OPTION         "serialize"
#define DESERIALIZE_OPTION       "deserialize"
#define BASE_OPTION              "base"
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
         (BINARY_OPTION          ",n", boost::program_options::bool_switch()->default_value(false), "Output binary data")
         (BASE_OPTION            ",b", boost::program_options::value< std::string >()->default_value("M"), "Base to serialize to")
         ;

      boost::program_options::variables_map args;
      boost::program_options::store( boost::program_options::parse_command_line( argc, argv, options ), args );

      if( args.count( HELP_OPTION ) )
      {
         std::cout << options << std::endl;
         return EXIT_SUCCESS;
      }

      std::string str_base = args[BASE_OPTION].as< std::string >();
      if( str_base.size() != 1 )
      {
         std::cerr << "Base must be exactly 1 character" << std::endl;
         return EXIT_FAILURE;
      }
      char base = str_base[0];
      if( args[BINARY_OPTION].as< bool >() )
         base = '\0';

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
         serialize_loop(base);
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
