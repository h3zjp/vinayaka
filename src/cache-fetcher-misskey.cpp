#include <iostream>
#include <ctime>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <socialnet-1.h>

#include "picojson.h"
#include "distsn.h"


using namespace std;


static string get_filtered_api (
	string in,
	string listener_host,
	string listener_user,
	unsigned int limit,
	unsigned int offset
) {

	picojson::value json_value;
	string json_parse_error = picojson::parse (json_value, in);
	if (! json_parse_error.empty ()) {
		cerr << json_parse_error << endl;
		exit (1);
	}

	auto users_array = json_value.get <picojson::array> ();

	vector <string> filtered_formats_raw;

	for (unsigned int cn = 0; cn < users_array.size (); cn ++) {
		auto user_value = users_array.at (cn);
		auto user_object = user_value.get <picojson::object> ();
		string host = user_object.at (string {"host"}).get <string> ();
		string user = user_object.at (string {"user"}).get <string> ();
		string screen_name = user_object.at (string {"screen_name"}).get <string> ();
		string bio = user_object.at (string {"bio"}).get <string> ();
		string avatar = user_object.at (string {"avatar"}).get <string> ();
		bool local = (host == listener_host);
		string type = user_object.at (string {"type"}).get <string> ();
		bool bot = (type == string {"Service"});
		string url = user_object.at (string {"url"}).get <string> ();
		bool optout = (host == string {"3.distsn.org"} && user == string {"optout"});

		bool following_bool
			= user_object.find (string {"following"}) != user_object.end ()
			&& user_object.at (string {"following"}).get <bool> ();

		if (optout || ((! local) && (! following_bool) && (! bot))) {
			stringstream out_user;
			out_user
				<< "{"
				<< "\"name\":\"" << escape_json (screen_name) << "\","
				<< "\"username\":\"" << escape_json (user) << "\","
				<< "\"avatarUrl\":\"" << escape_json (avatar) << "\","
				<< "\"description\":\"" << escape_json (bio) << "\","
				<< "\"host\":\"" << escape_json (host) << "\""
				<< "}";
			filtered_formats_raw.push_back (out_user.str ());
		}
	}

	vector <string> filtered_formats;
	for (unsigned int cn = 0; cn < limit && cn + offset < filtered_formats_raw.size (); cn ++) {
		string format = filtered_formats_raw.at (cn + offset);
		filtered_formats.push_back (format);
	}

	string out;
	out += string {"["};
	for (unsigned int cn = 0; cn < filtered_formats.size (); cn ++) {
		if (0 < cn) {
			out += string {","};
		}
		out += filtered_formats.at (cn);
	}
	out += string {"]"};
	return out;
}


static void full (string host, string user, unsigned int limit, unsigned int offset, char *argv [])
{
	bool hit;
	string result = fetch_cache (host, user, hit);
	if (hit) {
		cout << "Access-Control-Allow-Origin: *" << endl;
		cout << "Content-Type: application/json" << endl << endl;
		cout << get_filtered_api (result, host, user, limit, offset);
	} else {
		pid_t pid = fork ();
		if (pid == 0) {
			execv ("/usr/local/bin/vinayaka-user-match-impl", argv);
		} else {
			int status;
			waitpid (pid, &status, 0);
			string result_2 = fetch_cache (host, user, hit);
			cout << "Access-Control-Allow-Origin: *" << endl;
			cout << "Content-Type: application/json" << endl << endl;
			cout << get_filtered_api (result_2, host, user, limit, offset);
		}
	}
}


static void fallback (unsigned int limit, unsigned int offset)
{
	string s;
	{
		string file_name {"/var/lib/vinayaka/users-new-cache.json"};
		FileLock lock {file_name, LOCK_SH};
		FILE *in = fopen (file_name.c_str (), "r");
		if (in == nullptr) {
			cerr << file_name << " can not open." << endl;
			exit (1);
		}
		for (; ; ) {
		char b [1024];
			auto fgets_return = fgets (b, 1024, in);
			if (fgets_return == nullptr) {
				break;
			}
			s += string {b};
		}
	}

	cout << "Access-Control-Allow-Origin: *" << endl;
	cout << "Content-Type: application/json" << endl << endl;
	cout << get_filtered_api (s, string {}, string {}, limit, offset);
}


int main (int argc, char *argv [])
{
	string host {argv [1]};
	string user {argv [2]};

	unsigned int limit = 64;
	if (3 < argc) {
		stringstream limit_stream {argv [3]};
		limit_stream >> limit;
	}

	unsigned int offset = 0;
	if (4 < argc) {
		stringstream offset_stream {argv [4]};
		offset_stream >> offset;
	}

	auto http = make_shared <socialnet::Http> ();
	http->user_agent = user_agent;
	if (is_sect (host, * http)) {
		full (host, user, limit, offset, argv);
	} else {
		fallback (limit, offset);
	}
}


