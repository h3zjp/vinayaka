#ifndef DISTSN_H
#define DISTSN_H


#include <curl/curl.h>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <sys/file.h>
#include <unistd.h>

#include <tinyxml2.h>

#include <socialnet-1.h>

#include "picojson.h"


extern const std::string user_agent;


class ExceptionWithLineNumber: public std::exception {
public:
	unsigned int line;
public:
	ExceptionWithLineNumber () {
		line = 0;
	};
	ExceptionWithLineNumber (unsigned int a_line) {
		line = a_line;
	};
};


class ModelException: public ExceptionWithLineNumber {
public:
	ModelException () { };
	ModelException (unsigned int a_line): ExceptionWithLineNumber (a_line) { };
};


class ParseException: public ExceptionWithLineNumber {
public:
	ParseException () { };
	ParseException (unsigned int a_line): ExceptionWithLineNumber (a_line) { };
};


class UserAndWords {
public:
	std::string user;
	std::string host;
	std::vector <std::string> words;
};


class UserAndSpeed {
public:
	std::string host;
	std::string username;
	double speed;
public:
	UserAndSpeed (std::string a_host, std::string a_username, double a_speed) {
		host = a_host;
		username = a_username;
		speed = a_speed;
	};
};


class User {
public:
	std::string host;
	std::string user;
public:
	User () { /* Do nothing. */ };
	User (std::string a_host, std::string a_user) {
		host = a_host;
		user = a_user;
	};
	bool operator < (const User &r) const {
		return host < r.host || (host == r.host && user < r.user);
	};
};


class Profile {
public:
	std::string screen_name;
	std::string bio;
	std::string avatar;
	std::string type;
	std::string url;
	socialnet::eImplementation implementation;
	std::string activitypub_id;
	unsigned int number_of_followers;
	bool explicitly_discoverable;
	bool implicitly_discoverable;
};


class FileLock {
public:
	int fd;
	std::string path;
public:
	FileLock (std::string a_path, int operation = LOCK_EX);
	~FileLock ();
};


class Blacklist {
public:
	std::map <User, unsigned int> users_to_celebrityness;
	std::map <User, socialnet::eImplementation> users_to_implementation;
	std::vector <std::vector <std::string>> blacklist_table;
public:
	Blacklist ();
	bool operator () (std::string host_name, std::string user_name) const;
	bool operator () (
		socialnet::eImplementation implementation,
		std::string host_name,
		std::string user_name,
		unsigned int celebrityness
	) const;
};


class Optout {
public:
	std::map <User, std::string> users_to_bio;
	std::map <User, bool> users_to_implicitly_discoverable;
public:
	Optout ();
	bool operator () (std::string host_name, std::string user_name) const;
	bool operator () (
		std::string bio,
		bool implicitly_discoverable
	) const;
};


std::string escape_json (std::string in);
std::vector <std::string> get_words_from_toots
	(std::vector <std::string> toots,
	unsigned int word_length,
	unsigned int vocabulary_size);
std::vector <std::string> get_words_from_toots
	(std::vector <std::string> toots,
	unsigned int word_length,
	unsigned int vocabulary_size,
	std::map <std::string, unsigned int> word_to_popularity,
	unsigned int minimum_popularity);
double get_rarity (unsigned int occupancy);

std::vector <UserAndWords> read_storage (std::string filename);

std::vector <std::vector <std::string>> parse_csv (FILE *in);
std::string escape_csv (std::string in);
std::string escape_utf8_fragment (std::string in);

std::map <User, Profile> read_profiles ();
bool safe_url (std::string url);
void add_to_cache (std::string host, std::string user, std::string result);
std::string fetch_cache (std::string a_host, std::string a_user, bool & a_hit);

bool described (std::string screen_name, std::string bio, std::string avatar);
bool good_for_suggestion (const picojson::object a_user_object, std::string a_listener_host_name);

/* sort-user-speed.cpp */
std::vector <UserAndSpeed> get_users_and_speed ();
std::vector <UserAndSpeed> get_users_and_speed_impl (double limit);


#endif /* #ifndef DISTSN_H */

