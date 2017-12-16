#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <curl/curl.h>

#include "vlast.h"


VlastData profile = {FALSE, FALSE, FALSE, FALSE, FALSE,
                     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
VlastBuffer xml_buf = {NULL, 0, 0};

const gchar *tagtypes[] = {"artist", "album", "track", NULL};

const gchar *imgsizes[] = {"small", "medium", "large", "extralarge", "mega", NULL};
const gint num_imgsizes = sizeof (imgsizes) / sizeof (gchar*);

const gchar *languages[] = {"de", "en", "es", "fr", "it", "ja", "pl", "pt",
                            "ru", "sv", "tr", "zh", NULL};

const gchar *autocorrects[] = {"0", "1", NULL};


/* supported methods:
 *      api name, short api name, mandatory params, ignored params,
 *          'time1' param name, 'time2' param name, tag/tags */
const gchar *methods[][NUM_METH_STR] =
{
    {"auth.gettoken",            "au.gt","*",    "KUABRTPNLVY",NULL,NULL,NULL},
    {"auth.getsession",          "au.gs","*K",   "UABRTPNLVY", NULL,NULL,NULL},
    {"user.getinfo",             "u.gi", "U",    "KABRTPNLV",  NULL,NULL,NULL},
    {"user.getrecenttracks",     "u.grt","U",    "KABRTPLV",   "from","to",NULL},
    {"user.gettopartists",       "u.gta","U",    "KABRTLV",    NULL,NULL,NULL},
    {"user.gettopalbums",        "u.gtb","U",    "KABRTLV",    NULL,NULL,NULL},
    {"user.gettoptracks",        "u.gtt","U",    "KABRTLV",    NULL,NULL,NULL},
    {"user.gettoptags",          "u.gtg","U",    "KABRTPLV",   NULL,NULL,NULL},
    {"user.getlovedtracks",      "u.glt","U",    "KABRTPLV",   NULL,NULL,NULL},
    {"user.getfriends",          "u.gf", "U",    "KABRTPLV",   NULL,NULL,NULL},
    {"user.getweeklyartistchart","u.gac","U",    "KABRTPLV",   "from","to",NULL},
    {"user.getweeklyalbumchart", "u.gbc","U",    "KABRTPLV",   "from","to",NULL},
    {"user.getweeklytrackchart", "u.gtc","U",    "KABRTPLV",   "from","to",NULL},
    {"user.getweeklychartlist",  "u.gcl","U",    "KABRTPLV",   NULL,NULL,NULL},
    {"user.getartisttracks",     "u.gat","UA",   "KBRTL",      "starttimestamp","endtimestamp",NULL},
    {"user.getpersonaltags",     "u.gpg","UGY",  "KABRTPLV",   NULL,NULL,"tag"},
    {"library.getartists",       "l.ga", "U",    "KABRTPL",    NULL,NULL,NULL},
    {"artist.getinfo",           "a.gi", "A",    "KBRTPN",     NULL,NULL,NULL},
    {"artist.getcorrection",     "a.gc", "A",    "KUBRTPNLV",  NULL,NULL,NULL},
    {"artist.gettags",           "a.gg", "AU",   "KBRTPL",     NULL,NULL,NULL},
    {"artist.addtags",           "a.ag", "*UAG", "KBRTPL",     NULL,NULL,"tags"},
    {"artist.removetag",         "a.rg", "*UAG", "KBRTPL",     NULL,NULL,"tag"},
    {"artist.getsimilar",        "a.gs", "A",    "KUBRTPL",    NULL,NULL,NULL},
    {"artist.gettopalbums",      "a.gtb","A",    "KUBRTPL",    NULL,NULL,NULL},
    {"artist.gettoptracks",      "a.gtt","A",    "KUBRTPL",    NULL,NULL,NULL},
    {"artist.gettoptags",        "a.gtg","A",    "KUBRTPL",    NULL,NULL,NULL},
    {"artist.search",            "a.s",  "A",    "KUBRTPLV",   NULL,NULL,NULL},
    {"album.getinfo",            "b.gi", "BA",   "KRTPN",      NULL,NULL,NULL},
    {"album.gettags",            "b.gg", "ABU",  "KRTPL",      NULL,NULL,NULL},
    {"album.addtags",            "b.ag", "*UABG","KRTPL",      NULL,NULL,"tags"},
    {"album.removetag",          "b.rg", "*UABG","KRTPL",      NULL,NULL,"tag"},
    {"album.gettoptags",         "b.gtg","AB",   "KRUTPL",     NULL,NULL,NULL},
    {"album.search",             "b.s",  "B",    "KRUTPLV",    NULL,NULL,NULL},
    {"track.getinfo",            "t.gi", "TA",   "KBRPN",      NULL,NULL,NULL},
    {"track.getcorrection",      "t.gc", "TA",   "KUBRPNLV",   NULL,NULL,NULL},
    {"track.gettags",            "t.gg", "ATU",  "KBRPL",      NULL,NULL,NULL},
    {"track.addtags",            "t.ag", "*UATG","KBRPL",      NULL,NULL,"tags"},
    {"track.removetag",          "t.rg", "*UATG","KBRPL",      NULL,NULL,"tag"},
    {"track.getsimilar",         "t.gs", "TA",   "KUBRPL",     NULL,NULL,NULL},
    {"track.gettoptags",         "t.gtg","TA",   "KUBRPL",     NULL,NULL,NULL},
    {"track.search",             "t.s",  "T",    "KUBRPLV",    NULL,NULL,NULL},
    {"track.scrobble",           "t.scr","*UAT1","KPLVY",      "timestamp","duration",NULL},
    {"track.updatenowplaying",   "t.unp","*UAT", "KPLVY",      NULL,"duration",NULL},
    {"track.love",               "t.l",  "*UAT", "KBRPLVY",    NULL,NULL,NULL},
    {"track.unlove",             "t.u",  "*UAT", "KBRPLVY",    NULL,NULL,NULL},
    {"chart.gettopartists",      "c.ga", "",     "KUABRTPLV",  NULL,NULL,NULL},
    {"chart.gettoptracks",       "c.gt", "",     "KUABRTPLV",  NULL,NULL,NULL},
    {"chart.gettoptags",         "c.gg", "",     "KUABRTPLV",  NULL,NULL,NULL},
    {"geo.gettopartists",        "geo.a","C",    "KUABRTPLV",  NULL,NULL,NULL},
    {"geo.gettoptracks",         "geo.t","C",    "KUABRTPLV",  NULL,NULL,NULL},
    {"tag.getinfo",              "g.gi", "G",    "KUABRTPNV",  NULL,NULL,"tag"},
    {"tag.getsimilar",           "g.gs", "G",    "KUABRTPLV",  NULL,NULL,"tag"},
    {"tag.gettopartists",        "g.gta","G",    "KUABRTPLV",  NULL,NULL,"tag"},
    {"tag.gettopalbums",         "g.gtb","G",    "KUABRTPLV",  NULL,NULL,"tag"},
    {"tag.gettoptracks",         "g.gtt","G",    "KUABRTPLV",  NULL,NULL,"tag"},
    {"tag.gettoptags",           "g.gtg","",     "KUABRTPLV",  NULL,NULL,NULL},
    {"tag.getweeklychartlist",   "g.gcl","G",    "KUABRTPLV",  NULL,NULL,"tag"},
    {NULL,                       NULL,   NULL,   NULL,         NULL,NULL,NULL}
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


/* check taglist has 10 or fewer members
 * return TRUE on error */
static gboolean
check_taglist ()
{
    gint i;
    gchar *p;

    if (profile.tag == NULL ||
        methods[profile.method][METH_STR_TAG] == NULL ||
        strcmp (methods[profile.method][METH_STR_TAG], "tags") != 0) return FALSE;

    for (p = profile.tag, i = 0; *p != '\0'; p++)
    {
        if (*p == ',') i++;
    }

    return  (i > 9);
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


/* return TRUE on success */
gboolean
vlast_sk_save ()
{
    gboolean success;
    gchar *sk_dir, *sk_file;
    GError *error = NULL;

    sk_dir = g_build_filename (g_get_user_data_dir (), "vlast", "sk",
                                profile.api_key, NULL);
    if (g_mkdir_with_parents (sk_dir, 0700))
    {
        ERR("SK: failed to make dir '%s', can't save session key",
            sk_dir);

        g_free (sk_dir);

        return FALSE;
    }

    sk_file = g_build_filename (sk_dir, profile.user, NULL);
    if (g_file_set_contents (sk_file, profile.token, -1, &error))
    {
        (void) g_chmod (sk_file, 0600);

        success = TRUE;
    }
    else
    {
        ERR("SK: failed to save sk to '%s':\n%s", sk_file, error->message);

        g_error_free (error);

        success = FALSE;
    }

    g_free (sk_dir);
    g_free (sk_file);

    return success;
}


static void
load_config ()
{
    gchar *api_str, *img_size = NULL;
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

        api_str = g_key_file_get_string (kf, "Settings", "ApiKey", NULL);
        if (api_str != NULL)
        {
            profile.api_key = g_strdup (g_strstrip (api_str));
            g_free (api_str);
        }
        api_str = g_key_file_get_string (kf, "Settings", "ApiSecret", NULL);
        if (api_str != NULL)
        {
            profile.api_secret = g_strdup (g_strstrip (api_str));
            g_free (api_str);
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

    if (profile.api_key == NULL || profile.api_secret == NULL)
    {
        g_free (profile.api_key);
        g_free (profile.api_secret);

        profile.api_key =    g_strdup ("3bd2ead9afcba9077c7cb7a6927260e0");
        profile.api_secret = g_strdup ("856b0f4f1f825e9101e74027edc333d6");

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
            case 'K':
                g_free (profile.token);
                profile.token = NULL;
                break;

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

            case 'R':
                g_free (profile.album_artist);
                profile.album_artist = NULL;
                break;

            case 'P':
                profile.period = -1;
                break;

            case 'Y':
                profile.tagtype = -1;
                break;

            case '1':
                profile.time1 = -1;
                break;

            case '2':
                profile.time2 = -1;
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
            case '*':
                /* method requires signing */
                profile.sign_rq = TRUE;

                break;

            case 'K':
                if (profile.token == NULL)
                {
                    ERR("OPTS: need 'token' for this method");
                    retval = FALSE;
                }
                break;

            case 'U':
                if (profile.user == NULL)
                {
                    ERR("OPTS: need 'user' for this method");
                    retval = FALSE;
                }
                break;

            case 'A':
                if (profile.artist == NULL)
                {
                    ERR("OPTS: need 'artist' for this method");
                    retval = FALSE;
                }
                break;

            case 'B':
                if (profile.album == NULL)
                {
                    ERR("OPTS: need 'album' for this method");
                    retval = FALSE;
                }
                break;

            case 'T':
                if (profile.track == NULL)
                {
                    ERR("OPTS: need 'track' for this method");
                    retval = FALSE;
                }
                break;

            case 'G':
                if (profile.tag == NULL)
                {
                    ERR("OPTS: need 'tag' for this method");
                    retval = FALSE;
                }
                break;

            case 'C':
                if (profile.country == NULL)
                {
                    ERR("OPTS: need 'country' for this method");
                    retval = FALSE;
                }
                break;

            case 'Y':
                if (profile.tagtype < 0)
                {
                    ERR("OPTS: need 'tagtype' for this method");
                    retval = FALSE;
                }
                break;

            case '1':
                if (profile.time1 < 0)
                {
                    ERR("OPTS: need '%s' for this method",
                        (strcmp (methods[profile.method][METH_STR_TIME1], "timestamp") == 0
                        ? "timestamp" : "starts"));
                    retval = FALSE;
                }
                break;

            case '2':
                if (profile.time2 < 0)
                {
                    ERR("OPTS: need '%s' for this method",
                        (strcmp (methods[profile.method][METH_STR_TIME1], "duration") == 0
                        ? "duration" : "ends"));
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
    gint starts = -1, ends = -1, limit = -1, timestamp = -1, duration = -1;
    GError *error = NULL;
    GOptionContext *context;
    GOptionEntry entries[] = {
        { "method",   'm',    0, G_OPTION_ARG_STRING, &method,
            "use API method M", "M" },
        { "signed",   's',    0, G_OPTION_ARG_NONE, &profile.sign_rq,
            "force signed request", NULL },
        { "user",     'u',    0, G_OPTION_ARG_STRING, &profile.user,
            "for LastFM username U", "U" },
        { "artist",   'a',    0, G_OPTION_ARG_STRING, &profile.artist,
            "for artist A", "A" },
        { "album",    'b',    0, G_OPTION_ARG_STRING, &profile.album,
            "for album B", "B" },
        { "album-artist",'r',    0, G_OPTION_ARG_STRING, &profile.album_artist,
            "for album-artist R", "R" },
        { "track",    't',    0, G_OPTION_ARG_STRING, &profile.track,
            "for track T", "T" },
        { "tag",      'g',    0, G_OPTION_ARG_STRING, &profile.tag,
            "for tag G (taglist for methods *.addtags)", "G" },
        { "tagtype",  'y',    0, G_OPTION_ARG_STRING, &tagtype,
            "for tagging type Y [artist|album|track]", "Y" },
        { "country",    'c',    0, G_OPTION_ARG_STRING, &profile.country,
            "for ISO 3166-1 country name C (geo charts)", "C" },
        { "lang",     'L',    0, G_OPTION_ARG_STRING, &lang,
            "for language code LL", "LL" },
        { "autocorrect",'A',  0, G_OPTION_ARG_STRING, &autocorrect,
            "set autocorrect to state V [0|1]", "V" },
        { "token",    'k',    0, G_OPTION_ARG_STRING, &profile.token,
            "token K from auth.gettoken", "K" },
        { "timestamp",    0,     0, G_OPTION_ARG_INT, &timestamp,
            "at unix timestamp T", "T" },
        { "duration",    0,     0, G_OPTION_ARG_INT, &duration,
            "with duration D seconds", "D" },
        { "limit",    'l',    0, G_OPTION_ARG_INT, &limit,
            "fetch L items per page", "L" },
        { "page-num", 'n',    0, G_OPTION_ARG_STRING, &page_str,
            "fetch page number N or range N1-N2", "N|N1-N2" },
        { "period",   'p',    0, G_OPTION_ARG_STRING, &period,
            "fetch data for period P", "P" },
        { "starts",    0,     0, G_OPTION_ARG_INT, &starts,
            "from start time S", "S" },
        { "ends",    0,     0, G_OPTION_ARG_INT, &ends,
            "till end time E", "E" },
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

        /* skip method-related options if method was invalid */
        if (profile.method < 0) goto exit_opts;

        if (starts > 0 && methods[profile.method][METH_STR_TIME1] != NULL &&
            strcmp (methods[profile.method][METH_STR_TIME1], "timestamp") != 0)
        {
            profile.time1 = starts;
        }
        if (ends > 0 && methods[profile.method][METH_STR_TIME2] != NULL &&
            strcmp (methods[profile.method][METH_STR_TIME2], "duration") != 0)
        {
            profile.time2 = ends;
        }
        if (timestamp > 0 && methods[profile.method][METH_STR_TIME1] != NULL &&
            strcmp (methods[profile.method][METH_STR_TIME1], "timestamp") == 0)
        {
            profile.time1 = timestamp;
        }
        if (duration > 0 && methods[profile.method][METH_STR_TIME2] != NULL &&
            strcmp (methods[profile.method][METH_STR_TIME2], "duration") == 0)
        {
            profile.time2 = duration;
        }

        if (limit > 0) profile.limit = limit;

        if (check_taglist ())
        {
            ERR("too many items in tag list");

            retval = FALSE;
        }

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


/* adds strings pname & value to arrays */
static void
param_add_str (GPtrArray *params, GPtrArray *values,
               const gchar *pname, const gchar *value)
{
    DBG("URL+? add str %s=%s", pname, value);
    if (params == NULL || values == NULL || pname == NULL || value == NULL) return;

    g_ptr_array_add (params, (gpointer) pname);
    g_ptr_array_add (values, (gpointer) value);
}


/* converts int to string & adds pname & number string to arrays
 * returns number string to be freed when no longer needed */
static gchar *
param_add_int (GPtrArray *params, GPtrArray *values, const gchar *pname, gint value)
{
    gchar *str;

    DBG("URL+? add int %s=%d", pname, value);
    if (params == NULL || values == NULL || pname == NULL || value < 0) return NULL;

    str = g_strdup_printf ("%d", value);

    g_ptr_array_add (params, (gpointer) pname);
    g_ptr_array_add (values, (gpointer) str);

    return str;
}


static void
param_sort (GPtrArray *params, GPtrArray *values)
{
    gint i, j, d, n;
    gboolean f;

    if (params == NULL || values == NULL || params->len < 2) return;

    n = params->len;
    d = n;
    while (d > 1)
    {
        d /= 2;
        do
        {
            f = FALSE;
            for (i = 0, j = d; j < n; i++, j++)
            {
                if (strcmp ((char*)params->pdata[i], (char*)params->pdata[j]) > 0)
                {
                    gpointer t;

                    t = params->pdata[i];
                    params->pdata[i] = params->pdata[j];
                    params->pdata[j] = t;

                    t = values->pdata[i];
                    values->pdata[i] = values->pdata[j];
                    values->pdata[j] = t;

                    f = TRUE;
                }
            }
        } while (f);
    }
}


static void
sign_request (GPtrArray *params, GPtrArray *values)
{
    gint i;
    gchar *signature = NULL, *temp, *md5;

    param_sort (params, values);

    for (i = 0; i < params->len; i++)
    {
        if (i == 0)
        {
            signature = g_strconcat (params->pdata[0], values->pdata[0], NULL);
        }
        else
        {
            temp = g_strconcat (signature, params->pdata[i], values->pdata[i], NULL);
            g_free (signature);
            signature = temp;
        }
    }

    temp  = g_strconcat (signature, profile.api_secret, NULL);
    g_free (signature);
    signature = temp;

    md5 = g_compute_checksum_for_string (G_CHECKSUM_MD5, signature, -1);
    param_add_str (params, values, "api_sig", md5);
    g_free (signature);
}


/* return session key, or NULL on fail */
static gchar *
get_session_key ()
{
    gchar *sk_path, *sk_str;
    GError *error = NULL;

    if (profile.user == NULL) return NULL;

    sk_path = g_build_filename (g_get_user_data_dir (), "vlast", "sk",
                                profile.api_key, profile.user, NULL);
    DBG("sk path: %s", sk_path);

    if (!g_file_get_contents (sk_path, &sk_str, NULL, &error))
    {
        DBG("%s", error->message);

        g_error_free (error);

        sk_str = NULL;
    }

    g_free (sk_path);

    return sk_str;
}


static gchar *
build_url ()
{
    gint i;
    gchar *url, *sk = NULL;
    gchar *limit = NULL, *page = NULL, *time1 = NULL, *time2 = NULL;
    GPtrArray *paras, *values;

    if (profile.sign_rq && profile.user != NULL)
    {
        sk = get_session_key ();
        if (sk == NULL)
        {
            ERR("session key not found for user '%s' - reauthenticate?", profile.user);

            return NULL;
        }
    }

    paras = g_ptr_array_new ();
    values = g_ptr_array_new ();

    param_add_str (paras, values, "method", methods[profile.method][METH_STR_API]);
    param_add_str (paras, values, "api_key", profile.api_key);
    param_add_str (paras, values, "token", profile.token);
    param_add_str (paras, values, "sk", sk);

    if (!profile.sign_rq) param_add_str (paras, values, "user", profile.user);
    param_add_str (paras, values, "artist", profile.artist);
    param_add_str (paras, values, "album", profile.album);
    if (profile.artist != NULL && profile.album != NULL && profile.album_artist != NULL
        && strcmp (profile.artist, profile.album_artist) != 0)
    {
        param_add_str (paras, values, "albumArtist", profile.album_artist);
    }
    param_add_str (paras, values, "track", profile.track);
    param_add_str (paras, values, methods[profile.method][METH_STR_TAG], profile.tag);
    param_add_str (paras, values, "country", profile.country);

    if (profile.limit > 0)
    {
        limit = param_add_int (paras, values, "limit", profile.limit);
    }
    if (profile.num_page > 0)
    {
        page = param_add_int (paras, values, "page", profile.num_page);
    }
    if (profile.time1 > 0)
    {
        time1 = param_add_int (paras, values,
                               methods[profile.method][METH_STR_TIME1],
                               profile.time1);
    }
    if (profile.time2 > 0)
    {
        time2 = param_add_int (paras, values,
                               methods[profile.method][METH_STR_TIME2],
                               profile.time2);
    }

    if (profile.period >= 0)
    {
        param_add_str (paras, values, "period", periods[profile.period][PER_STR_API]);
    }
    if (profile.tagtype >= 0)
    {
        param_add_str (paras, values, "taggingtype", tagtypes[profile.tagtype]);
    }
    if (profile.lang >= 0)
    {
        param_add_str (paras, values, "lang", languages[profile.lang]);
    }
    if (profile.autocorrect >= 0)
    {
        param_add_str (paras, values, "autocorrect", autocorrects[profile.autocorrect]);
    }


    /* if making signed request, the parameters will need to be sorted
     * and then signature made before building request url */
    if (profile.sign_rq)
    {
        sign_request (paras, values);
    }

    /* build request url */
    url = g_strdup ("http://ws.audioscrobbler.com/2.0/");
    for (i = 0; i < paras->len; i++)
    {
        gchar *temp;
        gchar *escaped;

        escaped = g_uri_escape_string ((gchar*) g_ptr_array_index (values, i),
                                       NULL, FALSE);

        temp = g_strdup_printf ("%s%c%s=%s", url,
                                             (i == 0 ? '?' : '&'),
                                             (gchar*) g_ptr_array_index (paras, i),
                                             escaped);
        g_free (url);
        g_free (escaped);
        url = temp;
    }

    /* free stuff we no longer need */
    g_free (sk);
    g_free (limit);
    g_free (page);
    g_free (time1);
    g_free (time2);
    g_ptr_array_free (paras, TRUE);
    g_ptr_array_free (values, TRUE);

    return url;
}


static gboolean
make_request ()
{
    gchar *request, *filename;
    gboolean retval = TRUE;
    GError *error = NULL;
    CURL *curl;
    CURLcode res = CURLE_OK;

    request = build_url ();
    DBG("RQ: url = %s", request);
    if (request == NULL) return FALSE;
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
    g_free (profile.album_artist);
    g_free (profile.api_key);
    g_free (profile.api_secret);
    g_free (profile.artist);
    g_free (profile.country);
    g_free (profile.time_format);
    g_free (profile.tag);
    g_free (profile.token);
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
