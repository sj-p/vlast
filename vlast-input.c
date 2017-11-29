#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <glib.h>
#include <curl/curl.h>

#include "vlast.h"


VlastData profile = {FALSE, FALSE, -1, -1, -1, -1, -1, -1, -1, -1,
                     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
VlastBuffer xml_buf = {NULL, 0, 0};

const gchar *tagtypes[] = {"artist", "album", "track", NULL};

const gchar *imgsizes[] = {"small", "medium", "large", "extralarge", "mega", NULL};
const gint num_imgsizes = sizeof (imgsizes) / sizeof (gchar*);


/* supported methods:
 *      api name, short api name, xml tag, mandatory params, ignored params,
 *          'from' param name, 'to' param name */
const gchar *methods[][NUM_METH_STR] =
{
    {"user.getinfo",             "u.gi",  "user",             "U",    "ABTP",   NULL, NULL},
    {"user.getrecenttracks",     "u.grt", "recenttracks",     "U",    "ABTP",   "from", "to"},
    {"user.gettopartists",       "u.gta", "topartists",       "U",    "ABT",    NULL, NULL},
    {"user.gettopalbums",        "u.gtb", "topalbums",        "U",    "ABT",    NULL, NULL},
    {"user.gettoptracks",        "u.gtt", "toptracks",        "U",    "ABT",    NULL, NULL},
    {"user.gettoptags",          "u.gtg", "toptags",          "U",    "ABTP",   NULL, NULL},
    {"user.getlovedtracks",      "u.glt", "lovedtracks",      "U",    "ABTP",   NULL, NULL},
    {"user.getfriends",          "u.gf",  "friends",          "U",    "ABTP",   NULL, NULL},
    {"user.getweeklyartistchart","u.gac", "weeklyartistchart","U",    "ABTGP",  "from", "to"},
    {"user.getweeklyalbumchart", "u.gbc", "weeklyalbumchart", "U",    "ABTGP",  "from", "to"},
    {"user.getweeklytrackchart", "u.gtc", "weeklytrackchart", "U",    "ABTGP",  "from", "to"},
    {"user.getweeklychartlist",  "u.gcl", "weeklychartlist",  "U",    "ABTGP",  NULL, NULL},
    {"user.getartisttracks",     "u.gat", "artisttracks",     "UA",   "BTG",    "starttimestamp", "endtimestamp"},
    {"user.getpersonaltags",     "u.gpg", "taggings",         "UGY",  "ABTP",   NULL, NULL},
    {"library.getartists",       "l.ga",  "artists",          "U",    "ABTP",   NULL, NULL},
    {"artist.getinfo",           "a.gi",  "artist",           "A",    "BTGP",   NULL, NULL},
    {"artist.getcorrection",     "a.gc",  "artist",           "A",    "BTGP",   NULL, NULL},
    {"artist.gettags",           "a.gg",  "tags",             "AU",   "BTGP",   NULL, NULL},
    {"artist.getsimilar",        "a.gs",  "similarartists",   "A",    "UBTP",   NULL, NULL},
    {"artist.gettopalbums",      "a.gtb", "topalbums",        "A",    "UBT",    NULL, NULL},
    {"artist.gettoptracks",      "a.gtt", "toptracks",        "A",    "UBT",    NULL, NULL},
    {"artist.gettoptags",        "a.gtg", "toptags",          "A",    "UBTGP",  NULL, NULL},
    {"artist.search",            "a.s",   "results",          "A",    "UBTGP",  NULL, NULL},
    {"album.getinfo",            "b.gi",  "album",            "BA",   "TP",     NULL, NULL},
    {"album.gettags",            "b.gg",  "tags",             "ABU",  "TGP",    NULL, NULL},
    {"album.gettoptags",         "b.gtg", "toptags",          "AB",   "UTP",    NULL, NULL},
    {"album.search",             "b.s",   "results",          "B",    "UTGP",   NULL, NULL},
    {"track.getinfo",            "t.gi",  "track",            "TA",   "BGP",    NULL, NULL},
    {"track.getcorrection",      "t.gc",  "corrections",      "TA",   "BGP",    NULL, NULL},
    {"track.gettags",            "t.gg",  "tags",             "ATU",  "BGP",    NULL, NULL},
    {"track.getsimilar",         "t.gs",  "similartracks",    "TA",   "UBP",    NULL, NULL},
    {"track.gettoptags",         "t.gtg", "toptags",          "TA",   "UBP",    NULL, NULL},
    {"track.search",             "t.s",   "results",          "T",    "UBGP",   NULL, NULL},
    {"chart.gettopartists",      "c.ga",  "artists",          "",     "UABTP",  NULL, NULL},
    {"chart.gettoptracks",       "c.gt",  "tracks",           "",     "UABTP",  NULL, NULL},
    {"chart.gettoptags",         "c.gg",  "tags",             "",     "UABTP",  NULL, NULL},
    {"geo.gettopartists",        "geo.a", "topartists",       "C",    "UABTP",  NULL, NULL},
    {"geo.gettoptracks",         "geo.t", "tracks",           "C",    "UABTP",  NULL, NULL},
    {"tag.getinfo",              "g.gi",  "tag",              "G",    "UABTP",  NULL, NULL},
    {"tag.getsimilar",           "g.gs",  "similartags",      "G",    "UABTP",  NULL, NULL},
    {"tag.gettopartists",        "g.gta", "topartists",       "G",    "UABT",   NULL, NULL},
    {"tag.gettopalbums",         "g.gtb", "albums",           "G",    "UABT",   NULL, NULL},
    {"tag.gettoptracks",         "g.gtt", "tracks",           "G",    "UABT",   NULL, NULL},
    {"tag.gettoptags",           "g.gtg", "toptags",          "",     "UABGTP", NULL, NULL},
    {"tag.getweeklychartlist",   "g.gcl", "weeklychartlist",  "G",    "UABTP",  NULL, NULL},
    {NULL,                       NULL,    NULL,               NULL,   NULL,     NULL, NULL}
};


/* period values:
 *      api name, short form */
const gchar *periods[][NUM_PER_STR] =
{
    {"overall",     "all"},
    {"7day",        "7"},
    {"1month",      "1"},
    {"3month",      "3"},
    {"6month",      "6"},
    {"12month",     "12"},
    {NULL,          NULL}
};


static size_t
mem_write (void *buffer, size_t size, size_t nmemb, void *stream)
{
    size_t txfer;

    txfer = size * nmemb;

    if ((xml_buf.bsize - xml_buf.windex) < txfer)
    {
        size_t req_size;

        /* need to enlarge buffer */

        /* calc new size:
         *  quadruple size until big enough
         *  min size 8192
         * */
        req_size = xml_buf.windex + txfer;
        do
        {
            xml_buf.bsize = MAX(8192, 4 * xml_buf.bsize);
        }
        while (xml_buf.bsize < req_size);

        xml_buf.buffer = realloc (xml_buf.buffer, xml_buf.bsize);

        DBG("  new buffer size %d", xml_buf.bsize);
    }

    /* copy data from libcurl's buffer */
    memcpy (xml_buf.buffer + xml_buf.windex, buffer, txfer);
    xml_buf.windex += txfer;

    DBG("  copied %d bytes to buffer", txfer);

    return txfer;
}


gint
index_image_size (const gchar *img_size)
{
    gint i;

    if (img_size == NULL) return -1;

    for (i = 0; imgsizes[i] != NULL; i++)
    {
        if (g_ascii_strcasecmp (img_size, imgsizes[i]) == 0)
            return i;
    }

    return -1;
}


static void
load_config ()
{
    gchar *api_key, *img_size = NULL;
    GKeyFile *kf;

    if (profile.config_file == NULL)
    {
        profile.config_file = g_build_filename (g_get_user_config_dir (),
                                                "vlast.conf",
                                                NULL);
    }
    else
    {
        DBG("CONFIG: trying custom config file %s", profile.config_file);
    }

    kf = g_key_file_new ();

    if (g_key_file_load_from_file (kf, profile.config_file, G_KEY_FILE_NONE, NULL))
    {
        DBG("CONFIG: loaded config file");

        api_key = g_key_file_get_string (kf, "Settings", "ApiKey", NULL);
        if (api_key != NULL)
        {
            profile.api_key = g_strdup (g_strstrip (api_key));
            g_free (api_key);
        }

        /* if time format wasn't on command line, read from config */
        if (profile.time_format == NULL)
        {
            profile.time_format = g_key_file_get_string (kf, "Settings", "TimeFormat", NULL);
        }

        /* if image size wasn't on command line, read from config */
        if (profile.img_size < 0)
        {
            img_size = g_key_file_get_string (kf, "Settings", "ImageSize", NULL);

            if (img_size != NULL && img_size[0] != '\0')
            {
                if ((profile.img_size = index_image_size (img_size)) < 0)
                {
                    ERR("CONFIG: illegal imagesize %s", img_size);
                }

            }

            g_free (img_size);
        }

    }
    else
    {
        DBG("CONFIG: failed to load config file %s", profile.config_file);
    }

    g_key_file_free (kf);

    if (profile.api_key == NULL)
    {
        profile.api_key = g_strdup ("3bd2ead9afcba9077c7cb7a6927260e0");

        DBG("CONFIG: didn't get ApiKey from config, using default");
    }

    if (profile.time_format == NULL)
    {
        profile.time_format = g_strdup ("%F %T");

        DBG("CONFIG: didn't get TimeFormat from config, using default");
    }
}


static gboolean
load_options (int *argc, char ***argv)
{
    gchar *method = NULL, *period = NULL, *input_file = NULL, *tagtype = NULL;
    gchar *img_size = NULL;
    gboolean retval, mlist = FALSE, plist = FALSE, ilist = FALSE;
    gint i;
    gint starts = -1, ends = -1, limit = -1, num_page = -1;
    GError *error = NULL;
    GOptionContext *context;
    GOptionEntry entries[] = {
        { "method",   'm',    0, G_OPTION_ARG_STRING, &method,
            "use API method M", "M" },
        { "user",     'u',    0, G_OPTION_ARG_STRING, &profile.user,
            "for LastFM username U", "U" },
        { "artist",   'a',    0, G_OPTION_ARG_STRING, &profile.artist,
            "for artist A", "A" },
        { "album",    'b',    0, G_OPTION_ARG_STRING, &profile.album,
            "for album B", "B" },
        { "track",    't',    0, G_OPTION_ARG_STRING, &profile.track,
            "for track T", "T" },
        { "tag",      'g',    0, G_OPTION_ARG_STRING, &profile.tag,
            "for tag G", "G" },
        { "tagtype",  'y',    0, G_OPTION_ARG_STRING, &tagtype,
            "for tagging type Y [artist|album|track]", "Y" },
        { "country",    'c',    0, G_OPTION_ARG_STRING, &profile.country,
            "for ISO 3166-1 country name C (geo charts)", "C" },
        { "limit",    'l',    0, G_OPTION_ARG_INT, &limit,
            "fetch L items per page", "L" },
        { "page-num", 'n',    0, G_OPTION_ARG_INT, &num_page,
            "fetch page number N (1..)", "N" },
        { "period",   'p',    0, G_OPTION_ARG_STRING, &period,
            "fetch data for period P", "P" },
        { "starts",    0,     0, G_OPTION_ARG_INT, &starts,
            "fetch data from start S", "S" },
        { "ends",      0,     0, G_OPTION_ARG_INT, &ends,
            "fetch data till end E", "E" },
        { "outfile",  'o',    0, G_OPTION_ARG_FILENAME, &profile.xml_file,
            "output returned xml to file F", "F" },
        { "infile",   'i',    0, G_OPTION_ARG_FILENAME, &input_file,
            "don't fetch data, process xml file F", "F" },
        { "debug",    'd',    0, G_OPTION_ARG_NONE, &profile.debug,
            "print debug info on stderr", NULL },
        { "config",     0,    0, G_OPTION_ARG_FILENAME, &profile.config_file,
            "use configuration file F", "F" },
        { "time-format", 'T', 0, G_OPTION_ARG_STRING, &profile.time_format,
            "print date/time using format string T", "T" },
        { "image-size", 'I', 0, G_OPTION_ARG_STRING, &img_size,
            "print image URLs for size I (default: no urls)", "I" },
        { "list-methods",  0, 0, G_OPTION_ARG_NONE, &mlist,
            "list supported methods, then exit", NULL },
        { "list-periods",  0, 0, G_OPTION_ARG_NONE, &plist,
            "list short & long period strings, then exit", NULL },
        { "list-image-sizes",  0, 0, G_OPTION_ARG_NONE, &ilist,
            "list image size names", NULL },
        { NULL }
    };

    setlocale (LC_ALL, "");

    context = g_option_context_new (" - fetch & print data from LastFM");

    g_option_context_add_main_entries (context, entries, NULL);

    if (g_option_context_parse (context, argc, argv, &error))
    {
        DBG("OPTS: parsed options OK");
    }
    else
    {
        ERR("OPTS: option parsing failed: %s", error->message)

        g_error_free (error);

        return FALSE;
    }

    g_option_context_free (context);

    /* check options */
    retval = TRUE;

    if (mlist)
    {
        printf ("\nThese methods are currently supported (short & long names):\n");
        for (i = 0; methods[i][METH_STR_API] != 0; i++)
        {
            printf ("%8s    %s\n", methods[i][METH_STR_SHORT],
                                methods[i][METH_STR_API]);
        }
    }
    if (plist)
    {
        printf ("\nShort & long period strings:\n");
        for (i = 0; periods[i][PER_STR_API] != 0; i++)
        {
            printf ("%8s    %s\n", periods[i][PER_STR_SHORT],
                                    periods[i][PER_STR_API]);
        }
    }
    if (ilist)
    {
        printf ("\nImage size names:\n");
        for (i = 0; imgsizes[i] != 0; i++)
        {
            printf ("    %s\n", imgsizes[i]);
        }
    }
    if (mlist || plist || ilist) exit (0);

    if (input_file != NULL)
    {
        if (profile.xml_file == NULL)
        {
            DBG("OPTS: to use file input");

            profile.from_file = TRUE;

            profile.xml_file = input_file;
        }
        else
        {
            ERR("OPTS: you can't use both -i/--infile and -o/--outfile");

            g_free (input_file);

            retval = FALSE;
        }

        input_file = NULL;
    }

    /* process img_size */
    if (img_size != NULL)
    {
        if ((profile.img_size = index_image_size (img_size)) < 0)
        {
            ERR("OPTS: illegal imagesize %s", img_size);
        }

        g_free (img_size);
    }

    /* check API options */
    if (!profile.from_file)
    {
        if (method == NULL)
        {
            ERR("OPTS: no method given");

            retval = FALSE;
        }
        else
        {
            /* check method */
            for (i = 0; methods[i][METH_STR_API] != 0; i++)
            {
                if (g_ascii_strcasecmp (method, methods[i][METH_STR_SHORT]) == 0 ||
                    g_ascii_strcasecmp (method, methods[i][METH_STR_API]) == 0)
                {
                    profile.method = i;
                    break;
                }
            }
            if (profile.method < 0)
            {
                ERR("OPTS: method not supported");

                retval = FALSE;
            }
        }

        /* process tagtype */
        if (tagtype != NULL)
        {
            for (i = 0; tagtypes[i] != NULL; i++)
            {
                if (g_ascii_strcasecmp (tagtype, tagtypes[i]) == 0)
                {
                    profile.tagtype = i;
                    break;
                }
            }
            if (profile.tagtype < 0)
            {
                ERR("OPTS: invalid tagtype");
            }

            g_free (tagtype);
        }

        /* decipher period */
        if (period != NULL)
        {
            for (i = 0; periods[i][PER_STR_API] != 0; i++)
            {
                if (g_ascii_strcasecmp (period, periods[i][PER_STR_SHORT]) == 0 ||
                    g_ascii_strcasecmp (period, periods[i][PER_STR_API]) == 0)
                {
                    profile.period = i;

                    break;
                }
            }
            if (profile.period < 0)
            {
                ERR("OPTS: value of period option not recognised, ignoring");
            }
            else
            {
                DBG("OPTS: set period to %d (%s)", profile.period,
                                                   periods[profile.period][PER_STR_API]);
            }
        }

        if (starts > 0) profile.starts = starts;
        if (ends > 0) profile.ends = ends;
        if (limit > 0) profile.limit = limit;
        if (num_page > 0) profile.num_page = num_page;

        /* skip method-related options if method was invalid */
        if (profile.method < 0) goto exit_opts;

        /* check all mandatory options are present */
        DBG("OPTS: checking mandatory options %s present",
                    methods[profile.method][METH_STR_MAND]);
        for (i = 0; i < strlen (methods[profile.method][METH_STR_MAND]); i++)
        {
            switch (methods[profile.method][METH_STR_MAND][i])
            {
                case 'U':
                    if (profile.user == NULL)
                    {
                        ERR("OPTS: need user for this method");
                        retval = FALSE;
                    }
                    break;

                case 'A':
                    if (profile.artist == NULL)
                    {
                        ERR("OPTS: need artist for this method");
                        retval = FALSE;
                    }
                    break;

                case 'B':
                    if (profile.album == NULL)
                    {
                        ERR("OPTS: need album for this method");
                        retval = FALSE;
                    }
                    break;

                case 'T':
                    if (profile.track == NULL)
                    {
                        ERR("OPTS: need track for this method");
                        retval = FALSE;
                    }
                    break;

                case 'G':
                    if (profile.tag == NULL)
                    {
                        ERR("OPTS: need tag for this method");
                        retval = FALSE;
                    }
                    break;

                case 'C':
                    if (profile.country == NULL)
                    {
                        ERR("OPTS: need country for this method");
                        retval = FALSE;
                    }
                    break;

                case 'Y':
                    if (profile.tagtype < 0)
                    {
                        ERR("OPTS: need tagtype for this method");
                        retval = FALSE;
                    }
                    break;
            }
        }

        /* ignore superfluous options */
        DBG("OPTS: removing superfluous options %s",
                    methods[profile.method][METH_STR_FORB]);
        for (i = 0; i < strlen (methods[profile.method][METH_STR_FORB]); i++)
        {
            switch (methods[profile.method][METH_STR_FORB][i])
            {
                case 'U':
                    g_free (profile.user);
                    profile.user = NULL;
                    break;

                case 'A':
                    g_free (profile.artist);
                    profile.artist = NULL;
                    break;

                case 'T':
                    g_free (profile.track);
                    profile.track = NULL;
                    break;

                case 'B':
                    g_free (profile.album);
                    profile.album = NULL;
                    break;

                case 'G':
                    g_free (profile.tag);
                    profile.tag = NULL;
                    break;

                case 'P':
                    profile.period = -1;
                    break;

                case 'Y':
                    profile.tagtype = -1;
                    break;

                case 'S':
                    profile.starts = -1;
                    break;

                case 'E':
                    profile.ends = -1;
                    break;

            }
        }
    }


exit_opts:
    g_free (method);
    g_free (period);

    return retval;
}


static void
param_append_str (gchar **string, const gchar *param, const gchar *value)
{
    gchar *temp, *encoded;

    if (string == NULL || param == NULL || value == NULL) return;

    DBG("RQ: add str '%s=%s'", param, value);

    /* NB option parser converts CL strings to utf8, so
     * no conversion is needed here */
    encoded = g_uri_escape_string (value, NULL, FALSE);

    temp = g_strdup_printf ("%s%s%s%c%s",
                            (*string == NULL ? "" : *string),
                            (*string == NULL ? "" : "&"),
                            param, '=', encoded);

    g_free (encoded);

    g_free (*string);

    *string = temp;
}


static void
param_append_int (gchar **string, const gchar *param, gint value)
{
    gchar *temp;

    if (string == NULL || param == NULL || value < 0) return;

    DBG("RQ: add int '%s=%d'", param, value);

    /* NB option parser converts CL strings to utf8, so
     * no conversion is needed here */

    temp = g_strdup_printf ("%s%s%s%c%d",
                            (*string == NULL ? "" : *string),
                            (*string == NULL ? "" : "&"),
                            param, '=', value);

    g_free (*string);

    *string = temp;
}


static gboolean
load_xml_from_file ()
{
    gboolean retval;
    GError *error = NULL;

    retval = g_file_get_contents (profile.xml_file,
                                  &xml_buf.buffer,
                                  &xml_buf.windex,
                                  &error);

    if (retval)
    {
        DBG("FILE: loaded xml file, length %u bytes", xml_buf.windex);
    }
    else
    {
        ERR("FILE: failed to load from file:\n    %s", error->message);

        g_error_free (error);
    }

    return retval;
}


static gboolean
make_request ()
{
    gchar *request;
    gboolean retval = TRUE;
    GError *error = NULL;
    CURL *curl;
    CURLcode res = CURLE_OK;

    request = g_strconcat ("http://ws.audioscrobbler.com/2.0/"
                           "?method=", methods[profile.method][METH_STR_API],
                           "&api_key=", profile.api_key,
                           NULL);

    /* add other params, NULL/-ve values are ignored in param_append_*() */
    param_append_str (&request, "user", profile.user);
    param_append_str (&request, "artist", profile.artist);
    param_append_str (&request, "album", profile.album);
    param_append_str (&request, "track", profile.track);
    param_append_str (&request, "tag", profile.tag);
    param_append_str (&request, "country", profile.country);
    param_append_int (&request, "limit", profile.limit);
    param_append_int (&request, "page", profile.num_page);
    param_append_int (&request, methods[profile.method][METH_STR_START], profile.starts);
    param_append_int (&request, methods[profile.method][METH_STR_END], profile.ends);

    if (profile.period >=0)
    {
        param_append_str (&request, "period", periods[profile.period][PER_STR_API]);
    }

    if (profile.tagtype >=0)
    {
        param_append_str (&request, "taggingtype", tagtypes[profile.tagtype]);
    }

    DBG("RQ: url = %s", request);
    //exit(0);


    curl = curl_easy_init ();
    if (curl == NULL)
    {
        retval = FALSE;
        ERR("RQ: failed to init curl");

        goto exit_rq;
    }

    curl_easy_setopt (curl, CURLOPT_URL, request);
    curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, mem_write);

    res = curl_easy_perform (curl);

    if (profile.xml_file != NULL &&
        !g_file_set_contents (profile.xml_file, xml_buf.buffer, xml_buf.windex, &error))
    {
        ERR("RQ: failed to save response:\n     %s", error->message);

        g_error_free (error);
    }

    curl_easy_cleanup (curl);
    if (res == CURLE_OK)
    {
        DBG("RQ: fetch was successful, %u bytes", xml_buf.windex);
    }
    else
    {
        retval = FALSE;
        ERR("RQ: curl failed with code %d", res);

        goto exit_rq;
    }

exit_rq:
    g_free (request);

    return retval;
}


static void
free_profile ()
{
    g_free (xml_buf.buffer);

    g_free (profile.album);
    g_free (profile.api_key);
    g_free (profile.artist);
    g_free (profile.country);
    g_free (profile.time_format);
    g_free (profile.tag);
    g_free (profile.track);
    g_free (profile.user);
    g_free (profile.xml_file);
    g_free (profile.config_file);
}


int
main (int argc, char **argv)
{
    gboolean retval;

    retval = load_options (&argc, &argv);

    if (retval) load_config ();

    if (retval)
    {
        if (profile.from_file)
        {
            retval = load_xml_from_file ();
        }
        else
        {
            retval = make_request ();
        }
    }

    if (retval) retval = load_xml_doc ();

    free_profile ();

    return !retval;
}
