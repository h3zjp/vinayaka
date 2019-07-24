#include <iostream>
#include <ctime>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <socialnet-1.h>

#include "distsn.h"
#include "picojson.h"


using namespace std;


static string get_filtered_api ()
{
	socialnet::Newcomers newcomers;
	socialnet::Http http;
	http.user_agent = user_agent;
	newcomers.receive (http);
	auto well_explained_users = newcomers.get_well_explained_users ();

	vector <string> filtered_formats;

	for (auto user: well_explained_users) {
		stringstream out_user;
		out_user
			<< "{"
			<< "\"id\":\"" << escape_json (string {"0"}) << "\","
			<< "\"username\":\"" << escape_json (user.user_name) << "\","
			<< "\"acct\":\"" << escape_json (user.user_name + string {"@"} + user.host_name) << "\","
			<< "\"display_name\":\"" << escape_json (user.screen_name) << "\","
			<< "\"bot\":false,"
			<< "\"note\":\"" << escape_json (user.bio) << "\","
			<< "\"url\":\"" << escape_json (user.url) << "\","
			<< "\"avatar\":\"" << escape_json (user.avatar) << "\","
			<< "\"avatar_static\":\"" << escape_json (user.avatar) << "\","
			<< "\"followers_count\":0,"
			<< "\"following_count\":0,"
			<< "\"statuses_count\":0"
			<< "}";
		filtered_formats.push_back (out_user.str ());
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


int main (int argc, char *argv [])
{
	cout << "Access-Control-Allow-Origin: *" << endl;
	cout << "Content-Type: application/json" << endl << endl;
	cout << get_filtered_api ();
}

