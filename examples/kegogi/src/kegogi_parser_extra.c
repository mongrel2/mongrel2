

int parse_kegogi_file(const char *path, Command commands[],
                      int max_num_commands, ParamDict **defaults) {
    FILE *script;
    TokenList *tokens = NULL;
    bstring buffer = NULL;
    void *parser = NULL;

    CommandList commandList = {
        .size = max_num_commands,
        .count = 0,
        .defaults = NULL,
        .commands = commands
    };

    script = fopen(path, "r");
    check(script, "Failed to open file: %s", path);

    buffer = bread((bNread)fread, script);
    check_mem(buffer);

    fclose(script);
    script = NULL;

    tokens = get_kegogi_tokens(buffer);
    check_mem(tokens);

    bdestroy(buffer);
    buffer = NULL;

    parser = ParseAlloc(malloc);
    check_mem(parser);

    int i;
    for(i = 0; i < tokens->count; i++) {
        Parse(parser, tokens->tokens[i].type, &tokens->tokens[i],
              &commandList);
    }
    Parse(parser, TKNEWLINE, 0, &commandList);
    Parse(parser, 0, 0, &commandList);
    TokenList_destroy(tokens);
    tokens = NULL;

    *defaults = commandList.defaults;
    return commandList.count;

error:
    TokenList_destroy(tokens);
    bdestroy(buffer);
    if(script) fclose(script);
    if(parser) ParseFree(parser, free);

    return -1;
}
