
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

from core.data_structures.graph import Graph
from core.utils import expression
from core.utils import trace_method
import copy
import parser

##############################################################################

class InternalTuple(object):
    """Tuple used internally for constant tuples."""

    def _get_length(self, length):
        return len(self._values)
    def _set_length(self, length):
        self._values = [None] * length
    length = property(_get_length, _set_length)

    def compute(self):
        return

    def set_input_port(self, index, connector):
        self._values[index] = connector()

    def get_output(self, port):
        return tuple(self._values)

    def update(self):
        pass
        

##############################################################################

class BaseInterpreter(object):

    def __init__(self):
        """ BaseInterpreter() -> BaseInterpreter
        Initialize class members
        
        """
        self.done_summon_hook = None
        self.done_update_hook = None

    def get_name_dependencies(self, astList):
        """get_name_dependencies(astList) -> list of something 
        
        """
        
        result = []
        if astList[0]==1: # NAME token
            result += [astList[1]]
        else:
            for e in astList:
                if type(e) is ListType:
                    result += self.get_name_dependencies(e)
        return result

    def build_alias_dictionary(self, pipeline):
        aliases = {}
        for mid in pipeline.modules:
            for f in pipeline.modules[mid].functions:
                fsig = f.getSignature()
                for pidx in xrange(len(f.params)):
                    palias = f.params[pidx].alias
                    if palias and palias!='':
                        for f1 in reversed(pipeline.modules[mid].functions):
                            if f1.getSignature()==fsig:
                                p = f1.params[pidx]
                                aliases[palias] = (p.type, expression.parse_expression(str(p.strValue)))
                                break
        return aliases

    def compute_evaluation_order(self, aliases):
        # Build the dependencies graph
        dp = {}
        for alias,(atype,(base,exp)) in aliases.items():
            edges = []
            for e in exp:
                edges += self.get_name_dependencies()
            dp[alias] = edges
            
        # Topological Sort to find the order to compute aliases
        # Just a slow implementation, O(n^3)...
        unordered = copy.copy(list(aliases.keys()))
        ordered = []
        while unordered:
            added = []
            for i in xrange(len(unordered)):
                ok = True
                u = unordered[i]
                for j in xrange(len(unordered)):
                    if i!=j:
                        for v in dp[unordered[j]]:
                            if u==v:
                                ok = False
                                break
                        if not ok: break
                if ok: added.append(i)
            if not added:
                print 'Looping dependencies detected!'
                break
            for i in reversed(added):
                ordered.append(unordered[i])
                del unordered[i]
        return ordered

    def evaluate_exp(self, atype, base, exps, aliases):
        # FIXME: eval should pretty much never be used
        import datetime        
        for e in exps: base = (base[:e[0]] +
                               str(eval(e[1],
                                        {'datetime':locals()['datetime']},
                                        aliases)) +
                               base[e[0]:])
        if not atype in ['string', 'String']:
            if base=='':
                base = '0'
            try:
                base = eval(base,None,None)
            except:
                pass
        return base

    def resolve_aliases(self, pipeline,
                        customAliases=None):
        # Compute the 'locals' dictionary by evaluating named expressions
        aliases = self.build_alias_dictionary(pipeline)
        if customAliases:
            #customAliases can be only a subset of the aliases
            #so we need to build the Alias Dictionary always
            for k,v in customAliases.iteritems():
                aliases[k] = v
        
        ordered = self.compute_evaluation_order(aliases)
        casting = {'int': int, 'float': float, 'double': float, 'string': str,
                   'Integer': int, 'Float': float, 'String': str}
        for alias in reversed(ordered):
            (atype,(base,exps)) = aliases[alias]
            value = self.evaluate_exp(atype,base,exps,aliases)
            aliases[alias] = casting[atype](value)

        for mid in pipeline.modules:
            for f in pipeline.modules[mid].functions:
                for p in f.params:
                    if p.alias and p.alias!='':
                        p.evaluatedStrValue = str(aliases[p.alias])
                    else:
                        (base,exps) = expression.parse_expression(
                            str(p.strValue))
                        p.evaluatedStrValue = str(
                            self.evaluate_exp(p.type,base,exps,aliases))
        return aliases
    

    def set_done_summon_hook(self, hook):
        """ set_done_summon_hook(hook: function(pipeline, objects)) -> None
        Assign a function to call right after every objects has been
        summoned during execution
        
        """
        self.done_summon_hook = hook

    def set_done_update_hook(self, hook):
        """ set_done_update_hook(hook: function(pipeline, objects)) -> None
        Assign a function to call right after every objects has been
        updated
        
        """
        self.done_update_hook = hook

##############################################################################
