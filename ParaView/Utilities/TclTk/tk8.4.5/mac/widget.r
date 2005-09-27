/*
 * widget.r --
 *
 */

type 'TEXT'
{
    string;
};

#define TK_LIBRARY_RESOURCES 3000

resource 'TEXT' (TK_LIBRARY_RESOURCES+114, "tclshrc", purgeable) 
{
"# read widgets demo script\n"
"console hide\n"
"source [file join $tk_library demos widget]\n"
};