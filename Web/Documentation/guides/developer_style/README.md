# ParaViewWeb Code Style Guidelines


## Introduction

This document is based on PEP-0008. It provides coding conventions for
the Python and JavaScript codes that comprise the ParaViewWeb distribution. This
document blatantly copies structure and text from the "Style Guide for Python
Code" (PEP-0008).

## Code layout

### Indentation

Use 4 spaces per indentation level.

JavaScript:

    function foo() {
        // some code
        ....
    }

Python:

    def foo():
        # some code
        pass

Comments should be indented to align with the code with which they correspond, as shown
above.

### Tabs or Spaces?

Never use tabs. Always use 4 spaces instead of a tab.

### Maximum Line Length

It is preferred to limit line length to 79 characters. However, for
better readability in certain cases, it is acceptable to have lines longer than 79 characters.

## Whitespaces in Expressions and Statements

### Pet Peeves

Avoid extraneous whitespaces in the following situations:

Immediately inside parenthesis, brackets, or braces:

    Yes: spam(ham[1], {eggs: 2})
    No:  spam( ham[ 1 ], { eggs: 2 } )

Immediately before a comma, semicolon, or colon:

    Yes: if x == 4: print x, y; x, y = y, x
    No:  if x == 4: print x , y ; x , y = y , x

Immediately before the open parenthesis that starts the argument list of a function call:

    Yes: spam(1)
    No:  spam (1)

Immediately before the open parenthesis that starts an indexing or slicing:

    Yes: dict["key"] = list[index]
    No:  dict ["key"] = list [index]

An assignment (or other) operator has more than one space adjacent to it to align it with another operator:

    Yes:
        var x = 1;
        var yesterday = 2;
    No:
        var x         = 1;
        var yesterday = 2;


A function name is followed by a parenthesis. A space should be added between a statement and a subsequent parenthesis, according to the examples below:

    Yes:
        if (foo == 12) { }
        for (var cc=0; cc < 12; cc++) { }
        spam(12);
    No:
        if(foo==12) { }
        for(var cc=0; ....)
        spam (12);

## Blocks (JavaScript)

if/else/for/while/try always have braces and always go on multiple lines.
Braces should always be used on blocks.

    Yes:
        if (foo === 12) {
            blah();
        }

    No:
        if (foo === 12)
            blah();
        if (foo == 12) blah();


Do not put statements on the same line as a conditional.

    Yes:
        if (foo === 12) {
            return;
        }
    No:
        if (foo === 12) return;

else/else if/catch go on the same line as the brace.

    if (blah) {
        baz();
    } else if (boo) {
        baz1();
    } else {
        baz2();
    }

## Naming Conventions

### Names to Avoid

Never use the characters 'l' (lowercase letter [el]), 'O' (uppercase letter [oh]),
or 'I' (uppercase letter [eye]) as single character variable names.
In some fonts, these characters are indistinguishable from the numerals one and
zero. When tempted to use 'l', use 'L' instead.

### Module Names

Modules should have short, all-lowercase names. Underscores can be used in the
module name if it improves readability.

### Class Names

Class names use the _CapWords_ convention. Classes for
internal use also have a leading underscore. This is true when referring
to dynamically constructed Object instances as well.

### Constants

Use _UPPERCASE\_NAMES_ when defining constants. Underscores can be used to improve
readability.
    VERSION = "2.0.0";
    DEFAULT_OPTIONS = {};

### Function Names

Function names use _mixedCase_ beginning with lowercase letters.
    function stillRender() {
    }

    def renderActiveView():
        pass


### Class Method Names

These are named similar to functions, using _mixedCase_.
    class Foo:
        def myMethod(self):
            pass

Use of leading underscores is accepted as per language guidelines.
