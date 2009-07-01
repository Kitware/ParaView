
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
# ################################################################################
# # QueryWF: Base class for queries over workflows

# class QueryWF(object):

#     def __init__(self, transformer, logQueries):
#         self.transformer = transformer
#         self. = logQueries
    

# ################################################################################

# class QueryWFModuleName(QueryWF):
    
#     def __init__(self, name):
#         self.name = name

#     def run(self, module):
#         return module.name == self.name

# ################################################################################

# class QueryWFParameter(QueryWF):
    
#     def __init__(self, param, name):
#         self.param = param
#         self.name = name

#     def run(self, module):
#         for f in module.functions:
#             if (f.name == name and 
#                 len(f.params) == 1 and
#                 f.params[0].name == self.name:
#                 return True
#         return False

# ################################################################################

# class QueryWFAnd(QueryWF):
            
#     def __init__(self, queries):
#         self.queries = queries

#     def run(self, module):
#         for query in self.queries:
#             if not query.run(module):
#                 return False
#         return True

# ################################################################################

# class QueryWFOr(QueryWF):
    
#     def __init__(self, queries):
#         self.queries = queries

#     def run(self, module):
#         for query in self.queries:
#             if query.run(module):
#                 return True
#         return False

# ################################################################################

# class QueryWFNot(QueryWF):

#     def __init__(self, query):
#         self.query = query

#     def run(self, module):
#         return not self.query.run(module)

# ################################################################################

# class QueryWFResultTransformer(object):
#     pass

# ################################################################################

# class QueryWFUpstream(QueryWFResultTransformer):

#     def run(self, pipeline, module_ids):
#         result = set()
#         inv_graph = pipeline.graph.inverse()
#         for i in module_ids:
#             result = result.union(set(inv_graph.bfs(i).keys() + [i]))
#         return result


# class QueryWFIdentity(QueryWFResultTransformer):

#     def run(self, pipeline, module_ids):
#         return module_ids


# class QueryWFUnion(QueryWFResultTransformer):

#     def __init__(self, queries):
#         self.queries = queries

#     def run(self, pipeline, module_ids):
#         result = set()
#         for query in queries:
#             result = result.union(query.run(pipeline, module_ids))

################################################################################

from core.utils import memo_method

 
class Query(object):

    def upstream(self, graph, id):
        return graph.bfs(id).keys()

    weekdayMap = {'mo':0, 'tu':1, 'we':2, 'th':3, 'fr':4, 'sa':5, 'su':6}

    def weekday(self, time, weekday):
        return time.weekday() == self.weekdayMap[weekday.lower()[:2]]

    def computeIndices(self):
        self.versionDict = {}

        if not len(self.queryResult):
            return
        elif len(self.queryResult[0]) == 1:
            for (v,) in self.queryResult:
                self.versionDict[v] = {}
        elif len(self.queryResult[0]) == 2:
            for (v, m) in self.queryResult:
                if not self.versionDict.has_key(v):
                    self.versionDict[v] = {m: []}
                self.versionDict[v][m] = {}
        else:
            assert len(self.queryResult[0]) == 3
            for (v, m, e) in self.queryResult:
                if not self.versionDict.has_key(v):
                    self.versionDict[v] = {m: [e]}
                elif not self.versionDict[v].has_key(m):
                    self.versionDict[v][m] = [e]
                else:
                    self.versionDict[v][m].append(e)

    def match(self, vistrail, action):
        return action.timestep in self.versionDict

    def matchModule(self, version_id, module):
        return (self.tupleLength < 2 or
                (version_id in self.versionDict and
                 module.id in self.versionDict[version_id]))

    def executionInstances(self, version_id, module_id):
        versionDict = self.versionDict
        assert self.tupleLength == 3
        if versionDict[version_id].has_key(module_id):
            return versionDict[version_id][module_id]
        else:
            return None

class Query1a(Query):

    def run(self, vistrail, name):
        result = []
        versions = vistrail.tagMap.values()
        for version in versions:
            p = vistrail.getPipeline(version)
            for module_id, module in p.modules.iteritems():
                if module.name == 'FileSink':
                    for f in module.functions:
                        if (f.name == 'outputName' and
                            len(f.params) == 1 and
                            f.params[0].value() == 'atlas-x.gif'):
                            result.append((version, module_id))
        self.queryResult = result
        self.tupleLength = 2
        self.computeIndices()
        return result


class Query1b(Query):

    def run(self, vistrail, name):
        result = []
        versions = vistrail.tagMap.values()
        for version in versions:
            p = vistrail.getPipeline(version)
            ms = []
            for module_id, module in p.modules.iteritems():
                if module.name == 'FileSink':
                    for f in module.functions:
                        if (f.name == 'outputName' and
                            len(f.params) == 1 and
                            f.params[0].value() == 'atlas-x.gif'):
                            ms.append(module_id)
            s = set()
            inv_graph = p.graph.inverse()
            for m in ms:
                s = s.union(set(inv_graph.bfs(m).keys() + [m]))
            for m in s:
                result.append((version, m))
        self.queryResult = result
        self.tupleLength = 2
        self.computeIndices()
        return result


class Query1c(Query):

    def run(self, vistrail, name):
        import gui.vis_application
        c = gui.vis_application.logger.db.cursor()
        c.execute("""
        select distinct module_id, wf_version from
        wf_exec, exec, vistrails
        where
           wf_exec.wf_exec_id = exec.wf_exec_id and
           vistrails.vistrails_name = %s and
           vistrails.vistrails_id = wf_exec.vistrails_id""", name)
        lst = list(c.fetchall())
        versions = set([x[1] for x in lst])
        executions = set(lst)
        result = []
        for version in versions:
            p = vistrail.getPipeline(int(version))
            ms = []
            for module_id, module in p.modules.iteritems():
                if (module_id, version) not in executions:
                    continue
                if module.name == 'FileSink':
                    for f in module.functions:
                        if (f.name == 'outputName' and
                            len(f.params) == 1 and
                            f.params[0].value() == 'atlas-x.gif'):
                            ms.append(module_id)
            s = set()
            inv_graph = p.graph.inverse()
            for m in ms:
                s = s.union(set(inv_graph.bfs(m).keys() + [m]))
            for m in s:
                result.append((int(version), m))
        self.queryResult = result
        self.tupleLength = 2
        self.computeIndices()
        return result
    

class Query2(Query):

    def run(self, vistrail, name):
        import gui.vis_application
        c = gui.vis_application.logger.db.cursor()
        c.execute("""
        select distinct module_id, wf_version from
        wf_exec, exec, vistrails
        where
           wf_exec.wf_exec_id = exec.wf_exec_id and
           vistrails.vistrails_name = %s and
           vistrails.vistrails_id = wf_exec.vistrails_id""", name)
        lst = list(c.fetchall())
        versions = set([x[1] for x in lst])
        executions = set(lst)
        result = []
        for version in versions:
            p = vistrail.getPipeline(int(version))
            inv_graph = p.graph.inverse()

            # s = upstream(x) union x where x.name = filesink blablabla
            s = set()
            for module_id, module in p.modules.iteritems():
                if (module_id, version) not in executions:
                    continue
                if module.name == 'FileSink':
                    for f in module.functions:
                        if (f.name == 'outputName' and
                            len(f.params) == 1 and
                            f.params[0].value() == 'atlas-x.gif'):
                            s = s.union(set(self.upstream(inv_graph, module_id) + [module_id]))
                            break

            # s2 = upstream(y) where y.name = softmean
            s2 = set()
            for module_id, module in p.modules.iteritems():
                if module.name == 'SoftMean':
                    s2 = s2.union(set(self.upstream(inv_graph, module_id) + [module_id]))

            qresult = s - s2
            
            for m in qresult:
                result.append((int(version), m))
        self.queryResult = result
        self.tupleLength = 2
        self.computeIndices()
        return result


class Query3(Query):

    def run(self, vistrail, name):
        import gui.vis_application
        c = gui.vis_application.logger.db.cursor()
        c.execute("""
        select distinct module_id, wf_version from
        wf_exec, exec, vistrails
        where
           wf_exec.wf_exec_id = exec.wf_exec_id and
           vistrails.vistrails_name = %s and
           vistrails.vistrails_id = wf_exec.vistrails_id""", name)
        lst = list(c.fetchall())
        versions = set([x[1] for x in lst])
        executions = set(lst)
        result = []
        for version in versions:
            p = vistrail.getPipeline(int(version))
            ms = []
            for module_id, module in p.modules.iteritems():
                if (module_id, version) not in executions:
                    continue
                if module.name == 'FileSink':
                    for f in module.functions:
                        if (f.name == 'outputName' and
                            len(f.params) == 1 and
                            f.params[0].value() == 'atlas-x.gif'):
                            ms.append(module_id)
            s = set()
            inv_graph = p.graph.inverse()
            for m in ms:
                s = s.union(set(inv_graph.bfs(m).keys() + [m]))
            for m in s:
                if (p.modules[m].has_annotation_with_key('stage') and
                    p.modules[m].get_annotation_by_key('stage').value in \
                        ['3','4','5']):
                    result.append((int(version), m))
        self.queryResult = result
        self.tupleLength = 2
        self.computeIndices()
        return result
        
            
class Query4(Query):

    def run(self, vistrail, name):
        import gui.vis_application
        c = gui.vis_application.logger.db.cursor()
        c.execute("""
        select distinct exec.ts_start, exec.ts_end, exec_id, module_id, wf_version from
        wf_exec, exec, vistrails
        where
           exec.module_name = %s and
           wf_exec.wf_exec_id = exec.wf_exec_id and
           vistrails.vistrails_name = %s and
           vistrails.vistrails_id = wf_exec.vistrails_id""", ('AlignWarp', name))

        result = []
        for ts_start, ts_end, exec_id, module_id, wf_version in c.fetchall():
            p = vistrail.getPipeline(int(wf_version))
            m = p.modules[module_id]
            assert m.name == 'AlignWarp'
            # We assume here that no module takes longer than a day to execute.
            wd = 'monday'
            if self.weekday(ts_start, wd) or self.weekday(ts_end, wd):
                result.append((wf_version, module_id, exec_id))

        self.queryResult = result
        self.tupleLength = 3
        self.computeIndices()
        return result

class Query5(Query):

    @memo_method
    def pipeline(self, vistrail, version):
        return vistrail.getPipeline(version)

    @memo_method
    def upstream_set(self, graph, m_id):
        return set(self.upstream(graph, m_id))

    def run(self, vistrail, name):
        import gui.vis_application
        c = gui.vis_application.logger.db.cursor()
        c.execute("""
        select distinct
           wf_exec.wf_version, exec.module_id
        from
           wf_exec, exec, annotation, vistrails
        where
           wf_exec.wf_exec_id = exec.wf_exec_id and
           vistrails.vistrails_name = %s and
           vistrails.vistrails_id = wf_exec.vistrails_id and
           annotation.exec_id = exec.exec_id and
           annotation.key = %s and
           annotation.value = %s""", (name, 'global maximum', '4095'))
        result = []
        all = c.fetchall()
        print len(all)
        version_module = {}
        for wf_version, module_id in all:
            if not wf_version in version_module:
                version_module[wf_version] = set([module_id])
            else:
                version_module[wf_version].add(module_id)
        print vistrail.tagMap
        for wf_version, module_ids in version_module.iteritems():
            print wf_version
            p = self.pipeline(vistrail, int(wf_version))
            inv_graph = p.graph.inverse()
            s = set()
            for m_id, module in p.modules.iteritems():
                if module.name == 'FileSink':
                    for f in module.functions:
                        if (f.name == 'outputName' and
                            len(f.params) == 1 and
                            f.params[0].value().find('atlas') != -1):
                            u_id = self.upstream_set(inv_graph, m_id)
                            if module_ids.intersection(u_id):
                                s = s.union(u_id).union(set([m_id]))
                            break
            for m in s:
                result.append((int(wf_version), m))
        self.queryResult = result
        self.tupleLength = 2
        self.computeIndices()
        return result
        

class Query6(Query):

    def run(self, vistrail, name):
        import gui.vis_application
        c = gui.vis_application.logger.db.cursor()
        c.execute("""
        select distinct module_id, wf_version from
        wf_exec, exec, vistrails
        where
           exec.module_name = %s and
           wf_exec.wf_exec_id = exec.wf_exec_id and
           vistrails.vistrails_name = %s and
           vistrails.vistrails_id = wf_exec.vistrails_id""", ('SoftMean', name))
        result = []
        for (module_id, wf_version) in c.fetchall():
            p = vistrail.getPipeline(int(wf_version))
            m = p.modules[module_id]
            assert m.name == 'SoftMean'
            inv_graph = p.graph.inverse()
            up = self.upstream(inv_graph, module_id)
            up.append(module_id)
            found = False
            for up_id in up:
                up_module = p.modules[up_id]
                if up_module.name == 'AlignWarp':
                    for f in up_module.functions:
                        if (f.name == 'model' and
                            len(f.params) == 1 and
                            f.params[0].value() == 12):
                            found = True
                            break
                if found:
                    break
            if found:
                for up_id in up:
                    result.append((wf_version, up_id))
        self.queryResult = result
        self.tupleLength = 2
        self.computeIndices()
        return result


class Query8(Query):

    def run(self, vistrail, name):
        result = []
        versions = vistrail.tagMap.values()
        for version in versions:
            s = set()
            p = vistrail.getPipeline(version)
            inv_graph = p.graph.inverse()
            for module_id, module in p.modules.iteritems():
                if module.name == 'AlignWarp':
                    found = False
                    u_ids = self.upstream(inv_graph, module_id)
                    for i in u_ids:
                        if (p.modules[i].has_annotation_with_key('center')
                            and p.modules[i].get_annotation_by_key('center') \
                                == 'UChicago'):
                            found = True
                            break
                    if found:
                        s = s.union(set(u_ids + [module_id]))
            for m in s:
                result.append((version, m))
        self.queryResult = result
        self.tupleLength = 2
        self.computeIndices()
        return result


class Query9(Query):

    def run(self, vistrail, name):
        result = []
        versions = vistrail.tagMap.values()
        for version in versions:
            s = set()
            p = vistrail.getPipeline(version)
            inv_graph = p.graph.inverse()
            for module_id, module in p.modules.iteritems():
                annot = module.annotations
                if (module.has_annotation_with_key('studyModality')
                    and module.get_annotation_by_key('studyModality') in \
                        ['visual', 'audio', 'speech']):
                    s = s.union(set(self.upstream(inv_graph, module_id) + [module_id]))
            for m in s:
                result.append((version, m))
        self.queryResult = result
        self.tupleLength = 2
        self.computeIndices()
        return result
