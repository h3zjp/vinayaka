# User Matching for GNU social, Mastodon, Pleroma, and microblog.pub

Find users similar to you, by their vocabulary.

https://vinayaka.distsn.org
(Alias: http://mastodonusermatching.tk)

## APIs

### Matching

https://vinayaka.distsn.org/cgi-bin/vinayaka-user-match-api.cgi?mastodon.social+Gargron  
(Full)

http://vinayaka.distsn.org/cgi-bin/vinayaka-user-match-filtered-api.cgi?mastodon.social+Gargron  
(Except oneself, bots, blacklisted, following)

https://vinayaka.distsn.org/cgi-bin/vinayaka-user-match-osa-api.cgi?mastodon.social+Gargron  
([おすすめフォロワー](https://followlink.osa-p.net/recommend.html) compatible)

https://vinayaka.distsn.org/cgi-bin/vinayaka-user-match-suggestions-api.cgi?mastodon.social+Gargron  
(Mastodon's `/api/v1/suggestions` compatible)

### Search

https://vinayaka.distsn.org/cgi-bin/vinayaka-user-search-api.cgi?nagiept

### Newcomers

http://vinayaka.distsn.org/cgi-bin/vinayaka-user-new-api.cgi  
(Full)

http://vinayaka.distsn.org/cgi-bin/vinayaka-user-new-suggestions-api.cgi?mastodon.social+Gargron  
(Mastodon's `/api/v1/suggestions` compatible)

### Active users

http://vinayaka.distsn.org/cgi-bin/vinayaka-user-speed-api.cgi?100 (Top n)

http://vinayaka.distsn.org/cgi-bin/vinayaka-user-speed-api.cgi (Full)

## Depends on

* https://gitlab.com/distsn/libsocialnet  
* https://gitlab.com/distsn/liblanguagemodel  

### Optional

* https://gitlab.com/distsn/vinayaka-blacklist (Anti-abuse policy)
* https://gitlab.com/distsn/distsn-fe (Web frontend)

## Install

Optional dependency: 

    $ sudo apt install build-essential libcurl4-openssl-dev libtinyxml2-dev libcrypto++-dev apache2
    $ make clean
    $ make
    $ sudo make install
    $ sudo make initialize
    $ sudo make activate-cgi
    $ crontab -e

Write following code in crontab:

```
 6 */4 * * * /usr/local/bin/vinayaka-user-speed-cron
12 */4 * * * /usr/local/bin/vinayaka-user-new-cron
18 5   * * * /usr/local/bin/vinayaka-user-avatar-cron
24 *   * * * /usr/local/bin/vinayaka-clear-cache-cron
30 */6 * * * /usr/local/bin/vinayaka-collect-raw-toots-cron
36 */6 * * * /usr/local/bin/vinayaka-store-raw-toots-cron
42 */8 * * * /usr/local/bin/vinayaka-prefetch-cron
48 */8 * * * /usr/local/bin/vinayaka-prefetch-influencers-cron
```

Write following code in crontab for the root:

```
6 2 * * 2 /usr/local/bin/vinayaka-https-renew-cron
```

## Update

    $ make clean
    $ make
    $ sudo make install
