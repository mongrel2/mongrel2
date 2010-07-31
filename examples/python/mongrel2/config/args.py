import re
import sys
import inspect
from mongrel2.config import rc


S_IP_ADDRESS = lambda x, token: ['ip_address', token]
S_WORD = lambda x, token:  ['word', token]
S_EMAIL_ADDR = lambda x, token:  ['email', token]
S_OPTION = lambda x, token:  ['option', token.split("-")[-1]]
S_INT = lambda x, token:  ['int', int(token) ]
S_BOOL = lambda x, token:  ['bool', bool(token) ]
S_EMPTY = lambda x, token:  ['empty', '']
S_STRING = lambda x, token:  ['string', token]
S_TRAILING = lambda x, token:  ['trailing', None]

class ArgumentError(Exception): 
    """Thrown when args encounters a command line format error."""
    pass


SCANNER = re.Scanner([
    (r"[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,4}", S_EMAIL_ADDR),
    (r"[0-9]+\.[0-9]+\.[0-9]+\.[0-9]", S_IP_ADDRESS),
    (r"-+[a-zA-Z0-9]+", S_OPTION),
    (r"True", S_BOOL),
    (r"[0-9]+", S_INT),
    (r"--", S_TRAILING),
    (r"[a-z\-]+", S_WORD),
    (r"\s", S_EMPTY),
    (r".+", S_STRING),
])


def match(tokens, of_type = None):
    """
    Responsible for taking a token off and processing it, ensuring it is 
    of the correct type.  If of_type is None (the default) then you are
    asking for anything.
    """
    # check the type (first element)
    if of_type:
        if not peek(tokens, of_type):
            raise ArgumentError("Expecting '%s' type of argument not %s in tokens: %r.  Read the help." % 
                               (of_type, tokens[0][0], tokens))

    # take the token off the front
    tok = tokens.pop(0)

    # return the value (second element)
    return tok[1]


def peek(tokens, of_type):
    """Returns true if the next token is of the type, false if not.  It does not
    modify the token stream the way match does."""
    if len(tokens) == 0:
        raise ArgumentError("This command expected more on the command line.  Not sure how you did that.")

    return tokens[0][0] == of_type


def trailing_production(data, tokens):
    """Parsing production that handles trailing arguments after a -- is given."""
    data['TRAILING'] = [x[1] for x in tokens]
    del tokens[:]

def option_production(data, tokens):
    """The Option production, used for -- or - options.  The number of - aren't 
    important.  It will handle either individual options, or paired options."""
    if peek(tokens, 'trailing'):
        # this means the rest are trailing arguments, collect them up
        match(tokens, 'trailing')
        trailing_production(data, tokens)
    else:
        opt = match(tokens, 'option')
        if not tokens:
            # last one, it's just true
            data[opt] = True
        elif peek(tokens, 'option') or peek(tokens, 'trailing'):
            # the next one is an option so just set this to true
            data[opt] = True
        else:
            # this option is set to something else, so we'll grab that
            data[opt] = match(tokens)


def options_production(tokens):
    """List of options, optionally after the command has already been taken off."""
    data = {}
    while tokens:
        option_production(data, tokens)
    return data


def command_production(tokens):
    """The command production, just pulls off a word really."""
    return match(tokens, 'word')


def tokenize(argv):
    """Goes through the command line args and tokenizes each one, trying to match
    something in the scanner.  If any argument doesn't completely parse then it
    is considered a 'string' and returned raw."""

    tokens = []
    for arg in argv:
        toks, remainder = SCANNER.scan(arg)
        if remainder or len(toks) > 1:
            tokens.append(['string', arg])
        else:
            tokens += toks
    return tokens


def parse(argv):
    """
    Tokenizes and then parses the command line as wither a command style or
    plain options style argument list.  It determines this by simply if the
    first argument is a 'word' then it's a command.  If not then it still
    returns the first element of the tuple as None.  This means you can do:

        command, options = args.parse(sys.argv[1:])

    and if command==None then it was an option style, if not then it's a command 
    to deal with.
    """
    tokens = tokenize(argv)
    if not tokens:
        return None, {}
    elif peek(tokens, "word"):
        # this is a command style argument
        return command_production(tokens), options_production(tokens)
    else:
        # options only style
        return None, options_production(tokens)


def determine_kwargs(function):
    """
    Uses the inspect module to figure out what the keyword arguments
    are and what they're defaults should be, then creates a dict with
    that setup.  The results of determine_kwargs() is typically handed
    to ensure_defaults().
    """
    spec = inspect.getargspec(function)
    keys = spec[0]
    values = spec[-1]
    result = {}
    for i in range(0, len(keys)):
        result[keys[i]] = values[i]
    return result

def ensure_defaults(options, reqs):
    """
    Goes through the given options and the required ones and does the
    work of making sure they match.  It will raise an ArgumentError
    if any option is required.  It will also detect that required TRAILING
    arguments were not given and raise a separate error for that.
    """
    for key in reqs:
        if reqs[key] == None:
            # explicitly set to required
            if key not in options:
                if key == "TRAILING":
                    raise ArgumentError("Additional arguments required after a -- on the command line.")
                else:
                    raise ArgumentError("Option -%s is required by this command." % key)
        else:
            if key not in options:
                options[key] = reqs[key]

def command_module(mod, command, options, ending="_command"):
    """Takes a module, uses the command to run that function."""
    function = mod.__dict__[command+ending]
    kwargs = determine_kwargs(function)
    ensure_defaults(options, kwargs)
    try:
        return function(**options)
    except TypeError, exc:
        print "ERROR: ", exc


def available_help(mod, ending="_command"):
    """Returns the dochelp from all functions in this module that have _command
    at the end."""
    help_text = []
    for key in mod.__dict__:
        if key.endswith(ending):
            name = key.split(ending)[0]
            help_text.append(name + ":\n" + mod.__dict__[key].__doc__)

    return help_text


def help_for_command(mod, command, ending="_command"):
    """
    Returns the help string for just this one command in the module.
    If that command doesn't exist then it will return None so you can
    print an error message.
    """

    if command in available_commands(mod):
        return mod.__dict__[command + ending].__doc__
    else:
        return None


def available_commands(mod, ending="_command"):
    """Just returns the available commands, rather than the whole long list."""
    commands = []
    for key in mod.__dict__:
        if key.endswith(ending):
            commands.append(key.split(ending)[0])

    commands.sort()
    return commands


def invalid_command_message(mod, exit_on_error):
    """Called when you give an invalid command to print what you can use."""
    print "You must specify a valid command.  Try these: "
    print ", ".join(available_commands(mod))

    if exit_on_error: 
        sys.exit(1)
    else:
        return False


def parse_and_run_command(argv, mod, default_command=None, exit_on_error=True):
    """
    A one-shot function that parses the args, and then runs the command
    that the user specifies.  If you set a default_command, and they don't
    give one then it runs that command.  If you don't specify a command,
    and they fail to give one then it prints an error.

    On this error (failure to give a command) it will call sys.exit(1).
    Set exit_on_error=False if you don't want this behavior, like if
    you're doing a unit test.
    """
    try:
        command, options = parse(argv)

        if not command and default_command:
            command = default_command
        elif not command and not default_command:
            return invalid_command_message(mod, exit_on_error)

        if command not in available_commands(mod):
            return invalid_command_message(mod, exit_on_error)

        command_module(mod, command, options)
    except ArgumentError, exc:
        rcf = rc.read_rc()

        try:
            function = mod.__dict__[command+"_command"]
            needed = determine_kwargs(function)
            new = {}
            for k in needed.keys():
                if k in rcf:
                    new[k] = rcf[k]
            command_module(mod, command, new)
        except ArgumentError, exc:
            print "ERROR: ", exc
            if exit_on_error:
                sys.exit(1)

    return True

