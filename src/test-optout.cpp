#include <iostream>

#include "distsn.h"


using namespace std;


int main ()
{
	auto optouted_users = get_optouted_users ();
	for (auto i: optouted_users) {
		string host_name = i.host;
		string user_name = i.user;
		cout << host_name << " " << user_name << endl;
	}
}

