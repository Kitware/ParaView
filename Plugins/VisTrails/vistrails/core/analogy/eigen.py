
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
from core.data_structures.bijectivedict import Bidict
from itertools import imap, chain
import copy
import core.modules.module_registry
try:
    import scipy
    _analogies_available = True
except ImportError:
    _analogies_available = False

import math
from pipeline_utils import *

reg = core.modules.module_registry.registry
from core.utils import append_to_dict_of_lists
from core.system import temporary_directory

##############################################################################
# This is the analogy implementation

##############################################################################
# EigenBase

def mzeros(*args, **kwargs):
    nkwargs = copy.copy(kwargs)
    nkwargs['dtype'] = float
    az = scipy.zeros(*args, **nkwargs)
    return scipy.matrix(az)

def mones(*args, **kwargs):
    nkwargs = copy.copy(kwargs)
    nkwargs['dtype'] = float
    az = scipy.ones(*args, **nkwargs)
    return scipy.matrix(az)

#mzeros = lambda *args, **kwargs: scipy.matrix(scipy.zeros(*args, **kwargs))
# mones = lambda *args, **kwargs: scipy.matrix(scipy.ones(*args, **kwargs))
 
class EigenBase(object):

    ##########################################################################
    # Constructor and initialization

    def __init__(self,
                 pipeline1,
                 pipeline2):
        self._p1 = pipeline1
        self._p2 = pipeline2
        self.init_vertex_similarity()
        self.init_edge_similarity()
        self._debug = False

    def init_vertex_similarity(self):
        num_verts_p1 = len(self._p1.graph.vertices)
        num_verts_p2 = len(self._p2.graph.vertices)
        m_i = mzeros((num_verts_p1, num_verts_p2))
        m_o = mzeros((num_verts_p1, num_verts_p2))
        def get_vertex_map(g):
            return Bidict([(v, k) for (k, v)
                           in enumerate(g.iter_vertices())])
        # vertex_maps: vertex_id to matrix index
        self._g1_vertex_map = get_vertex_map(self._p1.graph)
        self._g2_vertex_map = get_vertex_map(self._p2.graph)
        for i in xrange(num_verts_p1):
            for j in xrange(num_verts_p2):
                v1_id = self._g1_vertex_map.inverse[i]
                v2_id = self._g2_vertex_map.inverse[j]
                (in_s8y, out_s8y) = self.compare_modules(v1_id, v2_id)
                m_i[i,j] = in_s8y
                m_o[i,j] = out_s8y
        print m_i
        print m_o
        self._input_vertex_s8y = m_i
        self._output_vertex_s8y = m_o
        self._vertex_s8y = (m_i + m_o) / 2.0

    def init_edge_similarity(self):
        def get_edge_map(g):
            itor = enumerate(imap(lambda x: x[2],
                                  g.iter_all_edges()))
            return Bidict([(v, k) for (k, v)
                           in itor])
        # edge_maps: edge_id to matrix index
        self._g1_edge_map = get_edge_map(self._p1.graph)
        self._g2_edge_map = get_edge_map(self._p2.graph)

        m_e = mzeros((len(self._g1_edge_map),
                      len(self._g2_edge_map)))

        for i in xrange(len(self._g1_edge_map)):
            for j in xrange(len(self._g2_edge_map)):
                c1_id = self._g1_edge_map.inverse[i]
                c2_id = self._g2_edge_map.inverse[j]
                s8y = self.compare_connections(c1_id, c2_id)
                m_e[i, j] = s8y
        self._edge_s8y = m_e

    ##########################################################################
    # Atomic comparisons for modules and connections

    def create_type_portmap(self, ports):
        result = {}
        for port_name, port_descs in ports.iteritems():
            for port_desc in port_descs:
                sp = tuple(port_desc)
                append_to_dict_of_lists(result, sp, port_name)
        return result

    def compare_modules(self, p1_id, p2_id):
        """Returns two values \in [0, 1] that is how similar the
        modules are intrinsically, ie. without looking at
        neighborhoods. The first value gives similarity wrt input
        ports, the second to output ports."""
        (m1_inputs, m1_outputs) = self.get_ports(self._p1.modules[p1_id])
        (m2_inputs, m2_outputs) = self.get_ports(self._p2.modules[p2_id])


        m2_input_hist = self.create_type_portmap(m2_inputs)
        m2_output_hist = self.create_type_portmap(m2_outputs)

        output_similarity = 0.0
        total = 0
        # Outputs can be covariant, inputs can be contravariant
        # FIXME: subtypes, etc etc
        for (port_name, port_descs) in m1_outputs.iteritems():
            # we use max() .. because we want to count
            # nullary ports as well
            total_descs = max(len(port_descs), 1)
            total += total_descs
            # assert len(port_descs) == 1
            if (m2_outputs.has_key(port_name) and
                m2_outputs[port_name] == port_descs):
                output_similarity += float(total_descs)
            else:
                for port_desc in port_descs:
                    port_desc = tuple(port_desc)
                    if m2_output_hist.has_key(port_desc):
                        output_similarity += 1
        if len(m1_outputs):
            output_similarity /= total
        else:
            output_similarity = 0.2

        if (self._p1.modules[p1_id].name !=
            self._p2.modules[p2_id].name):
            output_similarity *= 0.99

        input_similarity = 0.0
        total = 0
        # FIXME: consider supertypes, etc etc
        
        for (port_name, port_descs) in m1_inputs.iteritems():
            # we use max() .. because we want to count
            # nullary ports as well
            total_descs = max(len(port_descs), 1)
            total += total_descs
            if (m2_inputs.has_key(port_name) and
                m2_inputs[port_name] == port_descs):
                input_similarity += 1.0
            else:
                for port_desc in port_descs:
                    port_desc = tuple(port_desc)
                    if m2_input_hist.has_key(port_desc):
                        input_similarity += 1

        if len(m1_inputs):
            input_similarity /= total
        else:
            input_similarity = 0.2

        if (self._p1.modules[p1_id].name !=
            self._p2.modules[p2_id].name):
            input_similarity *= 0.99

        return (input_similarity, output_similarity)

    def compare_connections(self, p1_id, p2_id):
        """Returns a value \in [0, 1] that says how similar
        the two connections are."""
        c1 = self._p1.connections[p1_id]
        c2 = self._p2.connections[p2_id]

        # FIXME: Make this softer in the future
        if c1.source.name != c2.source.name:
            return 0.0
        if c1.destination.name != c2.destination.name:
            return 0.0

        m_c1_sid = self._g1_vertex_map[c1.sourceId]
        m_c1_did = self._g1_vertex_map[c1.destinationId]
        m_c2_sid = self._g2_vertex_map[c2.sourceId]
        m_c2_did = self._g2_vertex_map[c2.destinationId]

        return (self._output_vertex_s8y[m_c1_sid, m_c2_sid] *
                self._input_vertex_s8y[m_c1_did, m_c2_did])


    ##########################################################################
    # Utility

    @staticmethod
    def pm(m, digits=5):
        def get_digits(x):
            if x == 0: return 0
            return int(math.log(abs(x) * 10.0, 10.0))
        vd = scipy.vectorize(get_digits)
        dm = vd(m)
        widths = dm.max(0)
        (l, c) = m.shape
        for i in xrange(l):
            EigenBase.pv(m[i,:],
                         digits=digits,
                         left_digits=widths)

    @staticmethod
    def pv(v, digits=5, left_digits=None):
        # FIXME - some scipy indexing seems to be currently
        # inconsistent across different deployed versions. Fix this.
        return
        if type(v) == scipy.matrix:
            v = scipy.array(v)[0]
        (c,) = v.shape
        print "[ ",
        for j in xrange(c):
            if left_digits != None:
                d = left_digits[0,j]
            else:
                d = 0
            fmt = ("%" +
                   str(d + digits + 1) + 
                   "." + str(digits) + "f ")
            print (fmt % v[j]),
        print "]"

    def print_s8ys(self):
        print "Input s8y"
        self.pm(self._input_vertex_s8y)
        print "\nOutput s8y"
        self.pm(self._output_vertex_s8y)
        print "\nConnection s8y"
        self.pm(self._edge_s8y)
        print "\nCombined s8y"
        self.pm(self._vertex_s8y)

    # FIXME: move this somewhere decent.
    def get_ports(self, module, include_optional=False):
        """get_ports(module) -> (input_ports, output_ports)

        Returns all ports for a given module name, all the way
        up the class hierarchy."""

        def remove_descriptions(d):
            def update_elements(spec):
                return [v.__name__ for v
                        in spec.types()]
            for k in d.keys():
                v = update_elements(d[k])
                if len(v):
                    d[k] = v
                else:
                    del d[k]

        inputs = dict([(port.name, port.spec) for
                       port in module.destinationPorts()
                       if (not port.optional or include_optional)])
        outputs = dict([(port.name, port.spec) for
                        port in module.sourcePorts()
                        if (not port.optional or include_optional)])

        remove_descriptions(inputs)
        remove_descriptions(outputs)
        return (inputs, outputs)

##############################################################################
# EigenPipelineSimilarity

class EigenPipelineSimilarity(EigenBase):

    ##########################################################################
    # Constructor and initialization

    def __init__(self,
                 pipeline1,
                 pipeline2):
        EigenBase.__init__(self, pipeline1, pipeline2)
        self.init_operator()

    def init_operator(self):
        num_verts_p1 = len(self._p1.graph.vertices)
        num_verts_p2 = len(self._p2.graph.vertices)
        def ix(a,b): return num_verts_p2 * a + b
        self._operator = mzeros((num_verts_p1 * num_verts_p2,
                                 num_verts_p1 * num_verts_p2))

        u = 0.85
        
        for i in xrange(num_verts_p1):
            v1_id = self._g1_vertex_map.inverse[i]
            for j in xrange(num_verts_p2):
                ix_ij = ix(i,j)
                self._operator[ix_ij, ix_ij] = u
                v2_id = self._g2_vertex_map.inverse[j]
                def edges(pip, v_id):
                    return chain(imap(lambda (f, t, i): (t, i),
                                      self._p1.graph.iter_edges_from(v1_id)),
                                 imap(lambda (f, t, i): (f, i),
                                      self._p1.graph.iter_edges_to(v1_id)))
                p1_edges = edges(self._p1, v1_id)
                p2_edges = edges(self._p2, v2_id)
                running_sum = 0.0
                for (_, p1_edge) in p1_edges:
                    for (_, p2_edge) in p2_edges:
                        e1_id = self._g1_edge_map[p1_edge]
                        e2_id = self._g2_edge_map[p2_edge]
                        running_sum += self._edge_s8y[e1_id, e2_id]
                p1_edges = edges(self._p1, v1_id)
                p2_edges = edges(self._p2, v2_id)
                if not running_sum:
                    continue
                for (p1_v, p1_edge_id) in p1_edges:
                    for (p2_v, p2_edge_id) in p2_edges:
                        e1_id = self._g1_edge_map[p1_edge_id]
                        e2_id = self._g2_edge_map[p2_edge_id]
                        p1_v_id = self._g1_vertex_map[p1_v]
                        p2_v_id = self._g2_vertex_map[p2_v]
                        value = self._edge_s8y[e1_id, e2_id]
                        value *= (1.0 - u) / running_sum
                        self._operator[ix_ij, ix(p1_v_id, p2_v_id)] = value

    ##############################################################################
    # Solve

    def step(self, m):
        v = m.reshape(len(self._p1.modules) *
                      len(self._p2.modules))
        v = scipy.dot(self._operator, v)
        v = v.reshape(len(self._p1.modules),
                      len(self._p2.modules)).transpose()
        v = (v / v.sum(0)).transpose()
        return v

    def solve(self):
        i = 0
        while i < 50:
            i += 1
            v = self.step(self._vertex_s8y)
            residue = self._vertex_s8y - v
            residue *= residue
            if residue.sum() < 0.0001:
                break
            self._vertex_s8y = v
        mp = [(self._g1_vertex_map.inverse[ix],
               self._g2_vertex_map.inverse[v])
              for (ix, v) in
              enumerate(self._vertex_s8y.argmax(1))]
        return dict(mp)
        
##############################################################################
# EigenPipelineSimilarity2

class EigenPipelineSimilarity2(EigenBase):

    def __init__(self, *args, **kwargs):
        basekwargs = copy.copy(kwargs)
        del basekwargs['alpha']
        EigenBase.__init__(self, *args, **basekwargs)
        self.init_operator(alpha=kwargs['alpha'])

    def init_operator(self, alpha):
        def edges(pip, v_id):
            def from_fn(x): return (x[1], x[2])
            def to_fn(x): return (x[0], x[2])
            return chain(imap(from_fn,   pip.graph.iter_edges_from(v_id)),
                         imap(to_fn,     pip.graph.iter_edges_to(v_id)))
        num_verts_p1 = len(self._p1.graph.vertices)
        num_verts_p2 = len(self._p2.graph.vertices)
        n = num_verts_p1 * num_verts_p2
        def ix(a,b): return num_verts_p2 * a + b
        # h is the raw substochastic matrix
        from scipy import sparse
        h = sparse.csr_matrix((n, n))
        # a is the dangling node vector
        a = mzeros(n)
        for i in xrange(num_verts_p1):
            v1_id = self._g1_vertex_map.inverse[i]
            for j in xrange(num_verts_p2):
                ix_ij = ix(i,j)
                v2_id = self._g2_vertex_map.inverse[j]
                running_sum = 0.0
                for (_, p1_edge) in edges(self._p1, v1_id):
                    for (_, p2_edge) in edges(self._p2, v2_id):
                        e1_id = self._g1_edge_map[p1_edge]
                        e2_id = self._g2_edge_map[p2_edge]
                        running_sum += self._edge_s8y[e1_id, e2_id]
                if running_sum == 0.0:
                    a[0, ix_ij] = 1.0
                    continue
                for (p1_v, p1_edge_id) in edges(self._p1, v1_id):
                    for (p2_v, p2_edge_id) in edges(self._p2, v2_id):
                        e1_id = self._g1_edge_map[p1_edge_id]
                        e2_id = self._g2_edge_map[p2_edge_id]
                        p1_v_id = self._g1_vertex_map[p1_v]
                        p2_v_id = self._g2_vertex_map[p2_v]
                        value = self._edge_s8y[e1_id, e2_id] / running_sum
                        h[ix_ij, ix(p1_v_id, p2_v_id)] = value

        self._alpha = alpha
        self._n = n
        self._h = h
        self._a = a
        self._e = mones(n) / n

    def step(self, pi_k):
        r = pi_k * self._h * self._alpha
        t = pi_k * self._alpha * self._a.transpose()
        r += self._v * (t[0,0] + 1.0 - self._alpha)
        return r

    def solve_v(self, s8y):
        fl = s8y.flatten()
        self._v = fl / fl.sum()
        v = copy.copy(self._e)
        step = 0
        def write_current_matrix():
            f = file('%s/%s_%03d.v' % (temporary_directory(),
                                       self._debug_matrix_file, step), 'w')
            x = v.reshape(len(self._p1.modules),
                          len(self._p2.modules))
            for i in xrange(len(self._p1.modules)):
                for j in xrange(len(self._p2.modules)):
                    f.write('%f ' % x[i,j])
                f.write('\n')
            f.close()
        while 1:
            if self._debug:
                write_current_matrix()
            new = self.step(v)
            r = (v-new)
            r = scipy.multiply(r,r)
            s = r.sum()
            if s < 0.0000001 and step >= 10:
                return v
            step += 1
            v = new

    def solve(self):
        def write_debug_pipeline_positions(pipeline, mmap, f):
            f.write('%d %d\n' % (len(pipeline.modules),
                                 len(pipeline.connections)))
            for k, v in mmap.iteritems():
                f.write('%d %d\n' % (k, v))
            c = pipeline_centroid(pipeline)
            mn, mx = pipeline_bbox(pipeline)
            f.write('%f %f %f %f\n' % (mn.x, mn.y, mx.x, mx.y))
            for i, m in pipeline.modules.iteritems():
                nc = m.center - c
                f.write('%d %s %f %f\n' % (i, m.name, nc.x, nc.y))
            for i, c in pipeline.connections.iteritems():
                f.write('%d %d %d\n' % (i, c.sourceId, c.destinationId))
            
        if self._debug:
            out = file('%s/pipelines.txt' % temporary_directory(), 'w')
            write_debug_pipeline_positions(self._p1, self._g1_vertex_map, out)
            write_debug_pipeline_positions(self._p2, self._g2_vertex_map, out)
            self.print_s8ys()
            
        self._debug_matrix_file = 'input_matrix'
        r_in  = self.solve_v(self._input_vertex_s8y)
        self._debug_matrix_file = 'output_matrix'
        r_out = self.solve_v(self._output_vertex_s8y)
        r_in = r_in.reshape(len(self._p1.modules),
                            len(self._p2.modules))
        r_out = r_out.reshape(len(self._p1.modules),
                              len(self._p2.modules))

        s = r_in.sum(1)
        s[s==0.0] = 1
        r_in /= s

        s = r_out.sum(1)
        s[s==0.0] = 1
        r_out /= s
        
        r_combined = scipy.multiply(r_in, r_out)

        # Breaks ties on combined similarity
        r_in = r_in * 0.9 + r_combined * 0.1
        r_out = r_out * 0.9 + r_combined * 0.1

        if self._debug:
            print "input similarity"
            self.pm(r_in, digits=3)
            print "output similarity"
            self.pm(r_out, digits=3)
            print "combined similarity"
            self.pm(r_combined, digits=3)

        inputmap = dict([(self._g1_vertex_map.inverse[ix],
                          self._g2_vertex_map.inverse[v[0,0]])
                         for (ix, v) in
                         enumerate(r_in.argmax(1))])
        outputmap = dict([(self._g1_vertex_map.inverse[ix],
                           self._g2_vertex_map.inverse[v[0,0]])
                          for (ix, v) in
                          enumerate(r_out.argmax(1))])
        combinedmap = dict([(self._g1_vertex_map.inverse[ix],
                             self._g2_vertex_map.inverse[v[0,0]])
                            for (ix, v) in
                            enumerate(r_combined.argmax(1))])
#         print inputmap
#         print outputmap
        return inputmap, outputmap, combinedmap


    
