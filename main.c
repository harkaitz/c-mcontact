#include "mcontact.h"
#include <unistd.h>
#include <libgen.h>
#include <sys/authorization.h>
#include <str/strarray.h>
#include <mdb.h>
#include <io/slog.h>
#include <types/uuid_ss.h>

#define COPYRIGHT_LINE \
    "Bug reports, feature requests to gemini|https://harkadev.com/oss" "\n" \
    "Copyright (c) 2022 Harkaitz Agirre, harkaitz.aguirre@gmail.com" "\n" \
    ""


void mcontact_print_cli (mcontact *_c, const char *_prefix);
bool mcontact_edit  (mcontact *_c, char *_opts[]);

static const char help[] =
    "Usage: %s USER OPERATION [OPTIONS...]"                 "\n"
    ""                                                      "\n"
    "Manage contacts saved by user USER in mdb(3)."         "\n"
    ""                                                      "\n"
    "Operations:"                                           "\n"
    ""                                                      "\n"
    "    edit   [id=ID] [OPTS...] : Create/modify contact." "\n"
    "    list   [NAME|@]          : List contacts."         "\n"
    "    delete ID ...            : Delete contacts."       "\n"
    ""                                                      "\n"
    COPYRIGHT_LINE
    ;

int main (int _argc, char *_argv[]) {
    
    int       e;
    int       r         = 1;
    mdb      *mdb       = NULL;
    mcontact  mcontact  = {0};
    mdb_iter *iter      = NULL;
    uuid_t    id        = {0};
    char     *pname     = basename(_argv[0]);
    char     *username  = (_argc>1) ?_argv[1]:NULL;
    char     *command   = (_argc>2) ?_argv[2]:NULL;
    char     *argument1 = (_argc>3) ?_argv[3]:NULL;
    char    **arguments = (_argc>=3)?_argv+3:NULL;
    
    /* Print help. */
    if (!username ||
        !command  ||
        !strcmp(username, "--help") ||
        !strcmp(username, "-h")) {
        printf(help, _argv[0]);
        return 0;
    }

    /* Start logging. */
    openlog(pname, LOG_PERROR, LOG_USER);

    /* Select user. */
    authorization_open(username);
    
    /* Connect to database. */
    e = mdb_create(&mdb, NULL);
    if (!e/*err*/) goto cleanup;
    e = mcontact_db_open(mdb, "rw");
    if (!e/*err*/) goto cleanup;
    /* Commands. */
    if (!strcmp(command, "edit")) {
        char *opts[_argc*2];
        streq2map(arguments, _argc*2, opts);
        char *id_ss = strmap_get(opts, "id", strcasecmp);
        if (id_ss) {
            e = uuid_parse_secure(id, id_ss, false, NULL);
            if (!e/*err*/) goto cleanup;
            e = mcontact_db_search(mdb, id, &mcontact, NULL);
            if (!e/*err*/) goto cleanup;
        }
        e = mcontact_edit(&mcontact, opts);
        if (!e/*err*/) goto cleanup;
        e = mcontact_validate(&mcontact, NULL);
        if (!e/*err*/) goto cleanup;
        if (id_ss) {
            e = mcontact_db_replace(mdb, &mcontact);
            if (!e/*err*/) goto cleanup;
        } else {
            e = mcontact_db_insert(mdb, &mcontact);
            if (!e/*err*/) goto cleanup;
        }
    } else if (!strcmp(command, "list")) {
        char *name = argument1; uuid_ss id_s;
        e = mcontact_db_iter_create(mdb, &iter);
        if (!e/*err*/) goto cleanup;
        while (mcontact_db_iter_loop(iter, id)) {
            if (!name) {
                printf("[%s]\n", uuid_str(id, &id_s));
                e = mcontact_db_search(mdb, id, &mcontact, NULL);
                if (!e/*err*/) goto cleanup;
                mcontact_print_cli(&mcontact, "    ");
            } else if (!strcmp(name, "@")) {
                printf("%s\n", uuid_str(id, &id_s));
            } else {
                e = mcontact_db_search(mdb, id, &mcontact, NULL);
                if (!e/*err*/) goto cleanup;
                if (strcasecmp(mcontact.name, name)) continue;
                printf("%s\n", uuid_str(id, &id_s));
            }
        }
        e = mdb_iter_destroy(&iter);
        if (!e/*err*/) goto cleanup;
    } else if (!strcmp(command, "delete")) {
        for (char **o = arguments; *o; o++) {
            e = uuid_parse_secure(id, *o, false, NULL);
            if (!e/*err*/) goto cleanup;
            e = mcontact_db_delete(mdb, id);
            if (!e/*err*/) goto cleanup;
        }
    } else {
        error("Invalid subcommand: %s", command);
        goto cleanup;
    }
    
    /* Cleanup. */
    r = 0;
    goto cleanup;
 cleanup:
    mdb_destroy(mdb);
    mdb_iter_destroy(&iter);
    return r;
}

void mcontact_print_cli(mcontact *_c, const char *_prefix) {
    const char *p = (_prefix)?_prefix:"";
    if (_c->name[0])        printf("%s" "name        : %s\n", p, _c->name);
    if (_c->address[0])     printf("%s" "address     : %s\n", p, _c->address);
    if (_c->town[0])        printf("%s" "town        : %s\n", p, _c->town);
    if (_c->province[0])    printf("%s" "province    : %s\n", p, _c->province);
    if (_c->zipcode[0])     printf("%s" "zipcode     : %s\n", p, _c->zipcode);
    if (_c->email.s[0])     printf("%s" "email       : %s\n", p, _c->email.s);
    if (_c->telephone)      printf("%s" "telephone   : %s\n", p, _c->telephone);
    if (_c->description[0]) printf("%s" "description : %s\n", p, _c->description);
}

bool mcontact_edit (mcontact *_c, char *_opts[]) {
    char **o; int e;
    for (o = _opts; _opts && *o; o+=2) {
        if (!*(o+1)) {

        } else if (!strcasecmp(*o, "id")) {
            e = uuid_parse_secure(_c->id, *(o+1), false, NULL);
            if (!e/*err*/) return false;
        } else if (!strcasecmp(*o, "name")) {
            strncpy(_c->name, *(o+1), sizeof(_c->name)-1);
        } else if (!strcasecmp(*o, "address")) {
            strncpy(_c->address, *(o+1), sizeof(_c->address)-1);
        } else if (!strcasecmp(*o, "town")) {
            strncpy(_c->town, *(o+1), sizeof(_c->town)-1);
        } else if (!strcasecmp(*o, "province")) {
            strncpy(_c->province, *(o+1), sizeof(_c->province)-1);
        } else if (!strcasecmp(*o, "zipcode")) {
            strncpy(_c->zipcode, *(o+1), sizeof(_c->zipcode)-1);
        } else if (!strcasecmp(*o, "email")) {
            e = email_parse(&_c->email, *(o+1));
            if (!e/*err*/) return false;
        } else if (!strcasecmp(*o, "telephone")) {
            strncpy(_c->telephone, *(o+1), sizeof(_c->telephone)-1);
        } else if (!strcasecmp(*o, "description")) {
            strncpy(_c->description, *(o+1), sizeof(_c->description)-1);
        } else {
            
        }
    }
    return true;
}
