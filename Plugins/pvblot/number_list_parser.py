##########################################################################
#
# number_list_parser
#
# usage: parse_number_list("1, 2, 10 to 15, 20 to 30 by 2", int)
#
#   will return: [1, 2, 10, 11, 12, 13, 14, 15, 20, 22, 24, 26, 28, 30]
#
# Currently only int is supported.  To support float you must uncomment
# the frange method below and then test it :-)

class Error(Exception): pass

def parse_number_list(s, number_class):
    """Take a string representing a list of numbers and number ranges and return
    a list of the parsed numbers.  For example: '1, 2, 50 to 100 by 5'
    Acceptable strings follow the grammar:

      A =: <A> <B>
      B =: <number> | <C>
      C =: "<number> to <number>" <D>
      D =: "by <number>" | ""

    """
    # remove commas, convert to lower case, split into tokens and parse
    tokens = s.replace(',', '').lower().split()
    number_list = []
    while len(tokens):
        if tokens_are_range(tokens, number_class): number_list += pop_range_tokens(tokens, number_class)
        else: number_list += [pop_number_token(number_class, tokens)]
    return number_list

def is_number_class(token, number_class):
    try: number_class(token)
    except: return False
    return True

def convert_to_number_class(token, number_class):
    try: num = number_class(token)
    except ValueError: raise Error("Expected number type '%s' not '%s'" % (number_class.__name__, token))
    return num

def tokens_are_range(tokens, number_class):
    """Check if the given list of tokens appears to be a range of the form: 'X to Y'"""
    # If there are less than two tokens or the second token is a number then
    # these tokens do not appear to be a range
    if len(tokens) < 2 or is_number_class(tokens[1], number_class): return False
    return True

def pop_string_token(s, tokens):
    if not len(tokens): tokens += " "
    token = tokens.pop(0)
    if token != s: raise Error("Expected '%s' not '%s'" % (s, token))

def pop_number_token(number_class, tokens):
    if not len(tokens): tokens += " "
    return convert_to_number_class(tokens.pop(0), number_class)

def pop_range_tokens(tokens, number_class):
    """Given a list of tokens try to parse them as 'X to Y' or 'X to Y by Z'.
    If the parsing fails then raise a Error.  All parsed tokens are
    popped from the incoming tokens list."""

    # Get first three tokens as "X" "to" "Y"
    start = pop_number_token(number_class, tokens)
    pop_string_token("to", tokens)
    stop = pop_number_token(number_class, tokens)

    # If there are no more tokens or the next token is a number then
    # there is no "by Z", return the range
    if not len(tokens) or is_number_class(tokens[0], number_class):
        return build_number_range(number_class, start, stop)

    # Get the next two tokens as "BY" "Z", return the range
    pop_string_token("by", tokens)
    step = pop_number_token(number_class, tokens)
    return build_number_range(number_class, start, stop, step)

def build_number_range(number_class, start, stop, step=1):
    if step == 0: raise Error, "Invalid BY value '0'"
    if number_class is int:
        if (start > stop and step > 0):
            raise Error("Starting number '%s' > ending '%s'" % (start, stop))
        if (start < stop and step < 0):
            raise Error("Starting number '%s' < ending '%s' with negative step" % (start, stop))
        return range(start, stop+cmp(step, 0), step)
    else: raise Error("Unhandled number class '%s'" % number_class.__name__)

"""
import math
def frange(start, stop, step=1.0):
    start = float(start)
    count = int(math.ceil( (stop-start)/step )  )
    return list(start + n*step for n in xrange(count))
"""

