#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include <set>
#include <tuple>

#include <socialnet-1.h>

#include "picojson.h"
#include "distsn.h"


using namespace std;


static const unsigned int limit = 7 * 24 * 60 * 60;
static const unsigned int interval = 6 * 60 * 60;
static const string filename_store {"/var/lib/vinayaka/users-new-store.json"};
static const string filename_cache {"/var/lib/vinayaka/users-new-cache.json"};


class UserAndBirthday {
public:
	string host;
	string user;
	time_t birthday;
	string screen_name;
	string bio;
	string avatar;
	string type;
	string url;
	socialnet::eImplementation implementation;
	string activitypub_id;
	unsigned int celebrityness;
public:
	UserAndBirthday () {};
	UserAndBirthday
		(string a_host, string a_user, time_t a_birthday):
		host (a_host),
		user (a_user),
		birthday (a_birthday),
		implementation (socialnet::eImplementation::UNKNOWN),
		activitypub_id (a_user),
		celebrityness (0)
		{};
};


static bool valid_character (char c)
{
	return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || ('0' <= c && c <= '9') || (c == '_');
}


static bool valid_username (string s)
{
	for (auto c: s) {
		if (! valid_character (c)) {
			return false;
		}
	}
	return true;
}


static bool by_timestamp (const UserAndBirthday &a, const UserAndBirthday &b)
{
	return b.birthday < a.birthday;
}


static bool by_host_name (const UserAndBirthday &a, const UserAndBirthday &b)
{
	return tuple <string, string> {a.host, a.user} < tuple <string, string> {b.host, b.user};
}


static void read_storage (string filename, vector <UserAndBirthday> & a_users_and_birthday)
{
	a_users_and_birthday.clear ();

	FileLock {filename, LOCK_SH};
	FILE *in = fopen (filename.c_str (), "r");
	if (in == nullptr) {
		cerr << "File " << filename << " can not open." << endl;
		exit (1);
	}

	string s;
	for (; ; ) {
		if (feof (in)) {
			break;
		}
		char b [1024];
		fgets (b, 1024, in);
		s += string {b};
	}
	picojson::value json_value;
	string parse_error = picojson::parse (json_value, s);
	
	if (! parse_error.empty ()) {
		cerr << parse_error << endl;
		return;
	}
	
	auto json_array = json_value.get <picojson::array> ();

	vector <UserAndBirthday> users_and_first_toots;

	for (auto user_value: json_array) {
		auto user_object = user_value.get <picojson::object> ();
		string host = user_object.at (string {"host"}).get <string> ();
		string user = user_object.at (string {"user"}).get <string> ();
		string first_toot_timestamp_string = user_object.at (string {"first_toot_timestamp"}).get <string> ();
		time_t first_toot_timestamp;
		stringstream {first_toot_timestamp_string} >> first_toot_timestamp;
		users_and_first_toots.push_back (UserAndBirthday {host, user, first_toot_timestamp});
	}

	a_users_and_birthday = users_and_first_toots;
}


static vector <UserAndBirthday> get_users_in_all_hosts (vector <UserAndBirthday> known_users)
{
	auto http = make_shared <socialnet::Http> ();
	http->user_agent = user_agent;
	auto hosts = socialnet::get_hosts (http);

	map <User, UserAndBirthday> users_to_birthday;

	for (auto user_and_birthday: known_users) {
		User user {user_and_birthday.host, user_and_birthday.user};
		if (users_to_birthday.find (user) == users_to_birthday.end ()) {
			users_to_birthday.insert (pair <User, UserAndBirthday> {user, user_and_birthday});
		}
	}

	unsigned int cn = 0;
	for (auto host: hosts) {
		cerr << (cn + 1) << "/" << hosts.size () << " " << host->host_name << endl;
		try {
			auto toots = host->get_local_timeline (interval);

			for (auto toot: toots) {
				if (valid_username (toot.user_name)) {
					User user {toot.host_name, toot.user_name};
					UserAndBirthday user_and_birthday {toot.host_name, toot.user_name, toot.user_timestamp};
					if (users_to_birthday.find (user) == users_to_birthday.end ()) {
						users_to_birthday.insert (pair <User, UserAndBirthday> {user, user_and_birthday});
					} else {
						if (user_and_birthday.birthday < users_to_birthday.at (user).birthday) {
							users_to_birthday.at (user) = user_and_birthday;
						}
					}
				}
			}
		} catch (socialnet::ExceptionWithLineNumber e) {
			cerr << "ERROR: " << e.line << " " << host->host_name << endl;
		}
		cn ++;
	}

	vector <UserAndBirthday> users;

	for (auto user_to_birthday: users_to_birthday) {
		users.push_back (user_to_birthday.second);
	}

	time_t now = time (nullptr);
	vector <UserAndBirthday> young_users;

	for (auto user_and_birthday: users) {
		auto birthday = user_and_birthday.birthday;
		if (birthday < now && now - birthday < limit) {
			young_users.push_back (user_and_birthday);
		}
	}

	return young_users;
}


static void get_profile_for_all_users (vector <UserAndBirthday> &users_and_birthday)
{
	auto http = make_shared <socialnet::Http> ();
	http->user_agent = user_agent;

	sort (users_and_birthday.begin (), users_and_birthday.end (), by_host_name);

	for (auto &user_and_birthday: users_and_birthday) {
		string host = user_and_birthday.host;
		string user = user_and_birthday.user;
		string screen_name;
		string bio;
		string avatar;
		string type;
		string url = string {"https://"} + host + string {"/users/"} + user;
		auto implementation = socialnet::eImplementation::UNKNOWN;
		string activitypub_id = user;
		unsigned int celebrityness = 0;

		try {
			cerr << user << "@" << host << endl;
			auto socialnet_user = socialnet::make_user (host, user, http);
			if (! socialnet_user) {
				throw (socialnet::UserException {__LINE__});
			}
			url = socialnet_user->url ();
			implementation = socialnet_user->host->implementation ();
			socialnet_user->get_profile (screen_name, bio, avatar, type);
			celebrityness = socialnet_user->get_number_of_followers ();
		} catch (socialnet::ExceptionWithLineNumber e) {
			cerr << e.line << endl;
		}

		user_and_birthday.screen_name = screen_name;
		user_and_birthday.bio = bio;
		user_and_birthday.avatar = avatar;
		user_and_birthday.type = type;
		user_and_birthday.url = url;
		user_and_birthday.implementation = implementation;
		user_and_birthday.activitypub_id = activitypub_id;
		user_and_birthday.celebrityness = celebrityness;
	}
}


static void cache_sorted_result (vector <UserAndBirthday> newcomers, string filename)
{
	sort (newcomers.begin (), newcomers.end (), by_timestamp);

	FileLock {filename};
	ofstream out {filename};

	out << "[";
	for (unsigned int cn = 0; cn < newcomers.size (); cn ++) {
		if (0 < cn) {
			out << "," << endl;
		}
		auto user = newcomers.at (cn);
		out
			<< "{"
			<< "\"host\":\"" << escape_json (user.host) << "\","
			<< "\"user\":\"" << escape_json (user.user) << "\","
			<< "\"first_toot_timestamp\":\"" << user.birthday << "\",";
		if (safe_url (user.url)) {
			out << "\"url\":\"" << user.url << "\",";
		} else {
			out << "\"url\":\"\",";
		}
		out
			<< "\"screen_name\":\"" << escape_json (user.screen_name) << "\","
			<< "\"bio\":\"" << escape_json (user.bio) << "\",";
		if (safe_url (user.avatar)) {
			out << "\"avatar\":\"" << escape_json (user.avatar) << "\",";
		} else {
			out << "\"avatar\":\"\",";
		}
		out << "\"type\":\"" << escape_json (user.type) << "\",";
		out << "\"implementation\":\"" << socialnet::format (user.implementation) << "\",";
		out << "\"activitypub_id\":\"" << escape_json (user.activitypub_id) << "\"";
		out
			<< "}";
	}
	out << "]";
}


int main (int argc, char **argv)
{
	vector <UserAndBirthday> known_users;
	read_storage (filename_store, known_users);
	vector <UserAndBirthday> newcomers = get_users_in_all_hosts (known_users);
	cache_sorted_result (newcomers, filename_store);

	get_profile_for_all_users (newcomers);

	Blacklist blacklist;
	vector <UserAndBirthday> public_newcomers;
	
	for (auto i: newcomers) {
		if (
			(! blacklist (i.implementation, i.host, i.user, i.celebrityness))
			&& (! optouted (i.bio))
		) {
			public_newcomers.push_back (i);
		}
	}

	cache_sorted_result (public_newcomers, filename_cache);
	return 0;
}

