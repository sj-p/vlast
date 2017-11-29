#ifndef _VLAST_H_
#define _VLAST_H_

#include <glib.h>


typedef struct _VlastData
{
    gboolean    debug;
    gboolean    from_file;
    gint        method;
    gint        period;
    gint        tagtype;
    gint        starts;
    gint        ends;
    gint        limit;
    gint        num_page;
    gint        img_size;
    gchar      *api_key;
    gchar      *time_format;
    gchar      *xml_file;
    gchar      *config_file;
    gchar      *user;
    gchar      *artist;
    gchar      *album;
    gchar      *track;
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
    METH_STR_XML,
    METH_STR_MAND,
    METH_STR_FORB,
    METH_STR_START,
    METH_STR_END,
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
gint index_image_size (const gchar *img_size);


#endif
