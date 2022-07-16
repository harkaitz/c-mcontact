#define _GNU_SOURCE
#include "mcontact.h"
#include <str/str2num.h>
#include <ftemplate.h>
#include <types/uuid_ss.h>
#include <mdb.h>
#include <uuid/uuid.h>
#include <errno.h>
#include <stdio-rec.h>
#ifdef NO_GETTEXT
#  define _(T) (T)
#else
#  include <libintl.h>
#  define _(T) dgettext("c-mcontact", T)
#endif
#ifndef NL
#  define NL "\n"
#endif

__attribute__((weak)) const char *MCONTACT_HTML_TAGS_TD_LABEL = "";
__attribute__((weak)) const char *MCONTACT_HTML_TAGS_TD_DATA  = "";

bool mcontact_validate (const mcontact *_c, const char **_reason) {
    if (_c->name[0]=='\0') {
        if (_reason) *_reason = _("Missing name");
        return false;
    } else if (_c->email.s[0] && !email_is_valid(&_c->email, _reason)) {
        return false;
    } else {
        return true;
    }
}

bool mcontact_db_delete(mdb *_db, const uuid_t _c_id) {
    return mdb_delete(_db, "mcontact", mdb_k_uuid(_c_id));
}

bool mcontact_db_insert(mdb *_db, mcontact *_c) {
    return mdb_insert(_db, "mcontact", mdb_k_uuid_new(_c->id), _c, sizeof(mcontact));
}

bool mcontact_db_replace(mdb *_db, const mcontact *_c) {
    mdb_replace(_db, "mcontact", mdb_k_uuid(_c->id), _c, sizeof(mcontact));
    return true;
}

bool mcontact_db_search(mdb *_db, const uuid_t _c_id, mcontact *_c, bool *_c_found) {
    return mdb_search_cp(_db, "mcontact", mdb_k_uuid(_c_id), _c, sizeof(mcontact), _c_found);
}

bool mcontact_db_iter_create(mdb *_db, mdb_iter **_iter) {
    return mdb_iter_create(_db, "mcontact", _iter);
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

void mcontact_db_iter_destroy(mdb_iter **_i) {
    mdb_iter_destroy(_i);
}

bool mcontact_ftemplate(FILE *_fp, const mcontact *_c, const char _k[], bool _escape) {
    switch(str2num(_k, strcasecmp,
                   "V:c_id"         ,  1,
                   "V:c_name"       ,  2,
                   "V:c_address"    ,  3,
                   "V:c_town"       ,  4,
                   "V:c_province"   ,  5,
                   "V:c_zipcode"    ,  6,
                   "V:c_email"      ,  7,
                   "V:c_description",  8,
                   "V:c_telephone"  ,  9,
                   "V:c_table_rows" , 10,
                   NULL)) {
    case  1: rec_fputs(uuid_str(_c->id, UUID_SS_STORE), _fp, _escape); return true;
    case  2: rec_fputs(_c->name     , _fp, _escape);         return true;
    case  3: rec_fputs(_c->address  , _fp, _escape);         return true;
    case  4: rec_fputs(_c->town     , _fp, _escape);         return true;
    case  5: rec_fputs(_c->province , _fp, _escape);         return true;
    case  6: rec_fputs(_c->zipcode  , _fp, _escape);         return true;
    case  7: rec_fputs(email_str(&_c->email), _fp, _escape); return true;
    case  8: rec_fputs(_c->description, _fp, _escape);       return true;
    case  9: rec_fputs(_c->telephone  , _fp, _escape);       return true;
    case 10:
        rec_fprintf(_fp, _escape,
                    "    <tr>"                NL
                    "      <td %s>"           NL
                    "        %s:"             NL
                    "      </td>"             NL
                    "      <td %s>"           NL
                    "        %s"              NL
                    "      </td>"             NL
                    "    </tr>"               NL
                    , MCONTACT_HTML_TAGS_TD_LABEL
                    , _("Name")
                    , MCONTACT_HTML_TAGS_TD_DATA
                    , _c->name);
        if (_c->email.s[0]) {
            rec_fprintf(_fp, _escape,
                        "<tr>"                NL
                        "  <td %s>"           NL
                        "    %s:"             NL
                        "  </td>"             NL
                        "  <td %s>"           NL
                        "    <a href=\"mailto:%s\">%s</a>"  NL
                        "  </td>"             NL
                        "</tr>"               NL
                        , MCONTACT_HTML_TAGS_TD_LABEL
                        , _("E-mail")
                        , MCONTACT_HTML_TAGS_TD_DATA
                        , _c->email.s
                        , _c->email.s);
        }
        if (_c->telephone[0]) {
            rec_fprintf(_fp, _escape,
                        "<tr>"                NL
                        "  <td %s>"           NL
                        "    %s:"             NL
                        "  </td>"             NL
                        "  <td %s>"           NL
                        "    <a href=\"tel:%s\">%s</a>" NL
                        "  </td>"             NL
                        "</tr>"               NL
                        , MCONTACT_HTML_TAGS_TD_LABEL
                        , _("Telephone")
                        , MCONTACT_HTML_TAGS_TD_DATA
                        , _c->telephone
                        , _c->telephone);
        }
        return true;
    default:
        return false;
    }
}

void mcontact_print(FILE *_fp1, const mcontact *_c, FILE *_tmpl, bool _escape) {
    char key[64];
    fseek(_tmpl, 0, SEEK_SET);
    while (ftemplate(_fp1, _tmpl, "{{", "}}", sizeof(key), key)) {
        if (mcontact_ftemplate(_fp1, _c, key, _escape)) continue;
        ftemplate_key(_fp1, key);
    }
}

bool mcontact_db_list(FILE *_fp1, mdb *_mdb, FILE *_tmpl, bool _escape) {
    bool           r               = false;
    mdb_iter      *mdb_iter        = NULL;
    uuid_t         contact_id      = {0};
    mcontact       contact         = {0};
    bool           contact_found   = false;
    int            e;
    e = mcontact_db_iter_create(_mdb, &mdb_iter);
    if (!e/*err*/) goto cleanup;
    while (mcontact_db_iter_loop(mdb_iter, contact_id)) {
        e = mcontact_db_search(_mdb, contact_id, &contact, &contact_found);
        if (!e/*err*/) goto cleanup;
        if (!contact_found) continue;
        mcontact_print(_fp1, &contact, _tmpl, _escape);
    }
    mdb_iter_destroy(&mdb_iter);
    r = true;
 cleanup:
    mdb_iter_destroy(&mdb_iter);
    return r;
}
