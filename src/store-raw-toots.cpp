#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include <set>
#include <random>
#include <algorithm>

#include <socialnet-1.h>

#include "picojson.h"
#include "distsn.h"


using namespace std;


static vector <User> get_users ()
{
	vector <UserAndSpeed> users_and_speed = get_users_and_speed ();
	vector <User> users;
	for (unsigned int cn = 0; cn < users_and_speed.size () && cn < 30000; cn ++) {
		UserAndSpeed user_and_speed = users_and_speed.at (cn);
		User user {user_and_speed.host, user_and_speed.username};
		users.push_back (user);
	}
	return users;
}


static void write_to_storage (vector <pair <User, vector <string>>> users_and_toots, ofstream &out)
{
	for (auto user_and_toots: users_and_toots) {
		auto user = user_and_toots.first;
		auto toots = user_and_toots.second;
		for (auto toot: toots) {
			out
				<< "\"" << escape_csv (user.host) << "\"," 
				<< "\"" << escape_csv (user.user) << "\"," 
				<< "\"" << escape_csv (toot) << "\""
				<< endl;
		}
	}
}


static const unsigned int history_variations = 64;

	
static void get_and_save_toots (vector <User> users)
{
	auto http = make_shared <socialnet::Http> ();
	http->user_agent = user_agent;

	socialnet::HostPool host_pool;
	host_pool.http = http;

	vector <pair <User, vector <string>>> users_and_toots;

	for (auto user: users) {
		try {
			cerr << user.user << "@" << user.host << endl;
			
			auto socialnet_user = host_pool.make_user (user.host, user.user);
			if (! socialnet_user) {
				throw (socialnet::UserException {__LINE__});
			}
			
			string screen_name;
			string bio;
			string avatar;
			string type;
			string activitypub_id;
			bool explicitly_discoverable;
			bool implicitly_discoverable;
			socialnet_user->get_profile (
				screen_name,
				bio,
				avatar,
				type,
				activitypub_id,
				explicitly_discoverable,
				implicitly_discoverable
			);

			unsigned int page = 1;
			auto socialnet_statuses =socialnet_user->get_timeline (page);

			vector <string> toots;
			toots.push_back (screen_name);
			toots.push_back (bio);

			for (auto socialnet_status: socialnet_statuses) {
				string short_toot = socialnet_status.content.substr (0, 5000);
				toots.push_back (short_toot);
			}
			users_and_toots.push_back (pair <User, vector <string>> {user, toots});
		} catch (socialnet::ExceptionWithLineNumber e) {
			cerr << "Error " << user.user << "@" << user.host << " " << e.line << endl;
		}
	}
	
	random_device device;
	auto random_engine = default_random_engine (device ());
	uniform_int_distribution <unsigned int> distribution (0, history_variations - 1);
	unsigned int random_number = distribution (random_engine);
	
	stringstream filename;
	filename << "/var/lib/vinayaka/raw-toots/" << random_number << ".csv";
	ofstream out {filename.str ()};
	write_to_storage (users_and_toots, out);
}


int main (int argc, char **argv)
{
	auto users = get_users ();
	sort (users.begin (), users.end ());
	get_and_save_toots (users);
}


