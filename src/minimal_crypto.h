#include <crypto++/cryptlib.h>
#include <crypto++/sha.h>
#include <crypto++/filters.h>
#include <crypto++/hex.h>
#include <crypto++/channels.h>
#include <string>


namespace minimal_crypto {


using namespace std;
using namespace CryptoPP;


string hash (string a_secret_code)
{
	string secret_code {a_secret_code};
	string public_code;
	SHA256 sha256;
	HashFilter hash_filter (sha256, new HexEncoder (new StringSink (public_code)));
	ChannelSwitch channel_switch;
	channel_switch.AddDefaultRoute (hash_filter);
	StringSource string_source (secret_code, true /* pumpAll */, new Redirector (channel_switch));
	return public_code;
}


} /* namespace minimal_hash { */

