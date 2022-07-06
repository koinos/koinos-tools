#include <iostream>
#include <fstream>
#include <filesystem>

#include <boost/program_options.hpp>

#include <koinos/exception.hpp>
#include <koinos/log.hpp>
#include <koinos/crypto/elliptic.hpp>
#include <koinos/crypto/multihash.hpp>
#include <koinos/mq/client.hpp>
#include <koinos/util/base58.hpp>
#include <koinos/util/conversion.hpp>
#include <koinos/util/services.hpp>

#include <koinos/chain/object_spaces.pb.h>
#include <koinos/protocol/protocol.pb.h>
#include <koinos/rpc/chain/chain_rpc.pb.h>

#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>

#define HELP_OPTION               "help"

using namespace koinos;
using namespace boost;

namespace state {

namespace zone {

const auto kernel = std::string{};

} // zone

namespace space {

const chain::object_space metadata()
{
   chain::object_space s;
   s.set_system( true );
   s.set_zone( zone::kernel );
   s.set_id( chain::system_space_id::metadata );
   return s;
}

} // space

namespace key {

const auto genesis_key                = util::converter::as< std::string >( crypto::hash( crypto::multicodec::sha2_256, std::string( "object_key::genesis_key" ) ) );
const auto resource_limit_data        = util::converter::as< std::string >( crypto::hash( crypto::multicodec::sha2_256, std::string( "object_key::resource_limit_data" ) ) );
const auto max_account_resources      = util::converter::as< std::string >( crypto::hash( crypto::multicodec::sha2_256, std::string( "object_key::max_account_resources" ) ) );
const auto protocol_descriptor        = util::converter::as< std::string >( crypto::hash( crypto::multicodec::sha2_256, std::string( "object_key::protocol_descriptor" ) ) );
const auto compute_bandwidth_registry = util::converter::as< std::string >( crypto::hash( crypto::multicodec::sha2_256, std::string( "object_key::compute_bandwidth_registry" ) ) );
const auto block_hash_code            = util::converter::as< std::string >( crypto::hash( crypto::multicodec::sha2_256, std::string( "object_key::block_hash_code" ) ) );

} // key

} // state

chain::genesis_data default_genesis_data();

int main( int argc, char** argv )
{
   try
   {
      program_options::options_description options( "Options" );
      options.add_options()
         (HELP_OPTION        ",h", "Print usage message")
      ;

      program_options::variables_map args;
      program_options::store( program_options::parse_command_line( argc, argv, options ), args );

      koinos::initialize_logging( "koinos_genesis_tool", {}, "info" );

      if ( args.count( HELP_OPTION ) )
      {
         std::cout << options << std::endl;
         return EXIT_SUCCESS;
      }

      chain::genesis_data gdata = default_genesis_data();

      std::string out;
      google::protobuf::util::MessageToJsonString( gdata, &out );
      std::cout << out;

   }
   catch ( const std::exception& e )
   {
      LOG(error) << "Error: " << e.what();
   }

   return EXIT_SUCCESS;
}

chain::genesis_data default_genesis_data()
{
   chain::genesis_data gdata;

   crypto::private_key genesis_address = crypto::private_key::from_wif( "5JpEWwc5kaDTSfNNKkRLMFve6BwuARyafeCTMNK4ky9Dini8xvk" );

   auto entry = gdata.add_entries();
   entry->set_key( state::key::genesis_key );
   entry->set_value( genesis_address.get_public_key().to_address_bytes() );
   *entry->mutable_space() = state::space::metadata();

   chain::resource_limit_data rd;

   rd.set_disk_storage_cost( 10 );
   rd.set_disk_storage_limit( 409'600 );

   rd.set_network_bandwidth_cost( 5 );
   rd.set_network_bandwidth_limit( 1'048'576 );

   rd.set_compute_bandwidth_cost( 1 );
   rd.set_compute_bandwidth_limit( 100'000'000 );

   entry = gdata.add_entries();
   entry->set_key( state::key::resource_limit_data );
   entry->set_value( util::converter::as< std::string >( rd ) );
   *entry->mutable_space() = state::space::metadata();

   chain::max_account_resources mar;

   mar.set_value( 10'000'000 );

   entry = gdata.add_entries();
   entry->set_key( state::key::max_account_resources );
   entry->set_value( util::converter::as< std::string >( mar ) );
   *entry->mutable_space() = state::space::metadata();

   entry = gdata.add_entries();
   entry->set_key( state::key::protocol_descriptor );

   // protoc --experimental_allow_proto3_optional --descriptor_set_out=build/koinos_protocol.pb --include_imports `find koinos -name 'protocol.proto'`
   std::string protocol_descriptor = util::from_hex< std::string >( "0x0AC33B0A20676F6F676C652F70726F746F6275662F64657363726970746F722E70726F746F120F676F6F676C652E70726F746F627566224D0A1146696C6544657363726970746F7253657412380A0466696C6518012003280B32242E676F6F676C652E70726F746F6275662E46696C6544657363726970746F7250726F746F520466696C6522E4040A1346696C6544657363726970746F7250726F746F12120A046E616D6518012001280952046E616D6512180A077061636B61676518022001280952077061636B616765121E0A0A646570656E64656E6379180320032809520A646570656E64656E6379122B0A117075626C69635F646570656E64656E6379180A2003280552107075626C6963446570656E64656E637912270A0F7765616B5F646570656E64656E6379180B20032805520E7765616B446570656E64656E637912430A0C6D6573736167655F7479706518042003280B32202E676F6F676C652E70726F746F6275662E44657363726970746F7250726F746F520B6D6573736167655479706512410A09656E756D5F7479706518052003280B32242E676F6F676C652E70726F746F6275662E456E756D44657363726970746F7250726F746F5208656E756D5479706512410A077365727669636518062003280B32272E676F6F676C652E70726F746F6275662E5365727669636544657363726970746F7250726F746F52077365727669636512430A09657874656E73696F6E18072003280B32252E676F6F676C652E70726F746F6275662E4669656C6444657363726970746F7250726F746F5209657874656E73696F6E12360A076F7074696F6E7318082001280B321C2E676F6F676C652E70726F746F6275662E46696C654F7074696F6E7352076F7074696F6E7312490A10736F757263655F636F64655F696E666F18092001280B321F2E676F6F676C652E70726F746F6275662E536F75726365436F6465496E666F520E736F75726365436F6465496E666F12160A0673796E746178180C20012809520673796E74617822B9060A0F44657363726970746F7250726F746F12120A046E616D6518012001280952046E616D65123B0A056669656C6418022003280B32252E676F6F676C652E70726F746F6275662E4669656C6444657363726970746F7250726F746F52056669656C6412430A09657874656E73696F6E18062003280B32252E676F6F676C652E70726F746F6275662E4669656C6444657363726970746F7250726F746F5209657874656E73696F6E12410A0B6E65737465645F7479706518032003280B32202E676F6F676C652E70726F746F6275662E44657363726970746F7250726F746F520A6E65737465645479706512410A09656E756D5F7479706518042003280B32242E676F6F676C652E70726F746F6275662E456E756D44657363726970746F7250726F746F5208656E756D5479706512580A0F657874656E73696F6E5F72616E676518052003280B322F2E676F6F676C652E70726F746F6275662E44657363726970746F7250726F746F2E457874656E73696F6E52616E6765520E657874656E73696F6E52616E676512440A0A6F6E656F665F6465636C18082003280B32252E676F6F676C652E70726F746F6275662E4F6E656F6644657363726970746F7250726F746F52096F6E656F664465636C12390A076F7074696F6E7318072001280B321F2E676F6F676C652E70726F746F6275662E4D6573736167654F7074696F6E7352076F7074696F6E7312550A0E72657365727665645F72616E676518092003280B322E2E676F6F676C652E70726F746F6275662E44657363726970746F7250726F746F2E526573657276656452616E6765520D726573657276656452616E676512230A0D72657365727665645F6E616D65180A20032809520C72657365727665644E616D651A7A0A0E457874656E73696F6E52616E676512140A0573746172741801200128055205737461727412100A03656E641802200128055203656E6412400A076F7074696F6E7318032001280B32262E676F6F676C652E70726F746F6275662E457874656E73696F6E52616E67654F7074696F6E7352076F7074696F6E731A370A0D526573657276656452616E676512140A0573746172741801200128055205737461727412100A03656E641802200128055203656E64227C0A15457874656E73696F6E52616E67654F7074696F6E7312580A14756E696E7465727072657465645F6F7074696F6E18E7072003280B32242E676F6F676C652E70726F746F6275662E556E696E7465727072657465644F7074696F6E5213756E696E7465727072657465644F7074696F6E2A0908E80710808080800222C1060A144669656C6444657363726970746F7250726F746F12120A046E616D6518012001280952046E616D6512160A066E756D62657218032001280552066E756D62657212410A056C6162656C18042001280E322B2E676F6F676C652E70726F746F6275662E4669656C6444657363726970746F7250726F746F2E4C6162656C52056C6162656C123E0A047479706518052001280E322A2E676F6F676C652E70726F746F6275662E4669656C6444657363726970746F7250726F746F2E54797065520474797065121B0A09747970655F6E616D651806200128095208747970654E616D65121A0A08657874656E6465651802200128095208657874656E64656512230A0D64656661756C745F76616C7565180720012809520C64656661756C7456616C7565121F0A0B6F6E656F665F696E646578180920012805520A6F6E656F66496E646578121B0A096A736F6E5F6E616D65180A2001280952086A736F6E4E616D6512370A076F7074696F6E7318082001280B321D2E676F6F676C652E70726F746F6275662E4669656C644F7074696F6E7352076F7074696F6E7312270A0F70726F746F335F6F7074696F6E616C181120012808520E70726F746F334F7074696F6E616C22B6020A0454797065120F0A0B545950455F444F55424C451001120E0A0A545950455F464C4F41541002120E0A0A545950455F494E5436341003120F0A0B545950455F55494E5436341004120E0A0A545950455F494E543332100512100A0C545950455F46495845443634100612100A0C545950455F464958454433321007120D0A09545950455F424F4F4C1008120F0A0B545950455F535452494E471009120E0A0A545950455F47524F5550100A12100A0C545950455F4D455353414745100B120E0A0A545950455F4259544553100C120F0A0B545950455F55494E543332100D120D0A09545950455F454E554D100E12110A0D545950455F5346495845443332100F12110A0D545950455F53464958454436341010120F0A0B545950455F53494E5433321011120F0A0B545950455F53494E543634101222430A054C6162656C12120A0E4C4142454C5F4F5054494F4E414C100112120A0E4C4142454C5F5245515549524544100212120A0E4C4142454C5F5245504541544544100322630A144F6E656F6644657363726970746F7250726F746F12120A046E616D6518012001280952046E616D6512370A076F7074696F6E7318022001280B321D2E676F6F676C652E70726F746F6275662E4F6E656F664F7074696F6E7352076F7074696F6E7322E3020A13456E756D44657363726970746F7250726F746F12120A046E616D6518012001280952046E616D65123F0A0576616C756518022003280B32292E676F6F676C652E70726F746F6275662E456E756D56616C756544657363726970746F7250726F746F520576616C756512360A076F7074696F6E7318032001280B321C2E676F6F676C652E70726F746F6275662E456E756D4F7074696F6E7352076F7074696F6E73125D0A0E72657365727665645F72616E676518042003280B32362E676F6F676C652E70726F746F6275662E456E756D44657363726970746F7250726F746F2E456E756D526573657276656452616E6765520D726573657276656452616E676512230A0D72657365727665645F6E616D65180520032809520C72657365727665644E616D651A3B0A11456E756D526573657276656452616E676512140A0573746172741801200128055205737461727412100A03656E641802200128055203656E642283010A18456E756D56616C756544657363726970746F7250726F746F12120A046E616D6518012001280952046E616D6512160A066E756D62657218022001280552066E756D626572123B0A076F7074696F6E7318032001280B32212E676F6F676C652E70726F746F6275662E456E756D56616C75654F7074696F6E7352076F7074696F6E7322A7010A165365727669636544657363726970746F7250726F746F12120A046E616D6518012001280952046E616D65123E0A066D6574686F6418022003280B32262E676F6F676C652E70726F746F6275662E4D6574686F6444657363726970746F7250726F746F52066D6574686F6412390A076F7074696F6E7318032001280B321F2E676F6F676C652E70726F746F6275662E536572766963654F7074696F6E7352076F7074696F6E732289020A154D6574686F6444657363726970746F7250726F746F12120A046E616D6518012001280952046E616D65121D0A0A696E7075745F747970651802200128095209696E70757454797065121F0A0B6F75747075745F74797065180320012809520A6F75747075745479706512380A076F7074696F6E7318042001280B321E2E676F6F676C652E70726F746F6275662E4D6574686F644F7074696F6E7352076F7074696F6E7312300A10636C69656E745F73747265616D696E671805200128083A0566616C7365520F636C69656E7453747265616D696E6712300A107365727665725F73747265616D696E671806200128083A0566616C7365520F73657276657253747265616D696E672291090A0B46696C654F7074696F6E7312210A0C6A6176615F7061636B616765180120012809520B6A6176615061636B61676512300A146A6176615F6F757465725F636C6173736E616D6518082001280952126A6176614F75746572436C6173736E616D6512350A136A6176615F6D756C7469706C655F66696C6573180A200128083A0566616C736552116A6176614D756C7469706C6546696C657312440A1D6A6176615F67656E65726174655F657175616C735F616E645F686173681814200128084202180152196A61766147656E6572617465457175616C73416E6448617368123A0A166A6176615F737472696E675F636865636B5F75746638181B200128083A0566616C736552136A617661537472696E67436865636B5574663812530A0C6F7074696D697A655F666F7218092001280E32292E676F6F676C652E70726F746F6275662E46696C654F7074696F6E732E4F7074696D697A654D6F64653A055350454544520B6F7074696D697A65466F72121D0A0A676F5F7061636B616765180B200128095209676F5061636B61676512350A1363635F67656E657269635F73657276696365731810200128083A0566616C73655211636347656E65726963536572766963657312390A156A6176615F67656E657269635F73657276696365731811200128083A0566616C736552136A61766147656E65726963536572766963657312350A1370795F67656E657269635F73657276696365731812200128083A0566616C73655211707947656E65726963536572766963657312370A147068705F67656E657269635F7365727669636573182A200128083A0566616C7365521270687047656E65726963536572766963657312250A0A646570726563617465641817200128083A0566616C7365520A64657072656361746564122E0A1063635F656E61626C655F6172656E6173181F200128083A0474727565520E6363456E61626C654172656E6173122A0A116F626A635F636C6173735F707265666978182420012809520F6F626A63436C61737350726566697812290A106373686172705F6E616D657370616365182520012809520F6373686172704E616D65737061636512210A0C73776966745F707265666978182720012809520B737769667450726566697812280A107068705F636C6173735F707265666978182820012809520E706870436C61737350726566697812230A0D7068705F6E616D657370616365182920012809520C7068704E616D65737061636512340A167068705F6D657461646174615F6E616D657370616365182C2001280952147068704D657461646174614E616D65737061636512210A0C727562795F7061636B616765182D20012809520B727562795061636B61676512580A14756E696E7465727072657465645F6F7074696F6E18E7072003280B32242E676F6F676C652E70726F746F6275662E556E696E7465727072657465644F7074696F6E5213756E696E7465727072657465644F7074696F6E223A0A0C4F7074696D697A654D6F646512090A0553504545441001120D0A09434F44455F53495A45100212100A0C4C4954455F52554E54494D4510032A0908E8071080808080024A040826102722E3020A0E4D6573736167654F7074696F6E73123C0A176D6573736167655F7365745F776972655F666F726D61741801200128083A0566616C736552146D65737361676553657457697265466F726D6174124C0A1F6E6F5F7374616E646172645F64657363726970746F725F6163636573736F721802200128083A0566616C7365521C6E6F5374616E6461726444657363726970746F724163636573736F7212250A0A646570726563617465641803200128083A0566616C7365520A64657072656361746564121B0A096D61705F656E74727918072001280852086D6170456E74727912580A14756E696E7465727072657465645F6F7074696F6E18E7072003280B32242E676F6F676C652E70726F746F6275662E556E696E7465727072657465644F7074696F6E5213756E696E7465727072657465644F7074696F6E2A0908E8071080808080024A04080410054A04080510064A04080610074A04080810094A040809100A22E2030A0C4669656C644F7074696F6E7312410A05637479706518012001280E32232E676F6F676C652E70726F746F6275662E4669656C644F7074696F6E732E43547970653A06535452494E475205637479706512160A067061636B656418022001280852067061636B656412470A066A737479706518062001280E32242E676F6F676C652E70726F746F6275662E4669656C644F7074696F6E732E4A53547970653A094A535F4E4F524D414C52066A737479706512190A046C617A791805200128083A0566616C736552046C617A7912250A0A646570726563617465641803200128083A0566616C7365520A6465707265636174656412190A047765616B180A200128083A0566616C736552047765616B12580A14756E696E7465727072657465645F6F7074696F6E18E7072003280B32242E676F6F676C652E70726F746F6275662E556E696E7465727072657465644F7074696F6E5213756E696E7465727072657465644F7074696F6E222F0A054354797065120A0A06535452494E47100012080A04434F5244100112100A0C535452494E475F5049454345100222350A064A5354797065120D0A094A535F4E4F524D414C1000120D0A094A535F535452494E471001120D0A094A535F4E554D42455210022A0908E8071080808080024A040804100522730A0C4F6E656F664F7074696F6E7312580A14756E696E7465727072657465645F6F7074696F6E18E7072003280B32242E676F6F676C652E70726F746F6275662E556E696E7465727072657465644F7074696F6E5213756E696E7465727072657465644F7074696F6E2A0908E80710808080800222C0010A0B456E756D4F7074696F6E73121F0A0B616C6C6F775F616C696173180220012808520A616C6C6F77416C69617312250A0A646570726563617465641803200128083A0566616C7365520A6465707265636174656412580A14756E696E7465727072657465645F6F7074696F6E18E7072003280B32242E676F6F676C652E70726F746F6275662E556E696E7465727072657465644F7074696F6E5213756E696E7465727072657465644F7074696F6E2A0908E8071080808080024A0408051006229E010A10456E756D56616C75654F7074696F6E7312250A0A646570726563617465641801200128083A0566616C7365520A6465707265636174656412580A14756E696E7465727072657465645F6F7074696F6E18E7072003280B32242E676F6F676C652E70726F746F6275662E556E696E7465727072657465644F7074696F6E5213756E696E7465727072657465644F7074696F6E2A0908E807108080808002229C010A0E536572766963654F7074696F6E7312250A0A646570726563617465641821200128083A0566616C7365520A6465707265636174656412580A14756E696E7465727072657465645F6F7074696F6E18E7072003280B32242E676F6F676C652E70726F746F6275662E556E696E7465727072657465644F7074696F6E5213756E696E7465727072657465644F7074696F6E2A0908E80710808080800222E0020A0D4D6574686F644F7074696F6E7312250A0A646570726563617465641821200128083A0566616C7365520A6465707265636174656412710A116964656D706F74656E63795F6C6576656C18222001280E322F2E676F6F676C652E70726F746F6275662E4D6574686F644F7074696F6E732E4964656D706F74656E63794C6576656C3A134944454D504F54454E43595F554E4B4E4F574E52106964656D706F74656E63794C6576656C12580A14756E696E7465727072657465645F6F7074696F6E18E7072003280B32242E676F6F676C652E70726F746F6275662E556E696E7465727072657465644F7074696F6E5213756E696E7465727072657465644F7074696F6E22500A104964656D706F74656E63794C6576656C12170A134944454D504F54454E43595F554E4B4E4F574E100012130A0F4E4F5F534944455F454646454354531001120E0A0A4944454D504F54454E5410022A0908E807108080808002229A030A13556E696E7465727072657465644F7074696F6E12410A046E616D6518022003280B322D2E676F6F676C652E70726F746F6275662E556E696E7465727072657465644F7074696F6E2E4E616D655061727452046E616D6512290A106964656E7469666965725F76616C7565180320012809520F6964656E74696669657256616C7565122C0A12706F7369746976655F696E745F76616C75651804200128045210706F736974697665496E7456616C7565122C0A126E656761746976655F696E745F76616C756518052001280352106E65676174697665496E7456616C756512210A0C646F75626C655F76616C7565180620012801520B646F75626C6556616C756512210A0C737472696E675F76616C756518072001280C520B737472696E6756616C756512270A0F6167677265676174655F76616C7565180820012809520E61676772656761746556616C75651A4A0A084E616D6550617274121B0A096E616D655F7061727418012002280952086E616D655061727412210A0C69735F657874656E73696F6E180220022808520B6973457874656E73696F6E22A7020A0E536F75726365436F6465496E666F12440A086C6F636174696F6E18012003280B32282E676F6F676C652E70726F746F6275662E536F75726365436F6465496E666F2E4C6F636174696F6E52086C6F636174696F6E1ACE010A084C6F636174696F6E12160A04706174681801200328054202100152047061746812160A047370616E1802200328054202100152047370616E12290A106C656164696E675F636F6D6D656E7473180320012809520F6C656164696E67436F6D6D656E7473122B0A11747261696C696E675F636F6D6D656E74731804200128095210747261696C696E67436F6D6D656E7473123A0A196C656164696E675F64657461636865645F636F6D6D656E747318062003280952176C656164696E674465746163686564436F6D6D656E747322D1010A1147656E657261746564436F6465496E666F124D0A0A616E6E6F746174696F6E18012003280B322D2E676F6F676C652E70726F746F6275662E47656E657261746564436F6465496E666F2E416E6E6F746174696F6E520A616E6E6F746174696F6E1A6D0A0A416E6E6F746174696F6E12160A047061746818012003280542021001520470617468121F0A0B736F757263655F66696C65180220012809520A736F7572636546696C6512140A05626567696E1803200128055205626567696E12100A03656E641804200128055203656E64427E0A13636F6D2E676F6F676C652E70726F746F627566421044657363726970746F7250726F746F7348015A2D676F6F676C652E676F6C616E672E6F72672F70726F746F6275662F74797065732F64657363726970746F727062F80101A20203475042AA021A476F6F676C652E50726F746F6275662E5265666C656374696F6E0AB5020A146B6F696E6F732F6F7074696F6E732E70726F746F12066B6F696E6F731A20676F6F676C652F70726F746F6275662F64657363726970746F722E70726F746F2A6D0A0A62797465735F74797065120A0A064241534536341000120A0A06424153453538100112070A034845581002120C0A08424C4F434B5F4944100312120A0E5452414E53414354494F4E5F49441004120F0A0B434F4E54524143545F49441005120B0A074144445245535310063A4C0A056274797065121D2E676F6F676C652E70726F746F6275662E4669656C644F7074696F6E7318D086032001280E32122E6B6F696E6F732E62797465735F7479706552056274797065880101422E5A2C6769746875622E636F6D2F6B6F696E6F732F6B6F696E6F732D70726F746F2D676F6C616E672F6B6F696E6F73620670726F746F330A851A0A1E6B6F696E6F732F70726F746F636F6C2F70726F746F636F6C2E70726F746F120F6B6F696E6F732E70726F746F636F6C1A146B6F696E6F732F6F7074696F6E732E70726F746F2290010A0A6576656E745F64617461121A0A0873657175656E636518012001280D520873657175656E6365121C0A06736F7572636518022001280C420480B518055206736F7572636512120A046E616D6518032001280952046E616D6512120A046461746118042001280C52046461746112200A08696D70616374656418052003280C420480B518065208696D706163746564225E0A14636F6E74726163745F63616C6C5F62756E646C6512250A0B636F6E74726163745F696418012001280C420480B51805520A636F6E74726163744964121F0A0B656E7472795F706F696E7418022001280D520A656E747279506F696E742292010A1273797374656D5F63616C6C5F746172676574121B0A087468756E6B5F696418012001280D480052077468756E6B496412550A1273797374656D5F63616C6C5F62756E646C6518022001280B32252E6B6F696E6F732E70726F746F636F6C2E636F6E74726163745F63616C6C5F62756E646C654800521073797374656D43616C6C42756E646C6542080A0674617267657422B6020A1975706C6F61645F636F6E74726163745F6F7065726174696F6E12250A0B636F6E74726163745F696418012001280C420480B51805520A636F6E74726163744964121A0A0862797465636F646518022001280C520862797465636F646512100A03616269180320012809520361626912380A18617574686F72697A65735F63616C6C5F636F6E74726163741804200128085216617574686F72697A657343616C6C436F6E7472616374124C0A22617574686F72697A65735F7472616E73616374696F6E5F6170706C69636174696F6E1805200128085220617574686F72697A65735472616E73616374696F6E4170706C69636174696F6E123C0A1A617574686F72697A65735F75706C6F61645F636F6E74726163741806200128085218617574686F72697A657355706C6F6164436F6E747261637422750A1763616C6C5F636F6E74726163745F6F7065726174696F6E12250A0B636F6E74726163745F696418012001280C420480B51805520A636F6E74726163744964121F0A0B656E7472795F706F696E7418022001280D520A656E747279506F696E7412120A046172677318032001280C52046172677322710A197365745F73797374656D5F63616C6C5F6F7065726174696F6E12170A0763616C6C5F696418012001280D520663616C6C4964123B0A0674617267657418022001280B32232E6B6F696E6F732E70726F746F636F6C2E73797374656D5F63616C6C5F7461726765745206746172676574226F0A1D7365745F73797374656D5F636F6E74726163745F6F7065726174696F6E12250A0B636F6E74726163745F696418012001280C420480B51805520A636F6E7472616374496412270A0F73797374656D5F636F6E7472616374180220012808520E73797374656D436F6E747261637422F1020A096F7065726174696F6E12550A0F75706C6F61645F636F6E747261637418012001280B322A2E6B6F696E6F732E70726F746F636F6C2E75706C6F61645F636F6E74726163745F6F7065726174696F6E4800520E75706C6F6164436F6E7472616374124F0A0D63616C6C5F636F6E747261637418022001280B32282E6B6F696E6F732E70726F746F636F6C2E63616C6C5F636F6E74726163745F6F7065726174696F6E4800520C63616C6C436F6E747261637412540A0F7365745F73797374656D5F63616C6C18032001280B322A2E6B6F696E6F732E70726F746F636F6C2E7365745F73797374656D5F63616C6C5F6F7065726174696F6E4800520D73657453797374656D43616C6C12600A137365745F73797374656D5F636F6E747261637418042001280B322E2E6B6F696E6F732E70726F746F636F6C2E7365745F73797374656D5F636F6E74726163745F6F7065726174696F6E4800521173657453797374656D436F6E747261637442040A026F7022D0010A127472616E73616374696F6E5F68656164657212190A08636861696E5F696418012001280C5207636861696E4964121D0A0872635F6C696D697418022001280442023001520772634C696D697412140A056E6F6E636518032001280C52056E6F6E636512320A156F7065726174696F6E5F6D65726B6C655F726F6F7418042001280C52136F7065726174696F6E4D65726B6C65526F6F74121A0A05706179657218052001280C420480B5180652057061796572121A0A05706179656518062001280C420480B518065205706179656522BC010A0B7472616E73616374696F6E12140A02696418012001280C420480B5180452026964123B0A0668656164657218022001280B32232E6B6F696E6F732E70726F746F636F6C2E7472616E73616374696F6E5F6865616465725206686561646572123A0A0A6F7065726174696F6E7318032003280B321A2E6B6F696E6F732E70726F746F636F6C2E6F7065726174696F6E520A6F7065726174696F6E73121E0A0A7369676E61747572657318042003280C520A7369676E61747572657322B2030A137472616E73616374696F6E5F7265636569707412140A02696418012001280C420480B5180452026964121A0A05706179657218022001280C420480B518065205706179657212240A0C6D61785F70617965725F726318032001280442023001520A6D617850617965725263121D0A0872635F6C696D697418042001280442023001520772634C696D6974121B0A0772635F75736564180520012804420230015206726355736564122E0A116469736B5F73746F726167655F7573656418062001280442023001520F6469736B53746F726167655573656412380A166E6574776F726B5F62616E6477696474685F757365641807200128044202300152146E6574776F726B42616E6477696474685573656412380A16636F6D707574655F62616E6477696474685F75736564180820012804420230015214636F6D7075746542616E64776964746855736564121A0A0872657665727465641809200128085208726576657274656412330A066576656E7473180A2003280B321B2E6B6F696E6F732E70726F746F636F6C2E6576656E745F6461746152066576656E747312120A046C6F6773180B2003280952046C6F677322B6020A0C626C6F636B5F68656164657212200A0870726576696F757318012001280C420480B51803520870726576696F7573121A0A0668656967687418022001280442023001520668656967687412200A0974696D657374616D7018032001280442023001520974696D657374616D70123B0A1A70726576696F75735F73746174655F6D65726B6C655F726F6F7418042001280C521770726576696F757353746174654D65726B6C65526F6F7412360A177472616E73616374696F6E5F6D65726B6C655F726F6F7418052001280C52157472616E73616374696F6E4D65726B6C65526F6F74121C0A067369676E657218062001280C420480B5180652067369676E657212330A12617070726F7665645F70726F706F73616C7318072003280C420480B518045211617070726F76656450726F706F73616C7322B4010A05626C6F636B12140A02696418012001280C420480B518035202696412350A0668656164657218022001280B321D2E6B6F696E6F732E70726F746F636F6C2E626C6F636B5F686561646572520668656164657212400A0C7472616E73616374696F6E7318032003280B321C2E6B6F696E6F732E70726F746F636F6C2E7472616E73616374696F6E520C7472616E73616374696F6E73121C0A097369676E617475726518042001280C52097369676E617475726522B3030A0D626C6F636B5F7265636569707412140A02696418012001280C420480B5180352026964121A0A06686569676874180220012804420230015206686569676874122E0A116469736B5F73746F726167655F7573656418032001280442023001520F6469736B53746F726167655573656412380A166E6574776F726B5F62616E6477696474685F757365641804200128044202300152146E6574776F726B42616E6477696474685573656412380A16636F6D707574655F62616E6477696474685F75736564180520012804420230015214636F6D7075746542616E64776964746855736564122A0A1173746174655F6D65726B6C655F726F6F7418062001280C520F73746174654D65726B6C65526F6F7412330A066576656E747318072003280B321B2E6B6F696E6F732E70726F746F636F6C2E6576656E745F6461746152066576656E747312570A147472616E73616374696F6E5F726563656970747318082003280B32242E6B6F696E6F732E70726F746F636F6C2E7472616E73616374696F6E5F7265636569707452137472616E73616374696F6E526563656970747312120A046C6F677318092003280952046C6F677342375A356769746875622E636F6D2F6B6F696E6F732F6B6F696E6F732D70726F746F2D676F6C616E672F6B6F696E6F732F70726F746F636F6C620670726F746F33" );
   entry->set_value( protocol_descriptor );
   *entry->mutable_space() = state::space::metadata();

   std::map< std::string, uint64_t > thunk_compute {
      { "apply_block", 16465 },
      { "apply_call_contract_operation", 685 },
      { "apply_set_system_call_operation", 136081 },
      { "apply_set_system_contract_operation", 8692 },
      { "apply_transaction", 12542 },
      { "apply_upload_contract_operation", 3130 },
      { "call", 3573 },
      { "check_authority", 12653 },
      { "check_system_authority", 12750 },
      { "consume_account_rc", 735 },
      { "consume_block_resources", 753 },
      { "deserialize_message_per_byte", 1 },
      { "deserialize_multihash_base", 102 },
      { "deserialize_multihash_per_byte", 404 },
      { "event", 1222 },
      { "event_per_impacted", 101 },
      { "exit", 11636 },
      { "get_account_nonce", 821 },
      { "get_account_rc", 1072 },
      { "get_arguments", 809 },
      { "get_block", 1134 },
      { "get_block_field", 1417 },
      { "get_caller", 825 },
      { "get_chain_id", 1046 },
      { "get_contract_id", 778 },
      { "get_head_info", 2099 },
      { "get_last_irreversible_block", 772 },
      { "get_next_object", 11181 },
      { "get_object", 1054 },
      { "get_operation", 1081 },
      { "get_prev_object", 15445 },
      { "get_resource_limits", 1227 },
      { "get_transaction", 1584 },
      { "get_transaction_field", 1530 },
      { "hash", 1570 },
      { "keccak_256_base", 1406 },
      { "keccak_256_per_byte", 1 },
      { "log", 738 },
      { "object_serialization_per_byte", 1 },
      { "post_block_callback", 741 },
      { "post_transaction_callback", 721 },
      { "pre_block_callback", 730 },
      { "pre_transaction_callback", 729 },
      { "process_block_signature", 4499 },
      { "put_object", 1057 },
      { "recover_public_key", 29630 },
      { "remove_object", 893 },
      { "ripemd_160_base", 1343 },
      { "ripemd_160_per_byte", 1 },
      { "set_account_nonce", 749 },
      { "sha1_base", 1151 },
      { "sha1_per_byte", 1 },
      { "sha2_256_base", 1385 },
      { "sha2_256_per_byte", 1 },
      { "sha2_512_base", 1445 },
      { "sha2_512_per_byte", 1 },
      { "verify_account_nonce", 822 },
      { "verify_merkle_root", 1 },
      { "verify_signature", 762 },
      { "verify_vrf_proof", 144067 },
   };

   chain::compute_bandwidth_registry cbr;

   for ( const auto& [ key, value ] : thunk_compute )
   {
      auto centry = cbr.add_entries();
      centry->set_name( key );
      centry->set_compute( value );
   }

   entry = gdata.add_entries();
   entry->set_key( state::key::compute_bandwidth_registry );
   entry->set_value( util::converter::as< std::string >( cbr ) );
   *entry->mutable_space() = state::space::metadata();

   entry = gdata.add_entries();
   entry->set_key( state::key::block_hash_code );
   entry->set_value( util::converter::as< std::string >( unsigned_varint{ std::underlying_type_t< crypto::multicodec >( crypto::multicodec::sha2_256 ) } ) );
   *entry->mutable_space() = state::space::metadata();

   return gdata;
}
