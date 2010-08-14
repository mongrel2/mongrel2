

int parse_kegogi_file(const char *path, CommandList *commandList) {
    FILE *script;
    TokenList *tokens = NULL;
    bstring buffer = NULL;
    void *parser = NULL;


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
              commandList);
    }
    Parse(parser, 0, 0, commandList);
    TokenList_destroy(tokens);
    tokens = NULL;

    return commandList->count;

error:
    TokenList_destroy(tokens);
    bdestroy(buffer);
    if(script) fclose(script);
    if(parser) ParseFree(parser, free);

    return -1;
}
