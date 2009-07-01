
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
""" This file is used for VisTrails to interact with Graphviz Dotty to
use its graph for version tree layout"""

from core.data_structures.point import Point
import core.system
import os
import tokenize
import time

################################################################################

class DotNode(object):
    """
    DotNode holding a node in the Dotty output graph
    
    """
    def __init__(self):
        """ DotNode() -> DotNode
        Initialize DotNode as a data structure holding geometry info
        
        """
        self.p = Point(0,0)
        self.height = 0.0        
        self.width = 0.0
        self.id = 0

class DotLayout(object):
    """
    DotLayout is the graph outputed from Dotty which will be used and
    parsed by version tree view
    
    """
    def __init__(self):
        """ DotLayout() -> DotLayout()
        Initialize DotNode as a data structure holding graph structure
        
        """
        self.nodes = {}
        self.height = 0.0
        self.scale = 0.0
        self.width = 0.0

    def parse_dotty_output(self, file, scale=120.0):
        """ parse_dotty_output(file: str, scale: float) -> None
        Parse dotty output file into this DotLayout object
        
        """
        src = tokenize.generate_tokens(file.readline)
        token = src.next()
        while token[0] is not tokenize.ENDMARKER:
            if token[0] is tokenize.NAME:
                # read the first line, which is something like
                # graph scale width height
                if token[1] == 'graph':
                    token = src.next()
                    self.scale = float(token[1])
                    token = src.next()
                    self.width = float(token[1])
                    token = src.next()
                    self.height = float(token[1]) 
                elif token[1] == 'node':
                    n = DotNode()
                    token = src.next()
                    n.id = int(token[1])
                    token = src.next()
                    x = float(token[1])*scale
                    token = src.next()
                    y = float(token[1])*scale*2.0
                    n.p = Point(x,y)
                    token = src.next()
                    n.width = float(token[1])*scale
                    token = src.next()
                    n.height = float(token[1])*scale
                    self.nodes[n.id] = n
                elif token[1] == 'stop':
                    break
            token = src.next()

    def output_vistrail_graph(self, f, vistrail, graph ):
        """ output_vistrail_graph(f: str) -> None
        Using vistrail and graph to prepare a dotty graph input
        
        """
        for v,t in vistrail.tagMap.items():
            if v in graph.vertices.keys():
                f.write('  %s [label="%s"];\n' % (v, t.name))

        f.write('  0;\n')
        self.maxId = 0
        self.minid = 0
        for id in graph.vertices.keys():
            froom = graph.edges_from(id)
            for (first,second) in froom:
                f.write('  %s -> %s;\n' % (id, first))

    def layout_from(self, vistrail, graph):
        """ layout_from(vistrail: VisTrail, graph: Graph) -> None
        Take a graph from VisTrail version and use Dotty to lay it out
        
        """
        # Create VisTrail graph input
        tmp_graph_file = file(core.system.temporary_directory() +
                            'dot_tmp_vistrails.txt', 'w')
        tmp_graph_file.write('digraph G {\n')
        tmp_graph_file.write('  ordering=out;\n')
        self.output_vistrail_graph(tmp_graph_file, vistrail, graph)
        tmp_graph_file.write('}\n')
        tmp_graph_file.close()

        # Run Dotty
        temp_dir = core.system.temporary_directory()
        cmdline = (core.system.graph_viz_dot_command_line() +
                   temp_dir + 'dot_output_vistrails.txt ' +
                   temp_dir + 'dot_tmp_vistrails.txt')
        os.system(cmdline)

        dtty_file = temp_dir + 'dot_output_vistrails.txt'

        if not os.path.exists(dtty_file) :
            print ""
            print "Could not find %s" % dtty_file
            print "Is GraphViz installed and is dotty in your PATH?"
            print ""
            
        file_in = open(dtty_file)

        # Parse Dotty's output
        self.parse_dotty_output(file_in)
        core.system.remove_graph_viz_temporaries()
