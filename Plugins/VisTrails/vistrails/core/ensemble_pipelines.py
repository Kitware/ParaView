
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
from core.utils import expression
import sys
import copy

class EnsemblePipelines(object):
    def __init__(self, pipelines=None):
        if pipelines == None:
            self.pipelines = {}
        else:
            self.pipelines = dict([(k,copy.copy(v))
                                   for (k,v)
                                   in pipelines.iteritems()])
        self.aliases = {}
        self.sources = []
        self.active_pipelines = []

    def add_pipeline(self, id, pipeline):
        """add_pipeline(id: int, pipeline: Pipeline) -> None 
        adds a copy of pipeline to the local dictionary 
        
        """
        self.pipelines[id] = copy.copy(pipeline)

    def update(self, name, value):
        """update(name: str, value: str) -> None
        Propagates new value of an alias through a list of 
        active pipelines

        name - the name of the variable and key into the alias dictionary
        value - the new value of the variable
        returns - void
        """
        self.change_parameter(name, value)
        for id in self.active_pipelines:
            pipeline = self.pipelines[id]
            pipeline.set_alias_str_value(name, value)
            
    def assemble_aliases(self):
        """assemble_aliases() -> None
        Generate a list of all aliases across the active pipelines
        in self.pipelines, which is stored in self.aliases
        Also, for each key in self.aliases, self.sources has the same key,
        mapped to a tuple of the type (p, m, f, pa)
        where p is the index of the pipeline in self.pipelines, m is the
        index of the module, f of the function, and pa of the parameter
         
        """
        union = {}
        sources = {}
        for pi in self.active_pipelines:
            pipeline = self.pipelines[pi]
            for name, info in pipeline.aliases.iteritems():
                if not union.has_key(name):
                    value = str(pipeline.get_alias_str_value(name))
                    e = expression.parse_expression(value)
                    union[name] = (info[0], e)
                    sources[name] = [(pi, info[1], info[2], info[3])]
                else:
                    sources[name].append((pi, info[1], info[2], info[3]))
                    
        self.sources = sources
        self.aliases = union

    def change_parameter(self, name, value):
        """change_parameter(name:str, value:str) -> None
        Changes a parameter in the internal alias dictionary.
        In order to have the changes propagated in the pipelines, call 
        update(name,value) instead.
        
        """
        if self.aliases.has_key(name):
            info = self.aliases[name]
            #tuples don't allow changing in place
            self.aliases[name] = (info[0],(value,info[1][1]))
    
    def get_source(self, id, alias):
        """get_source(id: int, alias: str) -> tuple
        Return a tuple containing the module id, function id, and parameter id
        of an alias in a given pipeline """
        p_list = self.sources[alias]
        for info in p_list:
            if id == info[0]:
                return (info[1], info[2], info[3])
        return None
        
