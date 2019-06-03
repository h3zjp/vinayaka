#include "distsn.h"


using namespace std;


Blacklist::Blacklist ()
{
	const string filename {"/etc/vinayaka/blacklisted_users.csv"};
	FILE *in = fopen (filename.c_str (), "r");
	if (in == nullptr) {
		cerr << "File " << filename << " not found." << endl;
	} else {
		set <User> users;
		vector <vector <string>> table = parse_csv (in);
		for (auto row: table) {
			if (1 < row.size ()) {
				string host = row.at (0);
				string user = row.at (1);
				users.insert (User {host, user});
			}
		}
		blacklisted_users = users;
	}
}


bool Blacklist::operator () (string host_name, string user_name) const
{
	return blacklisted_users.find (User {host_name, user_name}) != blacklisted_users.end ()
		|| blacklisted_users.find (User {host_name, string ("*")}) != blacklisted_users.end ();
}

