#==============================================================================
#
#  Program:   ParaView
#  Module:    redtabhtml.py
#
#  Copyright (c) Kitware, Inc.
#  All rights reserved.
#  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
#
#     This software is distributed WITHOUT ANY WARRANTY; without even
#     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#     PURPOSE.  See the above copyright notice for more information.
#
#==============================================================================
#  Adds a subclass of the HtmlFormatter that highlights tabs in red
#  This is due to Python not accepthing a mix of spaces and tabs in its
#  source code.  To emphasize the tabs (since spaces are preferred in
#  ParaView), the tabs will be highlighted in red.

from pygments.formatters.html import HtmlFormatter

class RedTabHtmlFormatter(HtmlFormatter):
    def wrap(self, source, outfile):
        return self._wrap_code(source)

    def _wrap_code(self, source):
        yield 0,    "<div><pre>"
        for i, t in source:
            if i == 1:
                # it is a line of formatted code, change tabs to have red backgrounds
                t = t.replace("\t","<span style=\"background-color: #FF0000\">\t</span>")
            yield i, t
        yield 0, "</pre></div>"
