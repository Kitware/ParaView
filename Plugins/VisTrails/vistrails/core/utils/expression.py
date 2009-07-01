
############################################################################
##
## This file is part of the Vistrails ParaView Plugin.
##
## This file may be used under the terms of the GNU General Public
## License version 2.0 as published by the Free Software Foundation
## and appearing in the file LICENSE.GPL included in the packaging of
## this file.  Please review the following to ensure GNU General Public
## Licensing requirements will be met:
## http://www.opensource.org/licenses/gpl-2.0.php
##
## This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
## WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
##
############################################################################

############################################################################
##
## Copyright (C) 2006, 2007, 2008 University of Utah. All rights reserved.
##
############################################################################
""" Helper functions for parsing and evaluating expressions """

################################################################################

def evaluate_expressions(expressions):
    """ evaluate_expressions(expressions: str) -> int/float/string        
    Evaluate the contents of the line edit inside each '$' pair
    
    """

    # FIXME: eval should pretty much never be used
    (base, exps) = parse_expression(str(expressions))
    for e in exps:
        try:                        
            base = base[:e[0]] + str(eval(e[1],None,None)) + base[e[0]:]
        except:
            base = base[:e[0]] + '$' + e[1] + '$' + base[e[0]:]
    return base

def parse_expression(expression):
    """ parse_expression(expression: str) -> output (see below)        
    Parse the mixed expression string into expressions and string
    constants

    Keyword arguments:        
    output - (simplified string, [(pos:exp),(pos:exp),...]
        simplified string: the string without any '$exp$'. All
        $$ will be replace by a single $.
    (pos:exp) - the expression to be computed and where it should be
        inserted back to the simplified string, starting from
        the end of the string.

    """
    import re
    output = expression
    result = []
    p = re.compile(r'\$[^$]+\$')
    e = p.finditer(output)
    if e:
        offset = 0
        for s in e:
            exp = s.group()
            result.append((s.span()[0]-offset, exp[1:len(exp)-1]))
            offset += s.span()[1]-s.span()[0]
        result.reverse()
        output = p.sub('', output)
        output.replace('$$','$')
    return (output, result)
