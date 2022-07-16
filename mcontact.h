#ifndef MCONTACT_H
#define MCONTACT_H

#include <uuid/uuid.h>
#include <types/email.h>
#include <types/username.h>
#include <str/sizes.h>
#include <stdio.h>

typedef struct mdb      mdb;
typedef struct mdb_iter mdb_iter;

typedef struct __attribute__((__packed__)) mcontact {
    uuid_t   id;
    str64    name;
    str256   address;
    str64    town;
    str64    province;
    str64    zipcode;
    email    email;
    str512   description;
    str32    telephone;
} mcontact;

__attribute__((weak)) extern const char *MCONTACT_HTML_TAGS_TD_LABEL;
__attribute__((weak)) extern const char *MCONTACT_HTML_TAGS_TD_DATA;

bool mcontact_validate (const mcontact *, const char **_reason);

/* DATABASE */
bool mcontact_db_delete   (mdb *, const uuid_t);
bool mcontact_db_insert   (mdb *, mcontact *);
bool mcontact_db_replace  (mdb *, const mcontact *);
bool mcontact_db_search   (mdb *, const uuid_t, mcontact *, bool *_found);

/* ITERATOR */
bool mcontact_db_iter_create (mdb *, mdb_iter **);
bool mcontact_db_iter_loop   (mdb_iter *, uuid_t);
void mcontact_db_iter_destroy(mdb_iter **);

/* FTEMPLATE */
bool mcontact_ftemplate (FILE *_fp1, const mcontact *_c, const char _k[], bool _escape);
void mcontact_print     (FILE *_fp1, const mcontact *_c, FILE *_tmpl, bool _escape);
bool mcontact_db_list   (FILE *_fp1, mdb *_mdb, FILE *_tmpl, bool _escape);

#endif
