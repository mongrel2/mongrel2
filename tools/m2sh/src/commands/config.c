#include <dbg.h>
#include <unistd.h>
#include "../commands.h"
#include "../config_file.h"

int Command_load(Command *cmd)
{
    bstring db_file = option(cmd, "db", "config.sqlite");
    bstring conf_file = option(cmd, "config", "mongrel2.conf");
    bstring what = NULL;
    bstring why = NULL;

    check_file(conf_file, "config file", R_OK);

    Config_load(bdata(conf_file), bdata(db_file));

    what = bfromcstr("load");
    why = bfromcstr("command");

    log_action(db_file, what, why, NULL, conf_file);

error: // fallthrough
    bdestroy(what);
    bdestroy(why);
    return 0;
}
