.PHONY: all clean install uninstall initialize refresh-cache refresh-all activate-cgi

srcdir = .

CFLAGS = -std=c++14 -Wall -Wpointer-arith -O2 -fno-builtin -g

VPATH = $(srcdir)

SHELL = /bin/sh
RM = /bin/rm -vf
INSTALL = install
INSTALL_PROGRAM = $(INSTALL) -m a=rx
INSTALL_DATA = $(INSTALL) -m a=r
MKDIR = mkdir
RMDIR = rmdir
CHMOD = chmod
TOUCH = touch

cgidir = /usr/lib/cgi-bin
bindir = /usr/local/bin
etcdir = /etc/vinayaka

objects = \
	cache-fetcher.o \
	cache-fetcher-osa.o \
	cache-fetcher-filtered.o \
	cache-fetcher-suggestions.o \
	distsn.o \
	sort-user-speed.o \
	user-avatar-cron.o \
	user-match-api.o \
	user-speed-api.o \
	user-speed-cron.o \
	store-raw-toots.o \
	collect-raw-toots.o \
	user-search-api.o \
	user-new-suggestions-api.o \
	cache-fetcher-misskey.o \
	prefetch.o \
	prefetch-influencers.o \
	blacklist.o \
	optout.o \
	test-blacklist.o \
	test-optout.o

cgi_binaries = \
	vinayaka-user-speed-api.cgi \
	vinayaka-user-search-api.cgi \
	vinayaka-user-match-filtered-api.cgi \
	vinayaka-suggestions-vocabulary-api.cgi \
	vinayaka-suggestions-newcomers-api.cgi \
	vinayaka-user-match-misskey-api.cgi

cron_binaries = \
	vinayaka-user-avatar-cron \
	vinayaka-user-speed-cron \
	vinayaka-store-raw-toots-cron \
	vinayaka-collect-raw-toots-cron \
	vinayaka-prefetch-cron \
	vinayaka-prefetch-influencers-cron

impl_binaries = \
	vinayaka-user-match-impl \
	vinayaka-user-match-cache-fetcher \
	vinayaka-user-match-cache-fetcher-osa \
	vinayaka-test-blacklist \
	vinayaka-test-optout

impl_scripts = \
	vinayaka-user-match-resource-guard

binaries = $(cgi_binaries) $(cron_binaries) $(impl_binaries)

cgi_scripts = \
	vinayaka-user-match-api.cgi \
	vinayaka-user-match-osa-api.cgi \
	vinayaka-user-profiles-api.cgi \
	vinayaka-suggestions-api.cgi \
	vinayaka-user-match-suggestions-api.cgi \
	vinayaka-user-new-suggestions-api.cgi

cron_scripts = \
	vinayaka-clear-cache-cron \
	vinayaka-https-renew-cron

cgis = $(cgi_binaries) $(cgi_scripts)

crons = $(cron_binaries) $(cron_scripts)

impls = $(impl_binaries) $(impl_scripts)

etcs = 

loading_libraries = -lsocialnet-1 -lcurl -lcrypto++ -llanguagemodel-1

.SUFFIXES: .cpp .o

.cpp.o:
	$(CXX) -c $(CFLAGS) $<

all: $(binaries)

vinayaka-user-match-impl: distsn.o user-match-api.o sort-user-speed.o blacklist.o optout.o
	$(CXX) distsn.o user-match-api.o sort-user-speed.o blacklist.o optout.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-user-avatar-cron: distsn.o user-avatar-cron.o sort-user-speed.o blacklist.o optout.o
	$(CXX) distsn.o user-avatar-cron.o sort-user-speed.o blacklist.o optout.o $(loading_libraries)  $(CFLAGS) -o $@

vinayaka-user-match-cache-fetcher: distsn.o cache-fetcher.o
	$(CXX) distsn.o cache-fetcher.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-user-match-cache-fetcher-osa: distsn.o cache-fetcher-osa.o
	$(CXX) distsn.o cache-fetcher-osa.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-user-match-filtered-api.cgi: distsn.o cache-fetcher-filtered.o
	$(CXX) distsn.o cache-fetcher-filtered.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-suggestions-vocabulary-api.cgi: distsn.o cache-fetcher-suggestions.o
	$(CXX) distsn.o cache-fetcher-suggestions.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-user-speed-cron: distsn.o user-speed-cron.o
	$(CXX) distsn.o user-speed-cron.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-user-speed-api.cgi: distsn.o sort-user-speed.o blacklist.o optout.o user-speed-api.o
	$(CXX) distsn.o sort-user-speed.o blacklist.o optout.o user-speed-api.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-store-raw-toots-cron: distsn.o sort-user-speed.o blacklist.o optout.o store-raw-toots.o
	$(CXX) distsn.o sort-user-speed.o blacklist.o optout.o store-raw-toots.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-collect-raw-toots-cron: distsn.o sort-user-speed.o blacklist.o optout.o collect-raw-toots.o
	$(CXX) distsn.o sort-user-speed.o blacklist.o optout.o collect-raw-toots.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-user-search-api.cgi: distsn.o sort-user-speed.o blacklist.o optout.o user-search-api.o
	$(CXX) distsn.o sort-user-speed.o blacklist.o optout.o user-search-api.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-suggestions-newcomers-api.cgi: distsn.o sort-user-speed.o blacklist.o optout.o user-new-suggestions-api.o
	$(CXX) distsn.o sort-user-speed.o blacklist.o optout.o user-new-suggestions-api.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-user-match-misskey-api.cgi: distsn.o cache-fetcher-misskey.o
	$(CXX) distsn.o cache-fetcher-misskey.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-prefetch-cron: distsn.o sort-user-speed.o blacklist.o optout.o prefetch.o
	$(CXX) distsn.o sort-user-speed.o blacklist.o optout.o prefetch.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-prefetch-influencers-cron: distsn.o sort-user-speed.o blacklist.o optout.o prefetch-influencers.o
	$(CXX) distsn.o sort-user-speed.o blacklist.o optout.o prefetch-influencers.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-test-blacklist: distsn.o sort-user-speed.o blacklist.o optout.o test-blacklist.o
	$(CXX) distsn.o sort-user-speed.o blacklist.o optout.o test-blacklist.o $(loading_libraries) $(CFLAGS) -o $@

vinayaka-test-optout: distsn.o sort-user-speed.o blacklist.o optout.o test-optout.o
	$(CXX) distsn.o sort-user-speed.o blacklist.o optout.o test-optout.o $(loading_libraries) $(CFLAGS) -o $@

install: $(cgis)
	$(INSTALL_PROGRAM) $(cgis) --target-directory=$(cgidir)
	$(INSTALL_PROGRAM) $(crons) --target-directory=$(bindir)
	$(INSTALL_PROGRAM) $(impls) --target-directory=$(bindir)
	$(MKDIR) -p $(etcdir)
	$(TOUCH) $(etcdir)/blacklisted_users.csv

uninstall:
	-$(RMDIR) $(etcdir)
	-$(RM) $(addprefix $(bindir)/,$(impls))
	-$(RM) $(addprefix $(bindir)/,$(crons))
	-$(RM) $(addprefix $(cgidir)/,$(cgis))

initialize:
	$(MKDIR) -p /var/lib/vinayaka
	$(MKDIR) -p /var/lib/vinayaka/user-speed
	$(MKDIR) -p /var/lib/vinayaka/raw-toots
	$(MKDIR) -p /var/lib/vinayaka/model
	$(MKDIR) -p /var/lib/vinayaka/user-first-toot
	$(MKDIR) -p /var/lib/vinayaka/match-cache.d
	$(TOUCH) /var/lib/vinayaka/model/concrete-user-words.csv
	$(TOUCH) /var/lib/vinayaka/model/popularity.csv
	$(TOUCH) /var/lib/vinayaka/users-new-store.json
	$(TOUCH) /var/lib/vinayaka/users-new-cache.json
	$(CHMOD) -R a+rwx /var/lib/vinayaka

refresh-cache:
	-$(RM) /var/lib/vinayaka/match-cache.d/*

refresh-all: refresh-cache
	-$(RM) /var/lib/vinayaka/*.json
	-$(RM) /var/lib/vinayaka/*.csv
	-$(RM) /var/lib/vinayaka/user-speed/*
	-$(RM) /var/lib/vinayaka/raw-toots/*.csv
	-$(RM) /var/lib/vinayaka/model/*.csv
	-$(RM) /var/lib/vinayaka/user-first-toot/*.json

clean:
	-$(RM) $(objects) $(binaries)

activate-cgi:
	a2enmod cgid
	service apache2 restart


