
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
import math
import random
import copy

from itertools import imap, chain, izip

from core.utils import all
from core.data_structures.queue import Queue
from core.data_structures.stack import Stack

################################################################################
# Graph

class GraphException(Exception):
    pass

class Graph(object):
    """Graph holds a graph with possible multiple edges. The
    datastructures are all dictionary-based, so datatypes more general than ints
    can be used. For example:
    
    >>> import graph
    >>> g = graph.Graph()
    >>> g.add_vertex('foo')
    >>> g.add_vertex('bar')
    >>> g.add_edge('foo', 'bar', 'edge_foo')
    >>> g.add_edge('foo', 'bar', 'edge_bar')
    >>> g.add_edge('bar', 'foo', 'edge_back')
    >>> g.out_degree('foo')
    2
    >>> g.out_degree('bar')
    1    
    """

    ##########################################################################
    # Constructor
    
    def __init__(self):
        """ Graph() -> Graph
        Initialize an empty graph and return nothing

        """
        self.vertices = {}
        self.adjacency_list = {}
        self.inverse_adjacency_list = {}

    @staticmethod
    def map_vertices(graph, vertex_map=None, edge_map=None):
        """ map_verices(graph: Graph, vertex_map: dict): Graph

        Creates a new graph that is a mapping of vertex ids through
        vertex_map.

        """
        result = Graph()
        if vertex_map is None:
            vertex_map = dict((v, v) for v in self.vertices)
        if edge_map is None:
            edge_map = {}
            for vfrom, lto in graph.adjacency_list.iteritems():
                for (vto, eid) in lto:
                    edge_map[eid] = eid
        result.vertices = dict((vertex_map[v], True) for v in graph.vertices)
        for (vto, lto) in graph.adjacency_list.iteritems():
            result.adjacency_list[vto] = [(vertex_map[to], edge_map[eid]) for (to, eid) in lto]
        for (vto, lto) in graph.inverse_adjacency_list.iteritems():
            result.inverse_adjacency_list[vto] = [(vertex_map[to], edge_map[eid]) for (to, eid) in lto]
        return result

    ##########################################################################
    # Accessors
            
    def inverse(self):
        """inverse() -> Graph
        Inverse all edge directions on the graph and return a Graph

        """
        result = copy.copy(self)
        t = result.adjacency_list
        result.adjacency_list = result.inverse_adjacency_list
        result.inverse_adjacency_list = t
        return result

    def inverse_immutable(self):
        """inverse_immutable() -> Graph
        
        Fast version of inverse(), but requires that output not be
        mutated (it shares with self.)
        """
        result = Graph()
        result.vertices = self.vertices
        result.adjacency_list = self.inverse_adjacency_list
        result.inverse_adjacency_list = self.adjacency_list
        return result

    def undirected_immutable(self):
        """undirected_immutable() -> Graph

        Creates an undirected version of self. Notice that this
        version should not be mutated because there is sharing in the
        adjacency lists and the vertex map.

        Additionally, if self wasn't acyclic, then
        undirected_immutable() won't be simple.
        """
        result = Graph()
        result.vertices = self.vertices
        result.adjacency_list = dict((k, (self.adjacency_list[k] +
                                          self.inverse_adjacency_list[k]))
                                     for k in self.vertices)
        result.inverse_adjacency_list = result.adjacency_list
        return result
        
    def out_degree(self, froom):
        """ out_degree(froom: id type) -> int
        Compute the number of edges leaving 'froom' and return an int

        Keyword arguments:
        froom -- 'immutable' vertex id

        """
        return len(self.adjacency_list[froom])
    
    def in_degree(self, to):
        """ in_degree(to: id type) -> int
        Compute the number of edges entering 'to' and return an int

        Keyword arguments:
        to -- 'immutable' vertex id

        """
        return len(self.inverse_adjacency_list[to])
    
    def sinks(self):
        """ sinks() -> list(id type)
        Find all vertices whose out_degree is zero and return a list of ids

        """
        return [idx for idx in self.vertices.keys() \
                if self.out_degree(idx) == 0]
    
    def sources(self):
        """ sources() -> list(id type)
        Find all vertices whose in_degree is zero and return a list of ids

        """
        return [idx for idx in self.vertices.keys() if self.in_degree(idx) == 0]

    def edges_to(self, id):
        """ edges_to(id: id type) -> list(list)
        Find edges entering a vertex id and return a list of tuples (id,id)

        Keyword arguments:
        id : 'immutable' vertex id
        
        """
        return self.inverse_adjacency_list[id]

    def edges_from(self, id):
        """ edges_from(id: id type) -> list(list)
        Find edges leaving a vertex id and return a list of tuples (id,id)
        
        Keyword arguments:
        id : 'immutable' vertex id

        """
        return self.adjacency_list[id]

    def get_edge(self, frm, to):
        """ get_edge(frm, to) -> edge_id

        Returns the id from the edge from->to."""
        for (t, e_id) in self.edges_from(frm):
            if t == to:
                return e_id

    def has_edge(self, frm, to):
        """ has_edge(frm, to) -> bool

        True if there exists an edge (frm, to)"""

        for (t, _) in self.edges_from(frm):
            if t == to:
                return True
        return False

    ##########################################################################
    # Mutate graph

    def add_vertex(self, id):
        """ add_vertex(id: id type) -> None
        Add a vertex to the graph if it is not already in the graph
        and return nothing

        Keyword arguments:
        id -- vertex id
        
        """
        if not id in self.vertices:
            self.vertices[id] = None
            self.adjacency_list[id] = []
            self.inverse_adjacency_list[id] = []

    def add_edge(self, froom, to, id=None):
        """ add_edge(froom: id type, to: id type, id: id type) -> None
        Add an edge from vertex 'froom' to vertex 'to' and return nothing

        Keyword arguments:
        froom -- 'immutable' origin vertex id
        to    -- 'immutable' destination vertex id
        id    -- 'immutable' edge id (default None)
          
        """
        self.add_vertex(froom)
        self.add_vertex(to)
        self.adjacency_list[froom].append((to, id))
        self.inverse_adjacency_list[to].append((froom, id))
        
    def delete_vertex(self, id):
        """ delete_vertex(id: id type) -> None
        Remove a vertex from graph and return nothing

        Keyword arguments:
        -- id : 'immutable' vertex id
          
        """
        
        for (origin, edge_id) in self.inverse_adjacency_list[id]:
            t = (id, edge_id)
            self.adjacency_list[origin].remove(t)
        for (dest, edge_id) in self.adjacency_list[id]:
            t = (id, edge_id)
            self.inverse_adjacency_list[dest].remove(t)
        del self.adjacency_list[id]
        del self.inverse_adjacency_list[id]
        del self.vertices[id]

    class RenameVertexError(GraphException):
        pass
    
    def rename_vertex(self, old_vertex, new_vertex):
        """ rename_vertex(old_vertex, new_vertex) -> None

        renames old_vertex to new_vertex in the graph, updating the edges
        appropriately. Should not be used to merge vertices, will raise
        exception if new_vertex exists in graph.

        """
        if not (old_vertex in self.vertices):
            raise self.RenameVertexError("vertex '%s' does not exist" % old_vertex) 
        if new_vertex in self.vertices:
            raise self.RenameVertexError("vertex '%s' already exists" % new_vertex)
        self.add_vertex(new_vertex)

        # the slice ([:]) is important for copying, since change_edge
        # mutates the list we'll be traversing
        for (v_from, e_id) in self.inverse_adjacency_list[old_vertex][:]:
            self.change_edge(v_from, old_vertex, new_vertex, e_id, e_id)

        self.adjacency_list[new_vertex] = self.adjacency_list[old_vertex]
        del self.adjacency_list[old_vertex]
        del self.vertices[old_vertex]
       
    def change_edge(self, old_froom, old_to, new_to, old_id=None, new_id=None):
        """ change_edge(old_froom: id, old_to: id, new_to: id, 
                        old_id: id, new_id: id) -> None
        Changes the destination of an edge in a graph **in place**
        
        Keyword arguments:
        old_froom -- 'immutable' origin vertex id
        old_to    -- 'immutable' destination vertex id
        new_to    -- 'immutable' destination vertex id
        old_id    -- 'immutable' edge id (default None)
        new_id    -- 'immutable' edge id (default None)
        """
        
        if old_id == None:
            efroom = self.adjacency_list[old_froom]
            for i, edge in enumerate(efroom):
                if edge[0] == old_to:
                    old_id = edge[1]
                    forward_idx = i
                    break
        else:
            forward_idx = self.adjacency_list[old_froom].index((old_to, old_id))

        self.adjacency_list[old_froom][forward_idx] = ((new_to, new_id))
        self.inverse_adjacency_list[old_to].remove((old_froom, old_id))
        self.inverse_adjacency_list[new_to].append((old_froom, new_id))

    def delete_edge(self, froom, to, id=None):
        """ delete_edge(froom: id type, to: id type, id: id type) -> None
        Remove an edge from graph and return nothing

        Keyword arguments:
        froom -- 'immutable' origin vertex id
        to    -- 'immutable' destination vertex id
        id    -- 'immutable' edge id
          
        """
        if id is None:
            efroom = self.adjacency_list[froom]
            for edge in efroom:
                if edge[0] == to:
                    id = edge[1]
                    break
        if id is None:
            raise GraphException("delete_edge didn't find edge (%s,%s)"%
                                 (froom, to))
        self.adjacency_list[froom].remove((to, id))
        self.inverse_adjacency_list[to].remove((froom, id))

    ##########################################################################
    # Graph algorithms

    def closest_vertex(self, frm, target_list):
        """ closest_vertex(frm, target_list) -> id Uses bfs-like
        algorithm to find closest vertex to frm in target_list

        """
        if frm in target_list:
            return frm
        target_list = set(target_list)
        visited = set([frm])
        parent = {}
        q = Queue()
        q.push(frm)
        while 1:
            try:
                current = q.pop()
            except q.EmptyQueue:
                raise GraphException("no vertices reachable: %s %s" % (frm, list(target_list)))
            efrom = self.edges_from(current)
            for (to, eid) in efrom:
                if to in target_list:
                    return to
                if to not in visited:
                    parent[to] = current
                    q.push(to)
                    visited.add(to)

    def bfs(self, frm):
        """ bfs(frm:id type) -> dict(id type)
        Perform Breadth-First-Search and return a dict of parent id

        Keyword arguments:
        frm -- 'immutable' vertex id

        """
        visited = set([frm])
        parent = {}
        q = Queue()
        q.push(frm)
        while 1:
            try:
                current = q.pop()
            except q.EmptyQueue:
                break
            efrom = self.edges_from(current)
            for (to, eid) in efrom:
                if to not in visited:
                    parent[to] = current
                    q.push(to)
                    visited.add(to)
        return parent

    class GraphContainsCycles(GraphException):
        def __init__(self, v1, v2):
            self.back_edge = (v1, v2)
        def __str__(self):
            return ("Graph contains cycles: back edge %s encountered" %
                    self.back_edge)

    def dfs(self,
            vertex_set=None,
            raise_if_cyclic=False,
            enter_vertex=None,
            leave_vertex=None):
        """ dfs(self,vertex_set=None,raise_if_cyclic=False,enter_vertex=None,
                leave_vertex=None) -> (discovery, parent, finish)
        Performs a depth-first search on a graph and returns three dictionaries with
        relevant information. If vertex_set is not None, then it is used as
        the list of ids to perform the DFS on.
        
        See CLRS p. 541.

        enter_vertex, when present, is called just before visiting a vertex
        for the first time (and only once) with the vertex id as a parameter.

        leave_vertex, when present, is called just after visiting a vertex
        for the first time (and only once) with the vertex id as a parameter.
        """

        if not vertex_set:
            vertex_set = self.vertices

        # Ugly ugly python
        # http://mail.python.org/pipermail/python-list/2006-April/378964.html

        # We cannot explicitly "del data":
        # http://www.python.org/dev/peps/pep-0227/

        class Closure(object):
            
            def clear(self):
                del self.discovery
                del self.parent
                del self.finish
                del self.t
        
        # Straight CLRS p.541
        data = Closure()
        data.discovery = {} # d in CLRS
        data.parent = {} # \pi in CLRS
        data.finish = {}  # f in CLRS
        data.t = 0

        (enter, leave, back, other) = xrange(4)

        # inspired by http://www.ics.uci.edu/~eppstein/PADS/DFS.py

        def handle(v, w, edgetype):
            data.t += 1
            if edgetype == enter:
                data.discovery[v] = data.t
                if enter_vertex:
                    enter_vertex(w)
                if v != w:
                    data.parent[w] = v
            elif edgetype == leave:
                data.finish[w] = data.t
                if leave_vertex:
                    leave_vertex(w)
            elif edgetype == back and raise_if_cyclic:
                raise self.GraphContainsCycles(v, w)
        
        visited = set()
        gray = set()
        # helper function to build stack structure
        def st(v): return (v, iter(self.adjacency_list[v]))
        for vertex in vertex_set:
            if vertex not in visited:
                handle(vertex, vertex, enter)
                visited.add(vertex)
                stack = Stack()
                stack.push(st(vertex))
                gray.add(vertex)
                while stack.size:
                    parent, children = stack.top()
                    try:
                        child, _ = children.next()
                        if child in visited:
                            handle(parent, child, (child in gray
                                                   and back
                                                   or other))
                        else:
                            handle(parent, child, enter)
                            visited.add(child)
                            stack.push(st(child))
                            gray.add(child)
                    except StopIteration:
                        gray.remove(parent)
                        stack.pop()
                        if stack.size:
                            handle(stack.top()[0], parent, leave)
                handle(vertex, vertex, leave)

        result = (data.discovery, data.parent, data.finish)
        data.clear()
        return result

    class VertexHasNoParentError(GraphException):
        def __init__(self, v):
            Exception.__init__(self, v)
            self._v = v
        def __str__(self):
            return ("called parent() on vertex '%s', which has no parent nodes"
                    % self._v)

    def parent(self, v):
        """ parent(v: id type) -> id type
        Find the parent of vertex v and return an id

        Keyword arguments:
        v -- 'immutable' vertex id

        raises VertexHasNoParentError is vertex has no parent

        raises KeyError is vertex is not on graph

        """
        l=self.inverse_adjacency_list[v]
        if len(l):
            (froom, a) = l[-1]
        else:
            raise self.VertexHasNoParentError(v)
        return froom
    
    def vertices_topological_sort(self,vertex_set=None):
        """ vertices_topological_sort(self,vertex_set=None) ->
        sequence(vertices) Returns a sequence of all vertices, so that
        they are in topological sort order (every node traversed is
        such that their parent nodes have already been
        traversed). vertex_set is optionally a list of vertices on
        which to perform the topological sort.
        """
        (d, p, f) = self.dfs(vertex_set,raise_if_cyclic=True)
        lst = [(v, k) for (k,v) in f.iteritems()]
        lst.sort()
        lst.reverse()
        return [v for (k, v) in lst]

    def topologically_contractible(self, subgraph):
        """topologically_contractible(subgraph) -> Boolean.

        Returns true if contracting the subgraph to a single vertex
        doesn't create cycles. This is equivalent to checking whether
        a pipeline subgraph forms a legal abstraction."""
        x = copy.copy(self)
        conns_to_subgraph = self.connections_to_subgraph(subgraph)
        conns_from_subgraph = self.connections_from_subgraph(subgraph)
        for v in subgraph.vertices.iterkeys():
            x.delete_vertex(v)
        free_vertex = max(subgraph.vertices.iterkeys()) + 1
        x.add_vertex(free_vertex)
        for (edge_from, edge_to, edge_id) in conns_to_subgraph:
            x.add_edge(free_vertex, edge_to)
        for (edge_from, edge_to, edge_id) in conns_from_subgraph:
            x.add_edge(edge_from, free_vertex)
        try:
            x.vertices_topological_sort()
            return True
        except self.GraphContainsCycles:
            return False

    ##########################################################################
    # Subgraphs

    def subgraph(self, vertex_set):
        """ subgraph(vertex_set) -> Graph.

        Returns a subgraph of self containing all vertices and
        connections between them."""
        result = Graph()
        vertex_set = set(vertex_set)
        # add vertices
        for vertex in vertex_set:
            result.add_vertex(vertex)
        # add edges
        for vertex_from in vertex_set:
            for (vertex_to, edge_id) in self.edges_from(vertex_from):
                if vertex_to in vertex_set:
                    result.add_edge(vertex_from, vertex_to, edge_id)
        return result

    def connections_to_subgraph(self, subgraph):
        """connections_to_subgraph(subgraph) -> [(vert_from, vert_to, edge_id)]

        Returns the list of all edges that connect to a vertex \in
        subgraph. subgraph is assumed to be a subgraph of self"""
        vertices_to_traverse = set(self.vertices.iterkeys())
        subgraph_verts = set(subgraph.vertices.iterkeys())
        vertices_to_traverse -= subgraph_verts

        result = []
        for v in vertices_to_traverse:
            for e in self.adjacency_list[v]:
                (v_to, e_id) = e
                if v_to in subgraph_verts:
                    result.append((v, v_to, e_id))
        return result

    def connections_from_subgraph(self, subgraph):
        """connections_from_subgraph(subgraph) -> [(vert_from, vert_to, edge_id)]

        Returns the list of all edges that connect from a vertex \in
        subgraph to a vertex \not \in subgraph. subgraph is assumed to
        be a subgraph of self"""
        subgraph_verts = set(subgraph.vertices.iterkeys())
        vertices_to_traverse = subgraph_verts

        result = []
        for v in vertices_to_traverse:
            for e in self.adjacency_list[v]:
                (v_to, e_id) = e
                if v_to not in subgraph_verts:
                    result.append((v, v_to, e_id))
        return result

    ##########################################################################
    # Iterators


    def iter_edges_from(self, vertex):
        """iter_edges_from(self, vertex) -> iterable

        Returns an iterator over all edges in the form
        (vertex, vert_to, edge_id)."""
        def fn(edge):
            (edge_to, edge_id) = edge
            return (vertex, edge_to, edge_id)
        return imap(fn, self.adjacency_list[vertex])

    def iter_edges_to(self, vertex):
        """iter_edges_to(self, vertex) -> iterable

        Returns an iterator over all edges in the form
        (vertex, vert_to, edge_id)."""
        def fn(edge):
            (edge_from, edge_id) = edge
            return (edge_from, vertex, edge_id)
        return imap(fn, self.inverse_adjacency_list[vertex])

    def iter_all_edges(self):
        """iter_all_edges() -> iterable

        Returns an iterator over all edges in the graph in the form
        (vert_from, vert_to, edge_id)."""
        verts = self.iter_vertices()
        edge_itors = imap(self.iter_edges_from, verts)
        return chain(*[v for v in edge_itors])

    def iter_vertices(self):
        """iter_vertices() -> iterable

        Returns an iterator over all vertex ids of the graph."""
        return self.vertices.iterkeys()

    ##########################################################################
    # Special Python methods

    def __str__(self):
        """ __str__() -> str
        Format the graph for serialization and return a string

        """
        vs = self.vertices.keys()
        vs.sort()
        al = []
        for i in [map(lambda (t, i): (f, t, i), l)
                  for (f, l) in self.adjacency_list.items()]:
            al.extend(i)
        al.sort(edge_cmp)
        return "digraph G { " \
               + ";".join([str(s) for s in vs]) + ";" \
               + ";".join(["%s -> %s [label=\"%s\"]" % s for s in al]) + "}"

    def __repr__(self):
        """ __repr__() -> str
        Similar to __str__ to re-represent the graph and returns a string

        """
        return self.__str__()

    def __copy__(self):
        """ __copy__() -> Graph
        Make a copy of the graph and return a Graph

        """
        cp = Graph()
        cp.vertices = copy.copy(self.vertices)
        cp.adjacency_list = dict((k, v[:]) for (k,v) in self.adjacency_list.iteritems())
        cp.inverse_adjacency_list = dict((k, v[:]) for (k,v) in self.inverse_adjacency_list.iteritems())
        return cp

    def __eq__(self, other):
        # Does not test isomorphism - vertices must be consistently labeled
        # might be slow - don't use in tight code
        if type(self) <> type(other):
            return False
        for v in self.vertices:
            if not v in other.vertices:
                return False
        for vfrom, elist in self.adjacency_list.iteritems():
            for vto, eid in elist:
                if not other.get_edge(vfrom, vto) == eid:
                    return False
        return True

    def __ne__(self, other):
        return not (self == other)

    ##########################################################################
    # Etc

    @staticmethod
    def from_random(size):
        """ from_random(size:int) -> Graph
        Create a DAG with approximately size/e vertices and 3*|vertex| edges
        and return a Graph

        Keyword arguments:
        size -- the estimated size of the graph to generate

        """
        result = Graph()
        verts = filter(lambda x: x>0, peckcheck.a_list(peckcheck.an_int)(size))
        for v in verts:
            result.add_vertex(v)
        k = size / math.e
        p = (6*k) / ((k+1)*(k+2))
        eid = 0
        for v in verts:
            for k in verts:
                if v < k and random.random() > p:
                    result.add_edge(v, k, eid)
                    eid = eid + 1
        return result

def edge_cmp(v1, v2):
    """ edge_cmp(v1: id type, v2:id type) -> int
    Defines how the comparison must be done between edges  and return a boolean

    Keyword arguments:
    v1 -- 'sequence' edge information
    v2 -- 'sequence' other edge information
    
    """
    (from1, to1, id1) = v1
    (from2, to2, id2) = v2
    c1 = cmp(from1, from2)
    if c1:
        return c1
    c2 = cmp(to1, to2)
    if c2:
        return c2
    else:
        return cmp(id1, id2)

################################################################################
# Unit testing

import unittest
import random

class TestGraph(unittest.TestCase):
     """ Class to test Graph

     It tests vertex addition, the out_degree of a sink and in_degree of a
     source consistencies.
    
     """

     def make_complete(self, v):
         """returns a complete graph with v verts."""
         g = Graph()
         for x in xrange(v):
             g.add_vertex(x)
         for f in xrange(v):
             for t in xrange(f+1, v):
                 g.add_edge(f, t, f * v + t)
         return g

     def make_linear(self, v, bw=False):
         """returns a linear graph with v verts. if bw=True, add
         backward links."""
         g = Graph()
         for x in xrange(v):
             g.add_vertex(x)
         for x,y in izip(xrange(v-1), xrange(1, v)):
             g.add_edge(x, y, x)
             if bw:
                 g.add_edge(y, x, x + v)
         return g

     def get_default_graph(self):
         g = Graph()
         g.add_vertex(0)
         g.add_vertex(1)
         g.add_vertex(2)
         g.add_vertex(3)
         g.add_vertex(4)
         g.add_edge(0,1,0)
         g.add_edge(1,2,1)
         g.add_edge(0,3,2)
         g.add_edge(3,2,3)
         g.add_edge(2,4,4)
         return g
     
     def test1(self):
         """Test adding edges and vertices"""
         g = Graph()
         g.add_vertex('0')
         g.add_vertex('1')
         g.add_vertex('2')
         g.add_vertex('3')
         g.add_edge('0', '1', 0)
         g.add_edge('1', '2', 1)
         g.add_edge('2', '3', 2)
         parent = g.bfs('0')
         self.assertEquals(parent['3'], '2')
         self.assertEquals(parent['2'], '1')
         self.assertEquals(parent['1'], '0')

     def test2(self):
         """Test bread-first-search"""
         g = self.get_default_graph()
         p = g.bfs(0)
         k = p.keys()
         k.sort()
         self.assertEquals(k, [1, 2, 3, 4])
         inv = g.inverse()
         p_inv = inv.bfs(4)
         k2 = p_inv.keys()
         k2.sort()
         self.assertEquals(k2, [0, 1, 2, 3])
         
     def test3(self):
         """Test sink and source degree consistency"""
         g = Graph()
         for i in xrange(100):
             g.add_vertex(i);
         for i in xrange(1000):
             v1 = random.randint(0,99)
             v2 = random.randint(0,99)
             g.add_edge(v1, v2, i)
         sinkResult = [None for i in g.sinks() if g.out_degree(i) == 0]
         sourceResult = [None for i in g.sources() if g.in_degree(i) == 0]
         if len(sinkResult) <> len(g.sinks()):
             assert False
         if len(sourceResult) <> len(g.sources()):
             assert False

     def test_remove_vertices(self):
         g = self.make_linear(5)
         g.delete_vertex(1)
         g.delete_vertex(2)
     
     def test_DFS(self):
         """Test DFS on graph."""
         g = self.get_default_graph()
         g.dfs()

     def test_topological_sort(self):
         """Test toposort on graph."""
         g = self.get_default_graph()
         g.vertices_topological_sort()

         g = self.make_linear(10)
         r = g.vertices_topological_sort()
         assert r == [0,1,2,3,4,5,6,7,8,9]

         g = Graph()
         g.add_vertex('a')
         g.add_vertex('b')
         g.add_vertex('c')
         g.add_edge('a', 'b')
         g.add_edge('b', 'c')
         assert g.vertices_topological_sort() == ['a', 'b', 'c']

     def test_limited_DFS(self):
         """Test DFS on graph using a limited set of starting vertices."""
         g = self.get_default_graph()
         g.dfs(vertex_set=[1])
         g.dfs(vertex_set=[1,3])
         g.dfs(vertex_set=[1,2])

     def test_limited_topological_sort(self):
         """Test toposort on graph using a limited set of starting vertices."""
         g = self.get_default_graph()
         g.vertices_topological_sort(vertex_set=[1])
         g.vertices_topological_sort(vertex_set=[1,3])
         g.vertices_topological_sort(vertex_set=[1,2])

     def test_print_empty_graph(self):
         """Test print on empty graph"""
         g = Graph()
         g.__str__()

     def test_delete(self):
         """Tests consistency of data structure after deletion."""
         g = Graph()
         g.add_vertex(0)
         g.add_vertex(1)
         g.add_vertex(2)
         g.add_edge(0, 1, 0)
         g.add_edge(1, 2, 1)
         g.delete_vertex(2)
         self.assertEquals(g.adjacency_list[1], [])

     def test_raising_DFS(self):
         """Tests if DFS with cycle-checking will raise exceptions."""
         g = Graph()
         g.add_vertex(0)
         g.add_vertex(1)
         g.add_vertex(2)
         g.add_edge(0, 1)
         g.add_edge(1, 2)
         g.add_edge(2, 0)
         try:
             g.dfs(raise_if_cyclic=True)
         except Graph.GraphContainsCycles, e:
             pass
         else:
             raise Exception("Should have thrown")

     def test_call_inverse(self):
         """Test if calling inverse methods work."""
         g = Graph()
         g.add_vertex(0)
         g.add_vertex(1)
         g.add_vertex(2)
         g.add_edge(0, 1)
         g.add_edge(1, 2)
         g.add_edge(2, 0)
         g2 = g.inverse()
         g3 = g.inverse_immutable()
     
     def test_subgraph(self):
         """Test subgraph routines."""
         g = self.make_complete(5)
         sub = g.subgraph([0,1])
         assert 0 in sub.vertices
         assert 1 in sub.vertices
         assert (1,1) in sub.adjacency_list[0]
         assert (0,1) in sub.inverse_adjacency_list[1]

         g = self.make_linear(3)
         sub = g.subgraph([0, 2])
         assert 0 in sub.vertices
         assert 2 in sub.vertices
         assert sub.adjacency_list[0] == []
         assert sub.adjacency_list[2] == []
         
     def test_connections_to_subgraph(self):
         """Test connections_to_subgraph."""
         g = self.make_linear(5)
         sub = g.subgraph([3])
         assert len(g.connections_to_subgraph(sub)) == 1
         g = self.make_linear(5, True)
         sub = g.subgraph([3])
         assert len(g.connections_to_subgraph(sub)) == 2

     def test_connections_from_subgraph(self):
         """Test connections_from_subgraph."""
         g = self.make_linear(5)
         sub = g.subgraph([3])
         assert len(g.connections_from_subgraph(sub)) == 1
         g = self.make_linear(5, True)
         sub = g.subgraph([3])
         assert len(g.connections_from_subgraph(sub)) == 2

     def test_topologically_contractible(self):
         """Test topologically_contractible."""
         g = self.make_linear(5)
         sub = g.subgraph([1, 2])
         assert g.topologically_contractible(sub)
         sub = g.subgraph([1, 3])
         assert not g.topologically_contractible(sub)

         g = Graph()
         g.add_vertex(0)
         g.add_vertex(1)
         g.add_vertex(2)
         g.add_vertex(3)
         g.add_edge(0, 1)
         g.add_edge(2, 3)
         for i in xrange(1, 16):
             s = []
             for j in xrange(4):
                 if i & (1 << j): s.append(j)
             assert g.topologically_contractible(g.subgraph(s))

     def test_iter_vertices(self):
         g = self.get_default_graph()
         l = list(g.iter_vertices())
         l.sort()
         assert l == [0,1,2,3,4]

     def test_iter_edges(self):
         g = self.get_default_graph()
         l = [v for v in g.iter_all_edges()]
         l.sort()
         assert l == [(0,1,0), (0,3,2), (1, 2, 1), (2, 4, 4), (3, 2, 3)]

     def test_iter_edges_empty(self):
         """Test iterators on empty parts of the graph."""
         g = Graph()
         for a in g.iter_vertices():
             assert False
         g.add_vertex(0)
         for a in g.iter_edges_from(0):
             assert False
         for a in g.iter_edges_to(0):
             assert False
         for a in g.iter_all_edges():
             assert False

     def test_get_edge_none(self):
         g = Graph()
         g.add_vertex(0)
         g.add_vertex(1)
         assert g.get_edge(0, 1) == None

     def test_dfs_before(self):
         g = self.make_linear(10)
         inc = []
         dec = []
         def before(id): inc.append(id)
         def after(id): dec.append(id)
         g.dfs(enter_vertex=before,
               leave_vertex=after)
         assert inc == [0,1,2,3,4,5,6,7,8,9]
         assert inc == list(reversed(dec))
         assert all(izip(inc[:-1], inc[1:]),
                    lambda (a, b): a < b)
         assert all(izip(dec[:-1], dec[1:]),
                    lambda (a, b): a > b)

     def test_parent_source(self):
         g = self.make_linear(10)
         self.assertRaises(g.VertexHasNoParentError,
                           lambda: g.parent(0))
         for i in xrange(1, 10):
             assert g.parent(i) == i-1

     def test_rename_vertex(self):
         g = self.make_linear(10)
         self.assertRaises(g.RenameVertexError,
                           lambda: g.rename_vertex(0, 1))
         assert g.get_edge(0, 1) is not None
         assert g.get_edge(0, 11) is None
         g.rename_vertex(1, 11)
         assert g.get_edge(0, 1) is None
         assert g.get_edge(0, 11) is not None
         g.rename_vertex(11, 1)
         assert g.get_edge(0, 1) is not None
         assert g.get_edge(0, 11) is None

     def test_delete_get_edge(self):
         g = self.make_linear(10)
         self.assertRaises(GraphException, lambda: g.delete_edge(7, 9))
         assert g.has_edge(7, 8)
         g.delete_edge(7, 8)
         assert not g.has_edge(7, 8)

     def test_bfs(self):
         g = self.make_linear(5)
         lst = g.bfs(0).items()
         lst.sort()
         assert lst == [(1, 0), (2, 1), (3, 2), (4, 3)]
         lst = g.bfs(2).items()
         lst.sort()
         assert lst == [(3, 2), (4, 3)]

     def test_undirected(self):
         g = self.make_linear(5).undirected_immutable()
         lst = g.bfs(0).items()
         lst.sort()
         assert lst == [(1, 0), (2, 1), (3, 2), (4, 3)]
         lst = g.bfs(2).items()
         lst.sort()
         assert lst == [(0, 1), (1, 2), (3, 2), (4, 3)]

     def test_closest_vertex(self):
         g = self.make_linear(10)
         g.delete_edge(7, 8)
         g = g.undirected_immutable()
         self.assertRaises(GraphException, lambda: g.closest_vertex(1, [9]))
         assert g.closest_vertex(3, [2, 6, 7]) == 2
         assert g.closest_vertex(3, [2, 3, 6, 7]) == 3
         # Test using dictionary as target_list

         d1 = {2:True, 6:True, 7:False}
         d2 = {2:True, 6:True, 7:False, 3:False}
         d3 = {9:True}
         self.assertRaises(GraphException, lambda: g.closest_vertex(1, d3))
         assert g.closest_vertex(3, d1) == 2
         assert g.closest_vertex(3, d2) == 3

     def test_copy_not_share(self):
         g = self.make_linear(10)
         g2 = copy.copy(g)
         for v in g.vertices:
             assert id(g.adjacency_list[v]) <> id(g2.adjacency_list[v])
             assert id(g.inverse_adjacency_list[v]) <> id(g2.inverse_adjacency_list[v])

     def test_copy_works(self):
         g = self.make_linear(10)
         g2 = copy.copy(g)
         for v in g.vertices:
             assert v in g2.vertices
             assert g2.adjacency_list[v] == g.adjacency_list[v]
             assert g2.inverse_adjacency_list[v] == g.inverse_adjacency_list[v]

     def test_equals(self):
         g = self.make_linear(5)
         assert copy.copy(g) == g
         g2 = copy.copy(g)
         g2.add_vertex(10)
         assert g2 <> g

     def test_map_vertices(self):
         g = self.make_linear(5)
         m = {0: 0, 1: 1, 2: 2, 3: 3, 4: 4}
         assert g == Graph.map_vertices(g, m)
         m = {0: 5, 1: 6, 2: 7, 3: 8, 4: 9}
         assert g <> Graph.map_vertices(g, m)
         
if __name__ == '__main__':
    unittest.main()
