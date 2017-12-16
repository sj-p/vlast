Vlast
=====

A simple command line program for GNU/Linux to access last.fm's API

This program allows last.fm API queries to be made, and a subset of the
returned data is presented in a human-readable format.

See the last.fm API documentation at https://www.last.fm/api


Building
--------

Vlast needs these libraries:

    glib-2.0    (>= 2.16)
    libcurl
    libxml-2.0

To build you'll need a working C compiler. Also, the Makefile assumes that
you have pkg-config.

Clone the repo and cd into the vlast directory. Just run make to compile.
There is no configure script, you may need to edit the Makefile if your
setup is non-standard.

You can run the vlast executable from the build directory, or copy it to
wherever is convenient.


Configuration File
------------------

This is a key-value file similar to .desktop files for per-user settings.
Vlast looks in group [Settings] for keys ApiKey, ApiSecret, TimeFormat &
ImageSize

    ApiKey and ApiSecret are where you declare your own API key & API secret.
        (if either is missing defaults to internal key)

    TimeFormat is a strftime format string, to customise the way date/time
        is printed. (overriden by --time-format/-T; defaults to "%F %T")

    ImageSize is the name of the image size for which to print image urls.
        (overriden by --image-size/-I; defaults to no urls)

The default location for a configuration file is XDG_CONFIG_HOME/vlast.conf
(which will usually be ~/.config/vlast.conf). Or you can specify a location
on the command line using the --config option. If the file is not found,
default settings are used.


Use
---

List a summary of options:
    $ vlast -h

List supported names (including short names) for methods, periods, image sizes:
    $ vlast --list-methods|--list-periods|--list-image-sizes

Fetch data from last.fm according to given method:
    $ vlast [--config=file] [-d] -m method [parameter options] [-o xmlfile]

Read and print data from a previously saved file:
    $ vlast [--config=file] [-d] -i xmlfile


Command-line Options
--------------------

General:
    -h, --help              show help text
    --list-methods          list supported methods, then exit
    --list-periods          list short & long period strings, then exit
    --list-image-sizes      list image size names
    --list-langs            list supported language codes
    -d, --debug             print debug info on stderr
    --config=F              use configuration file F

Define your request:
    -m, --method=M          use API method M
    -s, --signed            force signed request
    -u, --user=U            for LastFM username U
    -a, --artist=A          for artist A
    -b, --album=B           for album B
    -r, --album-artist=R    for album-artist R
    -t, --track=T           for track T
    -g, --tag=G             for tag G (taglist for methods *.addtags)
    -y, --tagtype=Y         for tagging type Y artist|album|track
    -c, --country=C         for ISO 3166-1 country name C (geo charts)
    -L, --lang=LL           for ISO 639 alpha-2 language code LL
    -A, --autocorrect=V     set autocorrect to state V [0|1]
    -k, --token=K           token string K from auth.gettoken
    --timestamp=T           at unix timestamp T
    --duration=D            with duration D seconds

Specify bounds on data:
    -l, --limit=L           fetch L items per page
    -n, --page-num=N|N1-N2  fetch page number N or range N1-N2
    -p, --period=P          fetch data for period P
    --starts=S              fetch data from start time S
    --ends=E                fetch data till end time E

Output control:
    -T, --time-format=T     print date/time using format string T
    -I, --image-size=I      print image URLs for size I (default: no urls)
    -w, --wiki-full         print full wiki text (default: summary only)
    -q, --quiet             don't print output after saving xml file

XML files:
    -o, --outfile=F         output returned xml to file F
    -i, --infile=F          don't fetch data, process xml file F

Notes:
* values L, N & D are positive integers
* values S, E & T are unix timestamps (seconds since Epoch)
* short names of method M & period P can be used, see --list-*
* value G is a comma-separated list of up to 10 tags for methods *.addtags
    otherwise it's a single tag
* option names are not always the same as API parameter names, but it should
    be obvious to which parameter they refer
* by default, image urls are not printed; if enabled in config or on command
    line, some urls to images with requested size are printed, or, if not
    available, the next larger size, or if there is none, the next smaller
    size. N.B. API terms of use forbid you from accessing images at these URLs...


Page Ranges
-----------

For methods that support paging, it is possible to fetch a single page (-n N)
or a range of pages (-n N1-N2).

When using file in/out with a page range, it is necessary to generate a
filename for each page, based on the page number. The filename given on
the command line is used as a printf format string: one decimal specification
'%d' is required, zero-padding and precision digits are permitted. You can
represent a percent sign in the filename with '%%'.

Ranges where lower & upper limit are the same are allowed, resulting in
fetching/processing a single page. But the filename string must be a valid
format string.

This would generate files "test-002.xml" "test-003.xml" "test-004.xml":
    $ vlast -m x.xxx -o "test-%03d.xml" -n 2-4

This expects file "test-5.xml" to be present:
    $ vlast -i "test-%d.xml" -n 5-5


More Examples
-------------

Get last 5 scrobbles from my test account:
    $ vlast -m user.getrecenttracks -u fake_user_0 -l 5

Same again, but with short method name, save resulting xml:
    $ vlast -m u.grt -u fake_user_0 -l 5 -o last5.xml

Print data saved above, using custom date/time format:
    $ vlast -i last5.xml -T "%F (%a) %T"

Get info on that account, use a specific config file, print debug info:
    $ vlast -d -m u.gi -u fake_user_0 --config my_config

Get info on a specific track, include mega size image url:
    $ vlast -m t.gi -a Cher -t Believe --image-size=mega


Authentication
-------------
In order to use signed/write methods, you need to authenticate your user
account with the API key which you are using. The procedure is as follows:

1)  $ vlast --method=auth.gettoken
This will print a url that you should copy into & visit in a web browser,
and there accept the application.

2)  $ vlast --method=auth.getsession --token=<token>
replacing <token> by the token printed in the output above. On success, this
will print the username that has been authenticated and save the session key.

3) Check everything is okay by making a signed request, e.g.:
    $ vlast --method=user.getinfo --user=<username> --signed

The application should now appear in the list in your settings at
https://www.last.fm/settings/applications

The session key is stored at XDG_DATA_HOME/vlast/sk/<api_key>/<username>
(usually ~/.local/share/vlast/sk/<api_key>/<username>).


API Key
-------

While vlast has its own API key, it is recommended that users obtain & use
their own key. Obtain your own key by creating an API account at:
    https://www.last.fm/api/account/create
Copy the API key and secret strings into a configuration file as detailed
earlier.

When fetching a range of pages, vlast uses a crude rate-limiter to stay within
last.fm's rules. Otherwise, if you are making multiple requests, it's your
duty to stay within bounds, or banning by last.fm may result.


Limitations
-----------

* not all request parameters are supported
* not all returned data is used in generating output (but saving xml with
    the --outfile option allows data to be used by other tools)


Bugs
----

Lots, I'm sure.


TODO
----
