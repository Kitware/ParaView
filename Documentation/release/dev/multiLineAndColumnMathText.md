## Multicolumn and multiline support in equation rendering

The rendering of equations using MathText (with matplotlib enabled via the cmake option VTK_MODULE_ENABLE_VTK_RenderingMatplotlib) now supports multiline and multicolumn. You can define a new line by writing a new line in the text source, and you can define a new column by entering the character '|'. You can still write a '|' by escaping it with a backslash ('\|')
