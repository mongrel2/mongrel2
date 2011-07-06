int Command_commit(Command *cmd);
int Command_log(Command *cmd);
int log_action(bstring db_file, bstring what, bstring why, bstring where, bstring how);
