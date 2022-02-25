#include "mcontact.h"
#include <io/findent.h>
#include <io/stdmsg.h>
#include <str/mtext.h>
#include <str/str2num.h>
#include <types/uuid_ss.h>
#include <mdb.h>
#include <uuid/uuid.h>
#include <syslog.h>
#include <errno.h>

const char *g_mcontact_type = "mcontact";

bool mcontact_validate (const mcontact *_c) {
    const char *reason;
    if (_c->name[0]=='\0') {
        syslog(LOG_ERR, "Please specify a `name=NAME`.");
        stdmsg("%s", mtext_get("Missing name."));
        return false;
    } else if (_c->email.s[0] && !email_is_valid(&_c->email, &reason)) {
        syslog(LOG_ERR, "Invalid email address: %s", _c->email.s);
        stdmsg(mtext_get("Invalid email address: %s"), reason);
        return false;
    } else {
        return true;
    }
}

/* --------------------------------------------------------------------------
 * ---- DATABASE ------------------------------------------------------------
 * -------------------------------------------------------------------------- */

bool mcontact_db_open (mdb *_db, const char *_opt_mode) {
    int res;
    res = mdb_open(_db, g_mcontact_type, (_opt_mode)?_opt_mode:"rws", 0666);
    if (!res/*err*/) return false;
    return true;
}

bool mcontact_db_delete(mdb *_db, const uuid_t _c_id) {
    int   res = 0;
    mdb_k key = mdb_k_uuid(_c_id);
    res = mdb_auth_delete(_db, g_mcontact_type, key);
    if (!res/*err*/) return false;
    return true;
}

bool mcontact_db_insert(mdb *_db, mcontact *_c) {
    int   res;
    mdb_k key = mdb_k_uuid_new(_c->id);
    res = mdb_auth_insert_owner(_db, g_mcontact_type, key);
    if (!res/*err*/) return false;
    res = mdb_insert(_db, g_mcontact_type, key, _c, sizeof(mcontact));
    if (!res/*err*/) return false;
    return true;
}

bool mcontact_db_replace(mdb *_db, const mcontact *_c) {
    int   res;
    mdb_k key = mdb_k_uuid(_c->id);
    res = mdb_auth_check_owner(_db, g_mcontact_type, key);
    if (!res/*err*/) return false;
    res = mdb_replace(_db, g_mcontact_type, key, _c, sizeof(mcontact));
    if (!res/*err*/) return false;
    return true;
}

bool mcontact_db_search(mdb *_db, const uuid_t _c_id, mcontact *_c, bool *_c_found) {
    mdb_k key = mdb_k_uuid(_c_id);
    return mdb_search_cp(_db, g_mcontact_type, key, _c, sizeof(mcontact), _c_found);
}

/* --------------------------------------------------------------------------
 * ---- ITERATOR ------------------------------------------------------------
 * -------------------------------------------------------------------------- */

bool mcontact_db_iter_create(mdb *_db, mdb_iter **_iter) {
    return mdb_auth_iter_create(_db, g_mcontact_type, _iter);
}

bool mcontact_db_iter_loop(mdb_iter *_iter, uuid_t _c_id) {
    mdb_k k;
    if (mdb_iter_loop(_iter, &k)) {
        uuid_copy(_c_id, (void*)k.dptr);
        return true;
    } else {
        return false;
    }
}



    
