#include <sstream>

#include "distsn.h"


using namespace std;


Blacklist::Blacklist ()
{
	map <User, Profile> profiles = read_profiles ();
	for (auto i: profiles) {
		User user = i.first;
		Profile profile = i.second;
		unsigned int celebrityness = profile.number_of_followers;
		socialnet::eImplementation implementation = profile.implementation;
		if (users_to_celebrityness.find (user) == users_to_celebrityness.end ()) {
			users_to_celebrityness.insert (pair <User, unsigned int> {user, celebrityness});
		}
		if (users_to_implementation.find (user) == users_to_implementation.end ()) {
			users_to_implementation.insert (pair <User, socialnet::eImplementation> {user, implementation});
		}
	}

	const string filename {"/etc/vinayaka/blacklisted_users.csv"};
	FILE *in = fopen (filename.c_str (), "r");
	if (in == nullptr) {
		cerr << "File " << filename << " not found." << endl;
	} else {
		set <User> users;
		vector <vector <string>> table = parse_csv (in);
		blacklist_table = table;
	}
}


bool Blacklist::operator () (string a_host_name, string a_user_name) const
{
	User user {a_host_name, a_user_name};
	unsigned int celebrityness = 0;
	if (users_to_celebrityness.find (user) != users_to_celebrityness.end ()) {
		celebrityness = users_to_celebrityness.at (user);
	}
	socialnet::eImplementation implementation = socialnet::eImplementation::UNKNOWN;
	if (users_to_implementation.find (user) != users_to_implementation.end ()) {
		implementation = users_to_implementation.at (user);
	}

	for (vector <string> row: blacklist_table) {
		if (4 <= row.size ()) {
			string implementation_field_string = row.at (0);
			socialnet::eImplementation implementation_field;
			socialnet::decode (implementation_field_string, implementation_field);
			bool match_implementation = (
				implementation_field_string == string {"*"}
				|| implementation_field == implementation
			);

			string host_name_field = row.at (1);
			bool match_host_name = (
				host_name_field == string {"*"}
				|| host_name_field == a_host_name
			);

			string user_name_field = row.at (2);
			bool match_user_name = (
				user_name_field == string {"*"}
				|| user_name_field == a_user_name
			);

			string celebrityness_threshold_string = row.at (3);
			stringstream celebrityness_threshold_stream {celebrityness_threshold_string};
			unsigned int celebrityness_threshold_number = 0;
			celebrityness_threshold_stream >> celebrityness_threshold_number;
			bool match_celebrityness = (
				celebrityness_threshold_string == string {"*"}
				|| celebrityness_threshold_number <= celebrityness
			);

			if (
				match_implementation
				&& match_host_name
				&& match_user_name
				&& match_celebrityness
			) {
				return true;
			}			
		} else if (2 <= row.size ()) {
			string host_name_field = row.at (0);
			bool match_host_name = (
				host_name_field == string {"*"}
				|| host_name_field == a_host_name
			);

			string user_name_field = row.at (1);
			bool match_user_name = (
				user_name_field == string {"*"}
				|| user_name_field == a_user_name
			);

			if (
				match_host_name
				&& match_user_name
			) {
				return true;
			}			
		}
	}

	return false;
}

