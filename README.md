Vlast - a simple command line program for GNU/Linux to access last.fm's API
===========================================================================

This program allows last.fm API queries to be made, and a subset of the returned
data is presented in a human-readable format.

See the last.fm API documentation at https://www.last.fm/api


Building
--------
Vlast needs these libraries:
    glib-2.0    >= 2.16
    libcurl
    libxml-2.0

To build you'll need a working C compiler. Also, the Makefile assumes that
you have pkg-config.

Clone the repo and cd into the vlast directory. Just run 'make' to compile.
There is no configure script, you may need to edit the Makefile if your
setup is non-standard.

You can run the vlast executable from the build directory, or copy it to
wherever is convenient.


Configuration File
------------------
This is a key-value file similar to .desktop files. Vlast looks for keys
ApiKey, TimeFormat & ImageSize in group [Settings]

    ApiKey is where you declare your own API key. (defaults to internal key)

    TimeFormat is a strftime format string, to customise the way date/time
        is printed. (overriden by --time-format/-T; defaults to "%F %T")

    ImageSize is the name of the image size for which to print image urls.
        (overriden by --image-size/I; defaults to no urls)

The default location for a configuration file is XDG_CONFIG_HOME/vlast.conf
(which will usually be ~/.config/vlast.conf). Or you can specify a location
on the command line using the --config option. If the file is not found,
default settings are used.


Use
---
    List a summary of options:
        vlast -h

    List supported method names and/or period names, including short names
        vlast --list-methods|--list-periods

    Fetch data from last.fm according to given method
        vlast [--config=file] [-d] -m method [parameter options] [-o xmlfile]

    Read and print data from a previously saved file
        vlast [--config=file] [-d] -i xmlfile


Command-line Options
--------------------
General:
  -h, --help           show help text
  --list-methods       list supported methods, then exit
  --list-periods       list short & long period strings, then exit
  --list-image-sizes   list image size names
  -d, --debug          print debug info on stderr
  --config=F           use configuration file F

Define your request:
  -m, --method=M       use API method M
  -u, --user=U         for LastFM username U
  -a, --artist=A       for artist A
  -b, --album=B        for album B
  -t, --track=T        for track T
  -g, --tag=G          for tag G
  -y, --tagtype=Y      for tagging type Y [artist|album|track]
  -c, --country=C      for ISO 3166-1 country name C (geo charts)

Specify limitations on data:
  -l, --limit=L        fetch L items per page
  -n, --page-num=N     fetch page number N (1..)
  -p, --period=P       fetch data for period P
  --starts=S           fetch data from start time S
  --ends=E             fetch data till end time E

Output control:
  -T, --time-format=T  print date/time using format string T
  -I, --image-size=I   print image URLs for size I (default: no urls)

XML files:
  -o, --outfile=F      output returned xml to file F
  -i, --infile=F       don't fetch data, process xml file F


Notes:
    * values L & N are integers
    * values S & E are unix timestamps (seconds since since Epoch)
    * short names of method M & period P can be used, see --list-*
    * option names are not always the same as API parameter names, but it
        should be obvious to which parameter they refer
    * by default, image urls are not printed; if enabled in config or on
        command line, some urls to images with requested size are printed,
        or, if not available, the next larger size, or if there is none,
        the next smaller size


Examples
--------
    Get last 5 scrobbles from my test account:
        vlast -m user.getrecenttracks -u fake_user_0 -l 5

    Same again, but with short method name, save resulting xml:
        vlast -m u.grt -u fake_user_0 -l 5 -o last5.xml

    Print data saved above, using custom date/time format:
        vlast -i last5.xml -T "%F (%a) %T"

    Get info on that account, use a specific config file, print debug info:
        vlast -d -m u.gi -u fake_user_0 --config my_config

    Get info on a specific track, include mega size image url:
        vlast -m t.gi -a Cher -t Believe --image-size=mega


API Key
-------
While vlast has its own API key, it is recommended that users obtain & use
their own key. Obtain your own key by creating an API account at:
    https://www.last.fm/api/account/create

Since the program makes at most one request before exiting, vlast does no
rate-limiting. If you are making multiple requests, ensure that you comply
with last.fm's requirements; otherwise banning by last.fm may result.


Limitations
-----------
* not all API methods are supported, notably write methods
* not all request parameters are supported
* not all returned data is used in generating output (but see --outfile option)


Bugs
----
    Lots, I'm sure.


TODO
----
* support specifying a page range, and fetch data for each page consecutively
* vlast ought to drop all superfluous API parameters when fetching data
    (this is as yet only partially implemented)
