#include "mcontact.h"
#include <unistd.h>
#include <libgen.h>
#include <sys/authorization.h>
#include <str/strarray.h>
#include <mdb.h>
#include <io/mconfig.h>
#include <io/findent.h>
#include <types/uuid_ss.h>
#include <str/str2num.h>

#define NL "\n"
#define COPYRIGHT_LINE \
    "Bug reports, feature requests to gemini|https://harkadev.com/oss" "\n" \
    "Copyright (c) 2022 Harkaitz Agirre, harkaitz.aguirre@gmail.com" "\n" \
    ""


int  main_print_mcontact (mcontact *_c, FILE *_fp1);
bool main_edit_mcontact  (mcontact *_c, char *_opts[]);

static const char help[] =
    "Usage: %s ..."                                         NL
    ""                                                      NL
    "Manage contact databases written with `mcontact(3)`."  NL
    ""                                                      NL
    "Options: name=T address=T town=T province=T zipcode=T" NL
    "         email=E description=D"                        NL
    ""                                                      NL
    "    -a USER     : Select the contact sheet."           NL
    "    -t DATATYPE : Select the contact application."     NL
    ""                                                      NL
    "    -s [id=ID] [OPTS...] : Create/modify contact."     NL
    "    -l [NAME|@]          : List contacts."             NL
    "    -d ID...             : Delete contact."            NL
    ""                                                      NL
    COPYRIGHT_LINE
    ;

int main (int _argc, char *_argv[]) {
    int           opt,res;
    int           retval      = 1;
    char          cmd         = '\0';
    mdb          *db          = NULL;
    mconfig_t     mconfig     = MCONFIG_INITIALIZER();
    mcontact      mcontact    = {0};
    mdb_iter     *iter        = NULL;
    char         *opts[_argc*2];
    uuid_t        id;
    uuid_ss       id_s;
    char         *id_ss;
    char         *name;
    

    /* Print help and set logging. */
    _argv[0] = basename(_argv[0]);
    if (_argc == 1 || !strcmp(_argv[1], "--help") || !strcmp(_argv[1], "-h")) {
        printf(help, _argv[0]);
        return 0;
    }
    openlog(_argv[0], LOG_PERROR, LOG_USER);

    /* Parse command line arguments. */
    while((opt = getopt (_argc, _argv, "lsda:t:")) != -1) {
        switch (opt) {
        case 'l': case 's': case 'd': cmd = opt; break;
        case 'a': authorization_open(optarg); break;
        case 't': g_mcontact_type = optarg;   break;
        case '?':
        default:
            return 1;
        }
    }

    /* Load configuration. */
    res = mconfig_load(&mconfig, "/etc/mcontact.cfg", true);
    if (!res/*err*/) goto cleanup;
    res = mconfig_load(&mconfig, "/etc/mconfig.cfg", true);
    if (!res/*err*/) goto cleanup;
    
    /* Connect to database. */
    res = mdb_create(&db, NULL);
    if (!res/*err*/) goto cleanup;
    res = mcontact_db_open(db, NULL);
    if (!res/*err*/) goto cleanup;
    
    /* Commands. */
    switch(cmd) {
    case 's':
        streq2map(_argv+optind, _argc*2, opts);
        id_ss = strmap_get(opts, "id", strcasecmp);
        if (id_ss) {
            res = uuid_parse_nn(id_ss, id);
            if (!res/*err*/) goto cleanup;
            res = mcontact_db_search(db, id, &mcontact, NULL);
            if (!res/*err*/) goto cleanup;
        }
        res = main_edit_mcontact(&mcontact, opts);
        if (!res/*err*/) goto cleanup;
        res = mcontact_validate(&mcontact);
        if (!res/*err*/) goto cleanup;
        if (id_ss) {
            res = mcontact_db_replace(db, &mcontact);
        } else {
            res = mcontact_db_insert(db, &mcontact);
        }
        if (!res/*err*/) goto cleanup;
        break;
    case 'l':
        if (optind < _argc) {
            name = _argv[optind];
        } else {
            name = NULL;
        }
        res = mcontact_db_iter_create(db, &iter);
        if (!res/*err*/) goto cleanup;
        while (mcontact_db_iter_loop(iter, id)) {
            if (!name) {
                fprintf_i(stdout, "[%s]", uuid_str(id, &id_s));
                res = mcontact_db_search(db, id, &mcontact, NULL);
                if (!res/*err*/) goto cleanup;
                findent(stdout, 1);
                main_print_mcontact(&mcontact, stdout);
                findent(stdout, -1);
            } else if (!strcmp(name, "@")) {
                fprintf_i(stdout, "%s", uuid_str(id, &id_s));
            } else {
                res = mcontact_db_search(db, id, &mcontact, NULL);
                if (!res/*err*/) goto cleanup;
                if (strcasecmp(mcontact.name, name)) continue;
                fprintf_i(stdout, "%s", uuid_str(id, &id_s));
            }
        }
        res = mdb_iter_destroy(&iter);
        if (!res/*err*/) goto cleanup;
        break;
    case 'd':
        for (opt=optind; opt<_argc; opt++) {
            res = uuid_parse_nn(_argv[opt], id);
            if (!res/*err*/) goto cleanup;
            res = mcontact_db_delete(db, id);
            if (!res/*err*/) goto cleanup;
        }
        break;
    }
    /* Cleanup. */
    retval = 0;
    goto cleanup;
 cleanup:
    mdb_destroy(db);
    mdb_iter_destroy(&iter);
    return retval;
}

int main_print_mcontact(mcontact *_c, FILE *_fp1) {
    int ret = 0;
    if (_c->name[0])        ret += fprintf_i(_fp1, "name        : %s", _c->name);
    if (_c->address[0])     ret += fprintf_i(_fp1, "address     : %s", _c->address);
    if (_c->town[0])        ret += fprintf_i(_fp1, "town        : %s", _c->town);
    if (_c->province[0])    ret += fprintf_i(_fp1, "province    : %s", _c->province);
    if (_c->zipcode[0])     ret += fprintf_i(_fp1, "zipcode     : %s", _c->zipcode);
    if (_c->email.s[0])     ret += fprintf_i(_fp1, "email       : %s", _c->email);
    if (_c->description[0]) ret += fprintf_i(_fp1, "description : %s", _c->description);
    return ret;
}

bool main_edit_mcontact (mcontact *_c, char *_opts[]) {
    char **opt; int res;
    for (opt = _opts; _opts && *opt; opt+=2) {
        switch(str2num(*opt, strcasecmp,
                       "id"   , 1, "name"       , 2, "address" , 3,
                       "town" , 4, "province"   , 5, "zipcode" , 6,
                       "email", 7, "description", 8,
                       NULL)) {
        case 1: /* id */
            res = uuid_parse(*(opt+1), _c->id);
            if (res==-1/*err*/) goto cleanup_invalid_uuid;
            break;
        case 2: /* name */
            strncpy(_c->name, *(opt+1), sizeof(_c->name)-1);
            break;
        case 3: /* address */
            strncpy(_c->address, *(opt+1), sizeof(_c->address)-1);
            break;
        case 4: /* town */
            strncpy(_c->town, *(opt+1), sizeof(_c->town)-1);
            break;
        case 5: /* province */
            strncpy(_c->province, *(opt+1), sizeof(_c->province)-1);
            break;
        case 6: /* zipcode */
            strncpy(_c->zipcode, *(opt+1), sizeof(_c->zipcode)-1);
            break;
        case 7: /* email */
            res = email_parse(&_c->email, *(opt+1));
            if (!res/*err*/) goto cleanup;
            break;
        case 8: /* description */
            strncpy(_c->description, *(opt+1), sizeof(_c->description)-1);
            break;
        }
    }
    return true;
 cleanup_invalid_uuid:
    syslog(LOG_ERR, "Invalid UUID: %s", *(opt+1));
    return false;
 cleanup:
    return false;
}
