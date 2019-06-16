#include <iostream>
#include <limits>
#include <unistd.h>
#include <sys/wait.h>

#include <languagemodel-1.h>

#include "distsn.h"


using namespace std;


static vector <User> get_users (unsigned int size)
{
	vector <UserAndSpeed> users_and_speed = get_users_and_speed ();
	vector <User> users;
	for (unsigned int cn = 0; cn < users_and_speed.size () && cn < size; cn ++) {
		UserAndSpeed user_and_speed = users_and_speed.at (cn);
		User user {user_and_speed.host, user_and_speed.username};
		users.push_back (user);
	}
	return users;
}


int main (int argc, char **argv)
{
	auto all_users = get_users (numeric_limits <unsigned int>::max ());
	map <User, Profile> profiles = read_profiles ();

	const unsigned int capacity = 480;
	const unsigned int celebrityness_threshold = 200;

	vector <User> influencers;

	for (auto user: all_users) {
		if (capacity <= influencers.size ()) {
			break;
		}
		if (
			profiles.find (user) != profiles.end ()
			&& profiles.at (user).type != string {"Service"}
			&& celebrityness_threshold <= profiles.at (user).number_of_followers
		) {
			influencers.push_back (user);
		}
	}

	auto prefetch_users = influencers;

	for (unsigned int cn = 0; cn < prefetch_users.size (); cn ++) {
		auto user = prefetch_users.at (cn);
		cerr << cn << " " << user.host << " " << user.user << endl;

		auto pid = fork ();
		if (pid < 0) {
			/* Error */
			/* Do nothing */
		} else if (pid == 0) {
			/* Child */
			string path {"/usr/local/bin/vinayaka-user-match-impl"};
			auto result = execl (path.c_str (), path.c_str (), user.host.c_str (), user.user.c_str (), nullptr);
			if (result < 0) {
				cerr << "Can not exec: " << errno << endl;
			}
		} else {
			/* Parent */
			int status = 0;
			wait (& status);
		}
		
		cerr << endl << endl;
	}
}


