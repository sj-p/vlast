#ifndef _VLAST_H_
#define _VLAST_H_

#include <glib.h>


typedef struct _VlastData
{
    gboolean    debug;
    gboolean    from_file;
    gboolean    wiki_full;
    gboolean    quiet;
    gboolean    sign_rq;
    gint        method;
    gint        period;
    gint        tagtype;
    gint        time1;
    gint        time2;
    gint        limit;
    gint        num_page;
    gint        last_page;
    gint        img_size;
    gint        lang;
    gint        autocorrect;
    gchar      *api_key;
    gchar      *api_secret;
    gchar      *token;
    gchar      *time_format;
    gchar      *xml_file;
    gchar      *config_file;
    gchar      *user;
    gchar      *artist;
    gchar      *album;
    gchar      *track;
    gchar      *album_artist;
    gchar      *tag;
    gchar      *country;
} VlastData;

typedef struct _VlastResults
{
    gint        page_num;
    gint        per_page;
    gint        total;
    gint        indent;
    gchar      *output;
} VlastResults;

typedef struct _VlastBuffer
{
    gchar      *buffer;
    gsize       bsize;
    gsize       windex;
} VlastBuffer;


enum
{
    METH_STR_API = 0,
    METH_STR_SHORT,
    METH_STR_MAND,
    METH_STR_FORB,
    METH_STR_TIME1,
    METH_STR_TIME2,
    METH_STR_TAG,
    NUM_METH_STR
};


enum
{
    PER_STR_API = 0,
    PER_STR_SHORT,
    NUM_PER_STR
};


#define ERR(...) {fprintf(stderr,"vlast ERROR: ");fprintf(stderr, __VA_ARGS__);fprintf(stderr,"\n");}
#define DBG(...) if(profile.debug){fprintf(stderr,"vlast: ");fprintf(stderr, __VA_ARGS__);fprintf(stderr,"\n");}


gboolean load_xml_doc ();
gboolean vlast_sk_save ();
gint index_image_size (const gchar *img_size);

#endif
