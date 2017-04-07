Proxy Documentation Formatting             {#ProxyDocumentationFormatting}
==============================

This page describes formatting options for proxy documentation.

###Formatting options are added for proxy documentation###
Documentation for proxies (and for input properties for proxies)
accepts reStructured text(RST) formatting options. Supported options
are: **bold** (use ``**bold**``), *italic* (use ``*italic*``),
unordered lists, and paragraphs (use an empty line). Unordered list
are formatted like in the following example, with an empty line
before and after the list.

    - first item
    - second item

Nested lists are not supported. Note that the text enclosed between
**Documentation** tags has to be aligned at column 0, (space is
significant in RST documents) and that we do not accept empty lines
between items in an unordered list. Formatted output will be displayed
in ParaView online help, ParaView Python documentation, and tooltips
displayed in ParaView client. See the **Calculator** and **Glyph**
filters for examples on how to format other filters.
