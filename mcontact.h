#ifndef MCONTACT_H
#define MCONTACT_H

#include <uuid/uuid.h>
#include <types/email.h>
#include <stdio.h>

typedef struct mdb      mdb;
typedef struct mdb_iter mdb_iter;

typedef struct __attribute__((__packed__)) mcontact {
    uuid_t id;
    char   name[64];
    char   address[256];
    char   town[64];
    char   province[64];
    char   zipcode[64];
    email  email;
    char   description[512];
} mcontact;

extern const char *g_mcontact_type;

bool mcontact_validate (const mcontact *_c);

/* DATABASE */
bool mcontact_db_open     (mdb *_db, const char *_opt_mode /* rwos */);
bool mcontact_db_delete   (mdb *_db, const uuid_t _c_id);
bool mcontact_db_insert   (mdb *_db, mcontact    *_c);
bool mcontact_db_replace  (mdb *_db, const mcontact *_c);
bool mcontact_db_search   (mdb *_db, const uuid_t _c_id, mcontact *_c, bool *_c_found);

/* ITERATOR */
bool mcontact_db_iter_create (mdb *_db, mdb_iter **_iter);
bool mcontact_db_iter_loop   (mdb_iter *_iter, uuid_t _c_id);
bool mdb_iter_destroy        (mdb_iter **_iter);

#endif
