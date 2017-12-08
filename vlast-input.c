#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <unistd.h>
#include <glib.h>
#include <curl/curl.h>

#include "vlast.h"


VlastData profile = {FALSE, FALSE, FALSE, FALSE,
                     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
VlastBuffer xml_buf = {NULL, 0, 0};

const gchar *tagtypes[] = {"artist", "album", "track", NULL};

const gchar *imgsizes[] = {"small", "medium", "large", "extralarge", "mega", NULL};
const gint num_imgsizes = sizeof (imgsizes) / sizeof (gchar*);

const gchar *languages[] = {"de", "en", "es", "fr", "it", "ja", "pl", "pt",
                            "ru", "sv", "tr", "zh", NULL};

const gchar *autocorrects[] = {"0", "1", NULL};


/* supported methods:
 *      api name, short api name, xml tag, mandatory params, ignored params,
 *          'from' param name, 'to' param name */
const gchar *methods[][NUM_METH_STR] =
{
    {"user.getinfo",             "u.gi", "user",             "U",  "ABTGPNLV",NULL,NULL},
    {"user.getrecenttracks",     "u.grt","recenttracks",     "U",  "ABTGPLV", "from","to"},
    {"user.gettopartists",       "u.gta","topartists",       "U",  "ABTGLV",  NULL,NULL},
    {"user.gettopalbums",        "u.gtb","topalbums",        "U",  "ABTGLV",  NULL,NULL},
    {"user.gettoptracks",        "u.gtt","toptracks",        "U",  "ABTGLV",  NULL,NULL},
    {"user.gettoptags",          "u.gtg","toptags",          "U",  "ABTGPLV", NULL,NULL},
    {"user.getlovedtracks",      "u.glt","lovedtracks",      "U",  "ABTGPLV", NULL,NULL},
    {"user.getfriends",          "u.gf", "friends",          "U",  "ABTGPLV", NULL,NULL},
    {"user.getweeklyartistchart","u.gac","weeklyartistchart","U",  "ABTGPLV", "from","to"},
    {"user.getweeklyalbumchart", "u.gbc","weeklyalbumchart", "U",  "ABTGPLV", "from","to"},
    {"user.getweeklytrackchart", "u.gtc","weeklytrackchart", "U",  "ABTGPLV", "from","to"},
    {"user.getweeklychartlist",  "u.gcl","weeklychartlist",  "U",  "ABTGPLV", NULL,NULL},
    {"user.getartisttracks",     "u.gat","artisttracks",     "UA", "BTGL",   "starttimestamp","endtimestamp"},
    {"user.getpersonaltags",     "u.gpg","taggings",         "UGY","ABTPLV",  NULL,NULL},
    {"library.getartists",       "l.ga", "artists",          "U",  "ABTGPL",  NULL,NULL},
    {"artist.getinfo",           "a.gi", "artist",           "A",  "BTGPN",   NULL,NULL},
    {"artist.getcorrection",     "a.gc", "artist",           "A",  "UBTGPNLV",NULL,NULL},
    {"artist.gettags",           "a.gg", "tags",             "AU", "BTGPL",   NULL,NULL},
    {"artist.getsimilar",        "a.gs", "similarartists",   "A",  "UBTGPL",  NULL,NULL},
    {"artist.gettopalbums",      "a.gtb","topalbums",        "A",  "UBTGPL",  NULL,NULL},
    {"artist.gettoptracks",      "a.gtt","toptracks",        "A",  "UBTGPL",  NULL,NULL},
    {"artist.gettoptags",        "a.gtg","toptags",          "A",  "UBTGPL",  NULL,NULL},
    {"artist.search",            "a.s",  "results",          "A",  "UBTGPLV", NULL,NULL},
    {"album.getinfo",            "b.gi", "album",            "BA", "TGPN",    NULL,NULL},
    {"album.gettags",            "b.gg", "tags",             "ABU","TGPL",    NULL,NULL},
    {"album.gettoptags",         "b.gtg","toptags",          "AB", "UTGPL",   NULL,NULL},
    {"album.search",             "b.s",  "results",          "B",  "UTGPLV",  NULL,NULL},
    {"track.getinfo",            "t.gi", "track",            "TA", "BGPN",    NULL,NULL},
    {"track.getcorrection",      "t.gc", "corrections",      "TA", "UBGPNLV", NULL,NULL},
    {"track.gettags",            "t.gg", "tags",             "ATU","BGPL",    NULL,NULL},
    {"track.getsimilar",         "t.gs", "similartracks",    "TA", "UBGPL",   NULL,NULL},
    {"track.gettoptags",         "t.gtg","toptags",          "TA", "UBGPL",   NULL,NULL},
    {"track.search",             "t.s",  "results",          "T",  "UBGPLV",  NULL,NULL},
    {"chart.gettopartists",      "c.ga", "artists",          "",   "UABGTPLV",NULL,NULL},
    {"chart.gettoptracks",       "c.gt", "tracks",           "",   "UABGTPLV",NULL,NULL},
    {"chart.gettoptags",         "c.gg", "tags",             "",   "UABGTPLV",NULL,NULL},
    {"geo.gettopartists",        "geo.a","topartists",       "C",  "UABGTPLV",NULL,NULL},
    {"geo.gettoptracks",         "geo.t","tracks",           "C",  "UABGTPLV",NULL,NULL},
    {"tag.getinfo",              "g.gi", "tag",              "G",  "UABTPNV", NULL,NULL},
    {"tag.getsimilar",           "g.gs", "similartags",      "G",  "UABTPLV", NULL,NULL},
    {"tag.gettopartists",        "g.gta","topartists",       "G",  "UABTPLV", NULL,NULL},
    {"tag.gettopalbums",         "g.gtb","albums",           "G",  "UABTPLV", NULL,NULL},
    {"tag.gettoptracks",         "g.gtt","tracks",           "G",  "UABTPLV", NULL,NULL},
    {"tag.gettoptags",           "g.gtg","toptags",          "",   "UABGTPLV",NULL,NULL},
    {"tag.getweeklychartlist",   "g.gcl","weeklychartlist",  "G",  "UABTPLV", NULL,NULL},
    {NULL,                       NULL,   NULL,               NULL, NULL,      NULL,NULL}
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


/* parse page num/range
 * return TRUE on error */
static gboolean
get_page_range (const gchar *page_str)
{
    gchar *eptr;
    gint first_page = 0, last_page = 0;

    if (page_str == NULL) return FALSE;
    if (page_str[0] == '\0') return TRUE;

    first_page = strtol (page_str, &eptr, 0);
    if (first_page < 1)
    {
        return TRUE;
    }
    if (eptr[0] == '\0')
    {
        profile.num_page = first_page;

        return FALSE;
    }
    if (eptr[0] != '-' || eptr[1] == '\0')
    {
        return TRUE;
    }
    last_page = strtol (eptr + 1, &eptr, 0);
    if (eptr[0] != '\0')
    {
        return TRUE;
    }
    if (last_page >= first_page)
    {
        profile.num_page = first_page;
        profile.last_page = last_page;

        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


/* when processing multiple pages, check filename contains
 * single number format '%[0-9]*d'
 * return TRUE on error */
static gboolean
check_xml_filename ()
{
    gint i;
    gboolean have_pc = FALSE;
    const gchar *p;
    gchar *eptr;

    /* nothing to do if range not requested or no file in/out */
    if (profile.last_page < 1 || profile.xml_file == NULL )
    {
        DBG("FILECHECK: no check necessary");

        return FALSE;
    }

    /* check there's a single '%' */
    for (p = profile.xml_file; *p != '\0'; p++)
    {
        if (*p == '%')
        {
            if (have_pc) return TRUE;
            have_pc = TRUE;
            DBG("FILECECK: got first '%%'");

            p++;
            if (*p == '\0') return TRUE;
            if (*p == '0')
            {
                p++;
                if (*p == '\0') return TRUE;
                DBG("FILECECK: got '0' padding");
            }
            i = strtol (p, &eptr, 10);
            DBG("FILECECK: got precision %d", i);
            if (i < 0) return TRUE;
            if (*eptr != 'd') return TRUE;
            DBG("FILECECK: got 'd' specifier");
            p = eptr;
        }
    }

    return (!have_pc);
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


/* silently ignore superfluous options */
static void
remove_extra_options ()
{
    const gchar *p;

    DBG("OPTS: removing superfluous options %s",
                methods[profile.method][METH_STR_FORB]);
    for (p = methods[profile.method][METH_STR_FORB]; *p != '\0'; p++)
    {
        switch (*p)
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

            case 'N':
                profile.num_page = -1;
                profile.last_page = -1;
                profile.limit = -1;
                break;

            case 'L':
                profile.lang = -1;
                break;

            case 'V':
                profile.autocorrect = -1;
                break;
        }
    }
}


/* check all mandatory options are present
 * return TRUE on success*/
static gboolean
check_mandatory_options ()
{
    const gchar *p;
    gboolean retval = TRUE;

    DBG("OPTS: checking mandatory options %s present",
                methods[profile.method][METH_STR_MAND]);
    for (p = methods[profile.method][METH_STR_MAND]; *p != '\0'; p++)
    {
        switch (*p)
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

    return retval;
}


/* parse & validate command line options
 * return TRUE on success */
static gboolean
load_options (int *argc, char ***argv)
{
    gchar *method = NULL, *period = NULL, *input_file = NULL, *tagtype = NULL;
    gchar *img_size = NULL, *page_str = NULL, *lang = NULL, *autocorrect = NULL;
    gboolean retval, mlist = FALSE, plist = FALSE, ilist = FALSE, llist = FALSE;
    gint i;
    gint starts = -1, ends = -1, limit = -1;
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
        { "lang",     'L',    0, G_OPTION_ARG_STRING, &lang,
            "for language code LL", "LL" },
        { "autocorrect",'A',  0, G_OPTION_ARG_STRING, &autocorrect,
            "set autocorrect to state V [0|1]", "V" },
        { "limit",    'l',    0, G_OPTION_ARG_INT, &limit,
            "fetch L items per page", "L" },
        { "page-num", 'n',    0, G_OPTION_ARG_STRING, &page_str,
            "fetch page number N or range N1-N2", "N|N1-N2" },
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
        { "wiki-full", 'w', 0, G_OPTION_ARG_NONE, &profile.wiki_full,
            "print full wiki text (default: summary only)", NULL },
        { "quiet", 'q', 0, G_OPTION_ARG_NONE, &profile.quiet,
            "don't print output after saving xml file", NULL },
        { "list-methods",  0, 0, G_OPTION_ARG_NONE, &mlist,
            "list supported methods, then exit", NULL },
        { "list-periods",  0, 0, G_OPTION_ARG_NONE, &plist,
            "list short & long period strings, then exit", NULL },
        { "list-image-sizes",  0, 0, G_OPTION_ARG_NONE, &ilist,
            "list image size names", NULL },
        { "list-langs",  0, 0, G_OPTION_ARG_NONE, &llist,
            "list supported language codes", NULL },
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
    if (llist)
    {
        printf ("\nLanguage codes:\n");
        for (i = 0; languages[i] != 0; i++)
        {
            printf ("    %s\n", languages[i]);
        }
    }
    if (mlist || plist || ilist || llist) exit (0);

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

    if (get_page_range (page_str))
    {
        ERR("OPTS: invalid page number/range");

        retval = FALSE;
    }
    DBG("OPTS: requested page range %d->%d", profile.num_page, profile.last_page);

    /* check API options */
    if (profile.from_file)
    {
        if (profile.last_page < 1) profile.num_page = -1;

        profile.quiet = FALSE;
    }
    else
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
            else if (profile.xml_file == NULL)
            {
                profile.quiet = FALSE;
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

        /* process language */
        if (lang != NULL)
        {
            for (i = 0; languages[i] != NULL; i++)
            {
                if (g_ascii_strcasecmp (lang, languages[i]) == 0)
                {
                    profile.lang = i;
                    break;
                }
            }
            if (profile.lang < 0)
            {
                ERR("OPTS: invalid language code");
            }
        }

        /* process autocorrect */
        if (autocorrect != NULL)
        {
            for (i = 0; autocorrects[i] != NULL; i++)
            {
                if (g_ascii_strcasecmp (autocorrect, autocorrects[i]) == 0)
                {
                    profile.autocorrect = i;
                    break;
                }
            }
            if (profile.autocorrect < 0)
            {
                ERR("OPTS: invalid autocorrect state");
            }
        }

        if (starts > 0) profile.starts = starts;
        if (ends > 0) profile.ends = ends;
        if (limit > 0) profile.limit = limit;

        /* skip method-related options if method was invalid */
        if (profile.method < 0) goto exit_opts;

        if (!check_mandatory_options ()) retval = FALSE;

        remove_extra_options ();
    }


exit_opts:
    if (check_xml_filename ())
    {
        retval = FALSE;

        ERR("if page range requested, filename must contain one format specification");
    }

    g_free (method);
    g_free (period);
    g_free (page_str);
    g_free (tagtype);
    g_free (lang);
    g_free (autocorrect);

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
    gchar *filename;
    gboolean retval;
    GError *error = NULL;

    if (profile.last_page > 0)
    {
        filename = g_strdup_printf (profile.xml_file, profile.num_page);

        DBG("FILE: loading file %s", filename);
        retval = g_file_get_contents (filename,
                                      &xml_buf.buffer,
                                      &xml_buf.windex,
                                      &error);
        g_free (filename);
    }
    else
    {
        DBG("FILE: loading file %s", profile.xml_file);
        retval = g_file_get_contents (profile.xml_file,
                                      &xml_buf.buffer,
                                      &xml_buf.windex,
                                      &error);
    }

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
    gchar *request, *filename;
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

    if (profile.lang >=0)
    {
        param_append_str (&request, "lang", languages[profile.lang]);
    }

    if (profile.autocorrect >=0)
    {
        param_append_str (&request, "autocorrect", autocorrects[profile.autocorrect]);
    }

    DBG("RQ: url = %s", request);
    //exit(0);

    /* reset position in buffer */
    xml_buf.windex = 0;

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

    /* save data in buffer if filename set
     * regardless of whether curl succeeded */
    if (profile.xml_file != NULL && xml_buf.buffer != NULL && xml_buf.windex > 0)
    {
        gboolean fail;

        if (profile.last_page > 0)
        {
            filename = g_strdup_printf (profile.xml_file, profile.num_page);

            fail = !g_file_set_contents (filename, xml_buf.buffer,
                                            xml_buf.windex, &error);

            g_free (filename);
        }
        else
        {
            fail = !g_file_set_contents (profile.xml_file, xml_buf.buffer,
                                            xml_buf.windex, &error);
        }

        if (fail)
        {
            ERR("RQ: failed to save response:\n     %s", error->message);

            g_error_free (error);
        }
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
    gboolean okay;

    okay = load_options (&argc, &argv);

    if (okay) load_config ();

    DBG("MAIN: okay:%d pages:%d->%d", okay, profile.num_page, profile.last_page);

    while (okay && (profile.last_page < 1 || profile.num_page <= profile.last_page))
    {
        DBG("MAIN: page:%d", profile.num_page);
        if (profile.from_file)
        {
            okay = load_xml_from_file ();
        }
        else
        {
            okay = make_request ();
        }

        if (okay && !profile.quiet)
        {
            okay = load_xml_doc ();
        }

        /* if page range not set we're done */
        if (profile.last_page < 1) break;

        profile.num_page++;

        /* crude rate-limiting if fetching multiple pages */
        if (!profile.from_file && profile.num_page <= profile.last_page)
        {
            usleep (200000);
        }
    }

    free_profile ();

    return !okay;
}
