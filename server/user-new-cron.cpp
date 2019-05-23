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


static const unsigned int limit = 3 * 24 * 60 * 60;


class UserAndBirthday {
public:
	string host;
	string user;
	time_t birthday;
	bool blacklisted;
	string screen_name;
	string bio;
	string avatar;
	string type;
	string url;
	socialnet::eImplementation implementation;
	string activitypub_id;
public:
	UserAndBirthday () {};
	UserAndBirthday
		(string a_host, string a_user, time_t a_birthday):
		host (a_host),
		user (a_user),
		birthday (a_birthday),
		blacklisted (false),
		implementation (socialnet::eImplementation::UNKNOWN),
		activitypub_id (a_user)
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


static vector <UserAndBirthday> for_host (shared_ptr <socialnet::Host> socialnet_host)
{
	map <User, UserAndBirthday> users_to_birthday;

	auto toots = socialnet_host->get_local_timeline (limit);

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

	time_t now = time (nullptr);

	vector <UserAndBirthday> users_and_birthdays;
	for (auto user_to_birthday: users_to_birthday) {
		auto user_and_birthday = user_to_birthday.second;
		auto birthday = user_and_birthday.birthday;
		if (birthday < now && now - birthday < limit) {
			users_and_birthdays.push_back (user_to_birthday.second);
		}
	}
	return users_and_birthdays;
}


static bool by_timestamp (const UserAndBirthday &a, const UserAndBirthday &b)
{
	return b.birthday < a.birthday;
}


static bool by_host_name (const UserAndBirthday &a, const UserAndBirthday &b)
{
	return tuple <string, string> {a.host, a.user} < tuple <string, string> {b.host, b.user};
}


static vector <UserAndBirthday> get_users_in_all_hosts ()
{
	vector <UserAndBirthday> users_in_all_hosts;
	auto hosts = socialnet::get_hosts ();
	unsigned int cn = 0;
	for (auto host: hosts) {
		cerr << (cn + 1) << "/" << hosts.size () << " " << host->host_name << endl;
		try {
			auto users_in_host = for_host (host);
			for (auto user: users_in_host) {
				users_in_all_hosts.push_back (user);
			}
		} catch (socialnet::ExceptionWithLineNumber e) {
			cerr << "ERROR: " << e.line << " " << host->host_name << endl;
		}
		cn ++;
	}

	set <User> blacklisted_users = get_blacklisted_users ();
	for (auto & user: users_in_all_hosts) {
		if (blacklisted_users.find (User {user.host, user.user}) != blacklisted_users.end ()
			|| blacklisted_users.find (User {user.host, string {"*"}}) != blacklisted_users.end ())
		{
			user.blacklisted = true;
		}
	}

	return users_in_all_hosts;
}


static void get_profile_for_all_users (vector <UserAndBirthday> &users_and_birthday)
{
	auto http = make_shared <socialnet::Http> ();

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

		try {
			cerr << user << "@" << host << endl;
			auto socialnet_user = socialnet::make_user (host, user, http);
			if (! socialnet_user) {
				throw (socialnet::UserException {__LINE__});
			}
			url = socialnet_user->url ();
			implementation = socialnet_user->host->implementation ();
			socialnet_user->get_profile (screen_name, bio, avatar, type);
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
	}
}


static void cache_sorted_result ()
{
	vector <UserAndBirthday> newcomers = get_users_in_all_hosts ();

	sort (newcomers.begin (), newcomers.end (), by_host_name);

	get_profile_for_all_users (newcomers);

	sort (newcomers.begin (), newcomers.end (), by_timestamp);

	const string filename {"/var/lib/vinayaka/users-new-cache.json"};
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
			<< "\"blacklisted\":" << (user.blacklisted? "true": "false") << ","
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
	cache_sorted_result ();
	return 0;
}

