#include "mcontact.h"
#include <str/str2num.h>
#include <str/compat.h>
#include <ftemplate.h>
#include <types/uuid_ss.h>
#include <mdb.h>
#include <uuid/uuid.h>
#include <errno.h>
#ifdef NO_GETTEXT
#  define _(T) (T)
#else
#  include <libintl.h>
#  define _(T) dgettext("c-mcontact", T)
#endif


__attribute__((weak)) const char *MCONTACT_DATABASE           = "mcontact";
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

/* --------------------------------------------------------------------------
 * ---- DATABASE ------------------------------------------------------------
 * -------------------------------------------------------------------------- */

bool mcontact_db_open (mdb *_db, const char *_opt_mode) {
    return mdb_open(_db, MCONTACT_DATABASE, _opt_mode, 0666);
}

bool mcontact_db_delete(mdb *_db, const uuid_t _c_id) {
    return mdb_auth_delete(_db, MCONTACT_DATABASE, mdb_k_uuid(_c_id));
}

bool mcontact_db_insert(mdb *_db, mcontact *_c) {
    mdb_k key = mdb_k_uuid_new(_c->id);
    return
        mdb_auth_insert_owner(_db, MCONTACT_DATABASE, key) &&
        mdb_insert(_db, MCONTACT_DATABASE, key, _c, sizeof(mcontact));
}

bool mcontact_db_replace(mdb *_db, const mcontact *_c) {
    mdb_k key = mdb_k_uuid(_c->id);
    return 
        mdb_auth_check_owner(_db, MCONTACT_DATABASE, key) &&
        mdb_replace(_db, MCONTACT_DATABASE, key, _c, sizeof(mcontact));
}

bool mcontact_db_search(mdb *_db, const uuid_t _c_id, mcontact *_c, bool *_c_found) {
    return mdb_search_cp(_db,
                         MCONTACT_DATABASE,
                         mdb_k_uuid(_c_id),
                         _c, sizeof(mcontact),
                         _c_found);
}

/* --------------------------------------------------------------------------
 * ---- ITERATOR ------------------------------------------------------------
 * -------------------------------------------------------------------------- */

bool mcontact_db_iter_create(mdb *_db, mdb_iter **_iter) {
    return mdb_auth_iter_create(_db, MCONTACT_DATABASE, _iter);
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

/* --------------------------------------------------------------------------
 * ---- FTEMPLATE -----------------------------------------------------------
 * -------------------------------------------------------------------------- */
bool mcontact_ftemplate(FILE *_fp, const mcontact *_c, const char _k[]) {
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
    case  1: fputs(uuid_str(_c->id, UUID_SS_STORE), _fp); return true;
    case  2: fputs(_c->name, _fp);              return true;
    case  3: fputs(_c->address, _fp);           return true;
    case  4: fputs(_c->town, _fp);              return true;
    case  5: fputs(_c->province, _fp);          return true;
    case  6: fputs(_c->zipcode, _fp);           return true;
    case  7: fputs(email_str(&_c->email), _fp); return true;
    case  8: fputs(_c->description, _fp);       return true;
    case  9: fputs(_c->telephone, _fp);         return true;
    case 10:
        fprintf(_fp,
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
            fprintf(_fp,
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
            fprintf(_fp,
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
    default: return false;
    }
}
void mcontact_print (FILE *_fp1, const mcontact *_c, FILE *_tmpl) {
    char key[64];
    fseek(_tmpl, 0, SEEK_SET);
    while (ftemplate(_fp1, _tmpl, "{{", "}}", sizeof(key), key)) {
        if (mcontact_ftemplate(_fp1, _c, key)) continue;
        ftemplate_key(_fp1, key);
    }
}
bool mcontact_db_list (FILE *_fp1, mdb *_mdb, FILE *_tmpl) {
    bool           r               = false;
    mdb_iter      *mdb_iter        = NULL;
    uuid_t         contact_id      = {0};
    mcontact       contact         = {0};
    bool           contact_found   = false;
    int            e;
    e = mcontact_db_open(_mdb, "r");
    if (!e/*err*/) goto cleanup;
    e = mcontact_db_iter_create(_mdb, &mdb_iter);
    if (!e/*err*/) goto cleanup;
    while (mcontact_db_iter_loop(mdb_iter, contact_id)) {
        e = mcontact_db_search(_mdb, contact_id, &contact, &contact_found);
        if (!e/*err*/) goto cleanup;
        if (!contact_found) continue;
        mcontact_print(_fp1, &contact, _tmpl);
    }
    e = mdb_iter_destroy(&mdb_iter);
    if (!e/*err*/) goto cleanup;
    r = true;
 cleanup:
    mdb_iter_destroy(&mdb_iter);
    return r;
}
