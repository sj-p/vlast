#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "vlast.h"


extern VlastData profile;
extern VlastBuffer xml_buf;
extern const gchar *tagtypes[];
extern const gchar *methods[][NUM_METH_STR];
extern const gchar *periods[][NUM_PER_STR];


static gchar *
get_node_prop (xmlNode *node, gchar *tag)
{
    xmlChar *text;
    gchar *locale_str;

    text = xmlGetProp (node, (xmlChar*) tag);
    if (text == NULL) return NULL;

    locale_str = g_locale_from_utf8 ((gchar*) text, -1, NULL, NULL, NULL);

    xmlFree (text);

    return locale_str;
}


static gchar *
get_node_contents (xmlNode *node)
{
    xmlChar *text;
    gchar *locale_str;
    int i;

    if (!xmlNodeIsText (node->children)) return NULL;

    text = xmlNodeGetContent (node);
    if (text == NULL) return NULL;

    /* strip trailing whitespace */
    for (i = strlen ((char*) text); i >= 0; i--)
    {
        if (g_ascii_isspace (text[i]))
            text[i] = '\0';
        else
            break;
    }

    locale_str = g_locale_from_utf8 ((gchar*) text, -1, NULL, NULL, NULL);

    xmlFree (text);

    return locale_str;
}


/* get contents of first node matching tagname */
static gchar *
get_child_contents_by_tag (xmlNode *first_node, const gchar *tagname)
{
    xmlNode *node;

    for (node = first_node; node != NULL; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE &&
            strcmp ((char*)node->name, tagname) == 0)
        {
            return get_node_contents (node);
        }
    }

    return NULL;
}


/* return first element node with given name */
static xmlNode *
get_child_node_by_tag (xmlNode *first_node, const gchar *tagname)
{
    xmlNode *node;

    for (node = first_node; node != NULL; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE &&
            strcmp ((char*)node->name, tagname) == 0)
        {
            return node;
        }
    }

    return NULL;
}


static gchar**
get_sub_list (xmlNode *first_node, const gchar *top_name, const gchar *tag_name)
{
    xmlNode *node, *top_node;
    gchar *tag, **tagsv;
    GPtrArray *pa;

    top_node = get_child_node_by_tag (first_node, top_name);
    if (top_node == NULL) return NULL;

    pa = g_ptr_array_new ();

    for (node = top_node->children; node != NULL; node = node->next)
    {
        if (node->type != XML_ELEMENT_NODE ||
            strcmp ((char*)node->name, tag_name) != 0) continue;

        tag = get_child_contents_by_tag (node->children, "name");

        if (tag != NULL)
            g_ptr_array_add (pa, tag);
    }

    if (pa->len > 0)
    {
        g_ptr_array_add (pa, NULL);

        tagsv =  (gchar**) pa->pdata;

        g_ptr_array_free (pa, FALSE);
    }
    else
    {
        tagsv = NULL;

        g_ptr_array_free (pa, TRUE);
    }

    return tagsv;
}


static gchar *
format_time (const time_t secs)
{
    struct tm *lt;
    gchar buffer[100];

    if (secs <= 0 ||
        (lt = localtime (&secs)) == NULL ||
        strftime (buffer, 100, profile.time_format, lt) == 0)
    {
        return g_strdup ("n/a");
    }

    return g_strdup (buffer);
}


static gchar *
build_indent (VlastResults *results)
{
    gchar *indent_str = NULL;
    gint i;

    if (results == NULL) return NULL;

    for (i = 0; i < results->indent; i++)
    {
        gchar *tmp = g_strdup_printf ("%s      ",
                                      (indent_str == NULL ? "" : indent_str));
        g_free (indent_str);
        indent_str = tmp;
    }

    return indent_str;
}


static void
add_leader (VlastResults *results, const gchar *title, gint count)
{
    gchar *leader;
    gchar *indent_str = NULL;

    if (results == NULL) return;

    indent_str = build_indent (results);

    if (count <= 0)
    {   /* show title without count */
        leader = g_strdup_printf ("%s\n%s%s\n",
                                  (results->output == NULL ? "" : results->output),
                                  (indent_str == NULL ? "" : indent_str),
                                  title);
    }
    else if (results->page_num > 0 && results->per_page > 0 && results->total > 0)
    {   /* show title with page count/total */
        leader = g_strdup_printf ("%s\n%s%s (%d/%d)\n",
                                  (results->output == NULL ? "" : results->output),
                                  (indent_str == NULL ? "" : indent_str),
                                  title,
                                  count + (results->page_num - 1) * results->per_page,
                                  results->total);
    }
    else if (count > 0)
    {   /* show title with count */
        leader = g_strdup_printf ("%s\n%s%s (%d)\n",
                                  (results->output == NULL ? "" : results->output),
                                  (indent_str == NULL ? "" : indent_str),
                                  title, count);
    }

    g_free (results->output);
    g_free (indent_str);

    results->output = leader;
}


static void
add_output_string (VlastResults *results, const gchar *label, const gchar *str)
{
    gchar *temp;
    gchar *indent_str = NULL;

    if (results == NULL) return;

    indent_str = build_indent (results);

    temp = g_strdup_printf ("%s%s%12s%c %s\n",
                            (results->output == NULL ? "" : results->output),
                            (indent_str == NULL ? "" : indent_str),
                            label,
                            (*label=='\0'?' ':':'), str);

    g_free (indent_str);
    g_free (results->output);

    results->output = temp;
}


static void
add_output_int (VlastResults *results, const gchar *label, gint num)
{
    gchar *temp;
    gchar *indent_str = NULL;

    if (results == NULL) return;

    indent_str = build_indent (results);

    temp = g_strdup_printf ("%s%s%12s: %d\n",
                                    (results->output == NULL ? "" : results->output),
                                    (indent_str == NULL ? "" : indent_str),
                                    label, num);

    g_free (indent_str);
    g_free (results->output);
    results->output = temp;
}


static void
add_output_time (VlastResults *results, const gchar *label, time_t secs, gboolean add_uts)
{
    gchar *temp, *time_str, *uts_str;
    gchar *indent_str = NULL;

    if (results == NULL) return;

    indent_str = build_indent (results);

    time_str = format_time (secs);

    uts_str = (add_uts ? g_strdup_printf (" (%ld)", secs) : NULL);

    temp = g_strdup_printf ("%s%s%12s: %s%s\n",
                                    (results->output == NULL ? "" : results->output),
                                    (indent_str == NULL ? "" : indent_str),
                                    label, time_str,
                                    (add_uts ? uts_str : ""));

    g_free (time_str);
    g_free (uts_str);
    g_free (results->output);
    results->output = temp;
}


static gboolean
add_output_str_from_tag (VlastResults *results, xmlNode *first, const gchar *tagname,
                         const gchar *label)
{
    gchar *text;

    text = get_child_contents_by_tag (first, tagname);
    if (text != NULL)
    {
        add_output_string (results, label, text);

        g_free (text);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


static gboolean
add_output_date_from_tag (VlastResults *results, xmlNode *first, const gchar *tagname,
                          const gchar *label)
{
    gchar *text;
    xmlNode *node;
    time_t t;

    node = get_child_node_by_tag (first, tagname);
    if (node == NULL) return FALSE;

    if (strcmp (tagname, "timestamp") == 0)
    {
        text = get_node_contents (node);
    }
    else
    {
        text = get_node_prop (node, "uts");
        if (text == NULL) text = get_node_prop (node, "unixtime");
    }
    if (text == NULL) return FALSE;

    t = atol (text);

    add_output_time (results, label, t, FALSE);

    g_free (text);

    return TRUE;
}


static void
add_output_int_from_tag (VlastResults *results, xmlNode *first, const gchar *tagname,
                         const gchar *label, gboolean always_add)
{
    gchar *text;
    gint num;

    text = get_child_contents_by_tag (first, tagname);
    if (text != NULL)
    {
        num = MAX(0, atol (text));
        if (always_add || num > 0) add_output_int (results, label, num);

        g_free (text);
    }
}


static void
add_output_bool_from_tag (VlastResults *results, xmlNode *first, const gchar *tagname,
                          const gchar *label, gchar *test_str, gboolean always_add)
{
    gchar *text;
    gboolean test;

    text = get_child_contents_by_tag (first, tagname);
    if (text != NULL)
    {
        test = (strcmp (text, test_str) == 0);

        if (always_add || test)
            add_output_string (results, label, (test ? "yes" : "no"));

        g_free (text);
    }
}


static void
add_output_dur_from_tag (VlastResults *results, xmlNode *first, const gchar *tagname,
                         const gchar *label, gboolean always_add)
{
    gchar *text;
    gint num;

    text = get_child_contents_by_tag (first, tagname);
    if (text != NULL)
    {
        num = MAX(0, atol (text));
        if (num > 30000) num /= 1000;

        if (always_add || num > 0)
        {
            gint mins, secs;
            gchar *duration;

            mins = num / 60;
            secs = num - mins * 60;

            duration = g_strdup_printf ("%d:%02d", mins, secs);

            add_output_string (results, label, duration);

            g_free (duration);
        }

        g_free (text);
    }
}


static gboolean
proc_artist_info (xmlNode *first_node, VlastResults *results, gint count)
{
    gboolean retval = FALSE;
    gchar **strs, **str;
    gint i = 0;
    xmlNode *node, *stat_node;

    add_leader (results, "artist", count);

    add_output_str_from_tag (results, first_node, "name", "name");

    add_output_bool_from_tag (results, first_node,
                              "ontour", "on tour", "1", FALSE);

    node = get_child_node_by_tag (first_node, "stats");
    stat_node = (node == NULL ? first_node : node->children);

    add_output_int_from_tag (results, stat_node, "listeners", "listeners", FALSE);
    add_output_int_from_tag (results, stat_node, "playcount", "plays", FALSE);

    add_output_str_from_tag (results, first_node, "match", "match");

    strs = get_sub_list (first_node, "similar", "artist");
    if (strs != NULL)
    {
        for (str = strs, i = 0; *str != NULL; str++, i++)
        {
             add_output_string (results, (i==0?"similar":""), *str);
        }
        g_strfreev (strs);
    }

    strs = get_sub_list (first_node, "tags", "tag");
    if (strs != NULL)
    {
        for (str = strs, i = 0; *str != NULL; str++, i++)
        {
             add_output_string (results, (i==0?"tags":""), *str);
        }
        g_strfreev (strs);
    }

    return retval;
}


static gboolean
proc_artists (xmlNode *first_node, VlastResults *results)
{
    gboolean retval = FALSE;
    xmlNode *node;
    gint i = 0;

    for (node = first_node; node != NULL; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE &&
            strcmp ((char*)node->name, "artist") == 0)
        {
            i++;

            proc_artist_info (node->children, results, i);

            retval = TRUE;
        }
    }

    if (i == 0)
    {
        add_output_string (results, "", "List of artists was empty");
    }

    return retval;
}


static gboolean
proc_track_info (xmlNode *first_node, VlastResults *results, gint count)
{
    gboolean retval = FALSE;
    gchar **tags, **tag;
    xmlNode *node;

    add_leader (results, "track", count);

    add_output_str_from_tag (results, first_node, "name", "track");

    if (!add_output_str_from_tag (results, first_node, "artist", "artist"))
    {
        node = get_child_node_by_tag (first_node, "artist");
        if (node != NULL)
            add_output_str_from_tag (results, node->children, "name", "artist");
    }

    if (!add_output_str_from_tag (results, first_node, "album", "album"))
    {
        node = get_child_node_by_tag (first_node, "album");
        if (node != NULL)
            add_output_str_from_tag (results, node->children, "title", "album");
    }

    add_output_date_from_tag (results, first_node, "date", "time");

    add_output_dur_from_tag (results, first_node, "duration",
                             "duration", FALSE);

    add_output_int_from_tag (results, first_node, "listeners",
                             "listeners", FALSE);
    add_output_int_from_tag (results, first_node, "playcount",
                             "plays", FALSE);

    add_output_int_from_tag (results, first_node, "userplaycount",
                             "user plays", FALSE);

    add_output_bool_from_tag (results, first_node, "userloved",
                              "loved", "1", FALSE);

    add_output_str_from_tag (results, first_node, "match", "match");

    tags = get_sub_list (first_node, "toptags", "tag");
    if (tags != NULL)
    {
        gint i;

        for (tag = tags, i = 0; *tag != NULL; tag++, i++)
            add_output_string (results, (i==0?"tags":""), *tag);

        g_strfreev (tags);
    }

    return retval;
}


static gboolean
proc_tracks (xmlNode *first_node, VlastResults *results)
{
    gboolean retval = FALSE;
    xmlNode *node;
    gint i = 0;

    for (node = first_node; node != NULL; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE &&
            strcmp ((char*)node->name, "track") == 0)
        {
            i++;

            proc_track_info (node->children, results, i);

            retval = TRUE;
        }
    }

    if (i == 0)
    {
        add_output_string (results, "", "List of tracks was empty");
    }

    return retval;
}


static gboolean
proc_album_info (xmlNode *first_node, VlastResults *results, gint count)
{
    xmlNode *tracks;

    add_leader (results, "album", count);

    if (!add_output_str_from_tag (results, first_node, "artist", "artist"))
    {
        xmlNode *a_node;

        a_node = get_child_node_by_tag (first_node, "artist");
        add_output_str_from_tag (results, a_node->children, "name", "artist");
    }

    add_output_str_from_tag (results, first_node, "name", "album");

    add_output_int_from_tag (results, first_node, "listeners",
                             "listeners", FALSE);

    add_output_int_from_tag (results, first_node, "playcount",
                             "plays", FALSE);

    add_output_int_from_tag (results, first_node, "userplaycount",
                             "user plays", FALSE);

    tracks = get_child_node_by_tag (first_node, "tracks");
    if (tracks != NULL) proc_tracks (tracks->children, results);

    return TRUE;
}


static gboolean
proc_albums (xmlNode *first_node, VlastResults *results)
{
    gboolean retval = FALSE;
    xmlNode *node;
    gint i = 0;

    for (node = first_node; node != NULL; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE &&
            strcmp ((char*)node->name, "album") == 0)
        {
            i++;

            proc_album_info (node->children, results, i);

            retval = TRUE;
        }
    }

    if (i == 0)
    {
        add_output_string (results, "", "List of albums was empty");
    }

    return retval;
}


static gboolean
proc_user_info (xmlNode *first_node, VlastResults *results, gint count)
{
    add_leader (results, "user", count);

    add_output_str_from_tag (results, first_node, "name", "username");

    add_output_str_from_tag (results, first_node, "type", "type");

    add_output_bool_from_tag (results, first_node, "subscriber",
                              "subscriber", "1", FALSE);

    add_output_date_from_tag (results, first_node, "registered", "since");

    add_output_str_from_tag (results, first_node, "realname", "real name");

    add_output_str_from_tag (results, first_node, "country", "country");

    add_output_int_from_tag (results, first_node, "age", "age", FALSE);

    add_output_int_from_tag (results, first_node, "playcount", "total plays", FALSE);

    return TRUE;
}


static gboolean
proc_users (xmlNode *first_node, VlastResults *results)
{
    gboolean retval = FALSE;
    xmlNode *node;
    gint i = 0;

    for (node = first_node; node != NULL; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE &&
            strcmp ((char*)node->name, "user") == 0)
        {
            i++;

            (void) proc_user_info (node->children, results, i);
        }
    }

    if (i == 0)
    {
        add_output_string (results, "", "List of users was empty");
    }

    return retval;
}


static gboolean
proc_tag_info (xmlNode *first_node, VlastResults *results, gint count)
{
    add_leader (results, "tag", count);

    add_output_str_from_tag (results, first_node, "name", "name");

    add_output_int_from_tag (results, first_node, "count", "count", FALSE);

    add_output_int_from_tag (results, first_node, "reach", "taggers", FALSE);

    add_output_int_from_tag (results, first_node, "total", "times used", FALSE);

    return TRUE;
}


static gboolean
proc_tags (xmlNode *first_node, VlastResults *results)
{
    gboolean retval = FALSE;
    xmlNode *node;
    gint i = 0;

    for (node = first_node; node != NULL; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE &&
            strcmp ((char*)node->name, "tag") == 0)
        {
            i++;

            proc_tag_info (node->children, results, i);

            retval = TRUE;
        }
    }

    if (i == 0)
    {
        add_output_string (results, "", "List of tags was empty");
    }

    return retval;
}


static gboolean
proc_chart_list (xmlNode *first_node, VlastResults *results, gint count)
{
    gboolean retval = FALSE;
    xmlNode *node;
    gint i = 0;

    add_leader (results, "chart list", count);

    for (node = first_node; node != NULL; node = node->next)
    {
        gchar *from, *to;
        time_t uts_f, uts_t;

        if (node->type == XML_ELEMENT_NODE &&
            strcmp ((char*)node->name, "chart") == 0)
        {
            i++;

            from = get_node_prop (node, "from");
            to = get_node_prop (node, "to");

            if (from != NULL && to != NULL)
            {
                uts_f = atol (from);
                uts_t = atol (to);

                add_output_time (results, "from", uts_f, TRUE);
                add_output_time (results, "to", uts_t, TRUE);

                add_output_string (results, "", "");
            }

            g_free (from);
            g_free (to);
        }
    }

    if (i == 0)
    {
        add_output_string (results, "", "Chart list was empty");
    }

    return retval;
}


static gboolean
proc_correction_info (xmlNode *first_node, VlastResults *results, gint count)
{
    gboolean retval = FALSE;
    xmlNode *node;
    gchar *corr;

    add_leader (results, "correction", count);

    corr = get_node_prop (first_node->parent, "artistcorrected");

    if (corr != NULL && strcmp (corr, "1") == 0)
        add_output_string (results, "", "artist corrected");

    g_free (corr);

    corr = get_node_prop (first_node->parent, "trackcorrected");

    if (corr != NULL && strcmp (corr, "1") == 0)
        add_output_string (results, "", "track corrected");

    g_free (corr);

    ++results->indent;
    for (node = first_node; node != NULL; node = node->next)
    {
        if (node->type != XML_ELEMENT_NODE) continue;

        if (strcmp ((char*) node->name, "artist") == 0)
            proc_artist_info (node->children, results, 0);
        else if (strcmp ((char*) node->name, "track") == 0)
            proc_track_info (node->children, results, 0);

    }
    --results->indent;

    return retval;
}


static gboolean
proc_corrections (xmlNode *first_node, VlastResults *results)
{
    gboolean retval = FALSE;
    xmlNode *node;
    gint i = 0;

    for (node = first_node; node != NULL; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE &&
            strcmp ((char*)node->name, "correction") == 0)
        {
            i++;

            proc_correction_info (node->children, results, i);

            retval = TRUE;
        }
    }

    if (i == 0)
    {
        add_output_string (results, "", "List of corrections was empty");
    }

    return retval;
}


static gboolean
proc_taggings (xmlNode *first_node, VlastResults *results)
{
    gboolean retval = FALSE;
    xmlNode *node;

    add_leader (results, "taggings", 0);

    ++results->indent;
    for (node = first_node; node != NULL; node = node->next)
    {
        if (node->type != XML_ELEMENT_NODE) continue;

        if (strcmp ((char*)node->name, "artists") == 0)
            proc_artists (node->children, results);
        else if (strcmp ((char*)node->name, "albums") == 0)
            proc_albums (node->children, results);
        else if (strcmp ((char*)node->name, "tracks") == 0)
            proc_tracks (node->children, results);
    }
    --results->indent;

    return retval;
}


static gboolean
proc_search_results (xmlNode *first_node, VlastResults *results)
{
    gboolean retval = FALSE;
    xmlNode *node;

    add_leader (results, "search results", 0);

    ++results->indent;
    for (node = first_node; node != NULL; node = node->next)
    {
        if (node->type != XML_ELEMENT_NODE) continue;

        if (strcmp ((char*)node->name, "artistmatches") == 0)
            proc_artists (node->children, results);
        else if (strcmp ((char*)node->name, "albummatches") == 0)
            proc_albums (node->children, results);
        else if (strcmp ((char*)node->name, "trackmatches") == 0)
            proc_tracks (node->children, results);
    }
    --results->indent;

    return retval;
}


static void
get_coordinates (xmlNode *node, VlastResults *results)
{
    gchar *text;

    text = get_node_prop (node, "page");
    if (text != NULL) results->page_num = atol (text);
    g_free (text);

    text = get_node_prop (node, "perPage");
    if (text != NULL) results->per_page = atol (text);
    g_free (text);

    text = get_node_prop (node, "total");
    if (text != NULL) results->total = atol (text);
    g_free (text);
}


static gboolean
proc_method (xmlNode *first_node)
{
    gboolean retval = FALSE;
    const gchar *method = NULL;
    xmlNode *node;
    VlastResults *results;


    if (!profile.from_file) method = methods[profile.method][METH_STR_XML];

    for (node = first_node; node != NULL; node = node->next)
    {
        if (node->type != XML_ELEMENT_NODE) continue;

        if (profile.from_file)
        {
            gint i;

            for (i = 0; methods[i][METH_STR_API] != NULL; i++)
            {
                if (g_ascii_strcasecmp ((gchar*) node->name,
                                        methods[i][METH_STR_XML]) == 0)
                {
                    profile.method = i;

                    break;
                }
            }

            if (profile.method < 0) continue;

            method = methods[profile.method][METH_STR_XML];
        }

        if (profile.from_file || strcmp ((char*) node->name, method) == 0)
        {
            DBG("P_METH: got xml method '%s'", method);

            results = g_new0 (VlastResults, 1);

            get_coordinates (node, results);

            if (g_str_has_suffix (method, "tracks") ||
                g_str_has_suffix (method, "trackchart"))
            {
                retval = proc_tracks (node->children, results);
            }
            else if (g_str_has_suffix (method, "artists") ||
                     g_str_has_suffix (method, "artistchart"))
            {
                retval = proc_artists (node->children, results);
            }
            else if (g_str_has_suffix (method, "albums") ||
                     g_str_has_suffix (method, "albumchart"))
            {
                retval = proc_albums (node->children, results);
            }
            else if (strcmp (method, "friends") == 0)
            {
                retval = proc_users (node->children, results);
            }
            else if (g_str_has_suffix (method, "tags"))
            {
                retval = proc_tags (node->children, results);
            }
            else if (strcmp (method, "corrections") == 0)
            {
                retval = proc_corrections (node->children, results);
            }
            else if (g_str_has_suffix (method, "artist"))
            {
                retval = proc_artist_info (node->children, results, 0);
            }
            else if (g_str_has_suffix (method, "album"))
            {
                retval = proc_album_info (node->children, results, 0);
            }
            else if (g_str_has_suffix (method, "track"))
            {
                retval = proc_track_info (node->children, results, 0);
            }
            else if (g_str_has_suffix (method, "tag"))
            {
                retval = proc_tag_info (node->children, results, 0);
            }
            else if (strcmp (method, "user") == 0)
            {
                retval = proc_user_info (node->children, results, 0);
            }
            else if (strcmp (method, "weeklychartlist") == 0)
            {
                retval = proc_chart_list (node->children, results, 0);
            }
            else if (strcmp (method, "taggings") == 0)
            {
                retval = proc_taggings (node->children, results);
            }
            else if (strcmp (method, "results") == 0)
            {
                retval = proc_search_results (node->children, results);
            }
            else
            {
                ERR("P_METH: xml method tag <%s> not recognised", method);
            }

            if (results->output != NULL)
            {
                fputs (results->output, stdout);

                g_free (results->output);
            }

            g_free (results);

            break;
        }
    }

    return retval;
}


static void
dump_xml ()
{
    if (profile.debug)
    {
        fprintf (stderr, "\n");
        fwrite (xml_buf.buffer, 1, xml_buf.windex, stderr);
        fprintf (stderr, "\n\n");
    }
}


static gboolean
proc_tree (xmlNode *top_node)
{
    xmlNode *node;
    gchar *text;
    gboolean retval = FALSE;

    if (strcmp ((char*) top_node->name, "lfm") != 0)
    {
        DBG("P_TREE: didn't get lfm tag");

        goto exit_dump;
    }

    text = get_node_prop (top_node, "status");
    if (text == NULL)
    {
        DBG("P_TREE: didn't get lfm status");

        goto exit_dump;
    }
    if (strcmp (text, "ok") == 0)
    {
        g_free (text);

        retval = proc_method (top_node->children);

        return retval;
    }
    g_free (text);

    /* failed, so get error code & message */
    node = get_child_node_by_tag (top_node->children, "error");
    if (node == NULL)
    {
        DBG("P_TREE: didn't get error tag");

        goto exit_dump;
    }

    fprintf (stderr, "\nLastFM reports fail: ");

    text = get_node_prop (node, "code");
    if (text != NULL)
    {
        fprintf (stderr, "code %s ", text);

        g_free (text);
    }

    text = get_node_contents (node);
    if (text != NULL)
    {
        fprintf (stderr, "'%s'", text);

        g_free (text);
    }

    fprintf (stderr, "\n\n");

    return FALSE;

exit_dump:
    dump_xml ();

    return FALSE;
}

gboolean
load_xml_doc ()
{
    gboolean retval = TRUE;
    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;

    LIBXML_TEST_VERSION

    /* parse the xml in buffer */
    doc = xmlReadMemory (xml_buf.buffer, xml_buf.windex, "response.xml", NULL, 0);
    if (doc == NULL)
    {
        ERR("XML: failed to parse the data");

        dump_xml ();

        retval = FALSE;

        goto exit_xml;
    }
    DBG("XML: parsed xml data");

    /* get the root element node */
    root_element = xmlDocGetRootElement (doc);
    if (root_element == NULL)
    {
        ERR("XML: failed to find root element");

        dump_xml ();

        retval = FALSE;
    }
    else
    {
        retval = proc_tree (root_element);
    }

exit_xml:
    if (doc != NULL) xmlFreeDoc (doc);

    xmlCleanupParser ();

    return retval;
}

