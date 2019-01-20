#ifndef _VLAST_H_
#define _VLAST_H_

#include <glib.h>


typedef struct _VlastData
{
    gboolean    debug;
    gboolean    dryrun;
    gboolean    from_file;
    gboolean    wiki_full;
    gboolean    quiet;
    gboolean    sign_rq;
    gboolean    show_mbids;
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
    gchar      *mbid;
} VlastData;

typedef struct _VlastResults
{
    gint        page_num;
    gint        per_page;
    gint        total;
    gchar      *output;
    gchar      *indent_mem;
    gchar      *indent_str;
} VlastResults;

typedef struct _VlastBuffer
{
    gchar      *buffer;
    gsize       bsize;
    gsize       windex;
} VlastBuffer;


enum
{
    VLAST_ERR_OK        = 0,
    VLAST_ERR_LASTFM    = 127,
    VLAST_ERR_OPTIONS   = 128,
    VLAST_ERR_LOAD_XML  = 129,
    VLAST_ERR_PARSE_XML = 130,
    VLAST_ERR_XML_DATA  = 131,
    VLAST_ERR_URL       = 132,
    VLAST_ERR_LIBCURL   = 133,
    VLAST_ERR_SAVE_XML  = 134,
    VLAST_ERR_HTTP      = 135,
    VLAST_ERR_HTTP_SAVE = 136,
    VLAST_ERR_SK        = 137
};


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


gint vlast_load_xml_doc ();
gboolean vlast_sk_save ();
gint vlast_index_image_size (const gchar *img_size);

#endif
