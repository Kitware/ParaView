r"""web_helper is a module that provides access to functions that helps to build
new protocols and process ParaView data structure into web friendly ones.
"""

import types
import sys
import os

# import paraview modules.
from paraview import simple, servermanager
from paraview.servermanager import ProxyProperty

# =============================================================================
# Pipeline management
# =============================================================================

class Pipeline:
    """
    Define a data structure that represent a pipeline as a tree.
    This provide also methods to get the data structure for the web environment
    """

    # --------------------------------------------------------------------------

    def __init__(self, name):
        self.root_node = { 'name': name, 'icon': 'server', 'children': [] }
        self.parent_ids = { '0':'0' }
        self.children_ids = { '0':[] }

    # --------------------------------------------------------------------------

    def clear(self):
        """
        Clear the pipeline tree.
        """
        self.root_node['children'] = []
        self.parent_ids = { '0':'0' }
        self.children_ids = { '0':[] }

    # --------------------------------------------------------------------------

    def addNode(self, parent_id, node_id):
        """
        Add node into the pipeline tree.
        """
        pid = str(parent_id)
        nid = str(node_id)

        # Add child
        if self.children_ids.has_key(pid):
            self.children_ids[pid].append(nid)
        else:
            self.children_ids[pid] = [nid]

        # Add parent
        self.parent_ids[nid] = pid

    # --------------------------------------------------------------------------

    def removeNode(self, id):
        """
        Remove a node from the pipeline tree.
        """
        nid = str(id)
        pid = self.parent_ids[nid]
        if pid:
            del self.parent_ids[nid]
            self.children_ids[pid].remove(nid)

    # --------------------------------------------------------------------------

    def getRootNode(self):
        """
        Create a tree structure of the pipeline with the current proxy state.
        """
        self.root_node['children'] = []
        self.__fill_children(self.root_node, self.children_ids['0'])
        return self.root_node

    # --------------------------------------------------------------------------

    def __fill_children(self, nodeToFill, childrenIds):
        for id in childrenIds:
            node = getProxyAsPipelineNode(id)
            nid = str(node['proxy_id'])

            if nodeToFill.has_key('children'):
                nodeToFill['children'].append(node)
            else:
                nodeToFill['children'] = [ node ]

            if self.children_ids.has_key(nid):
                self.__fill_children(node, self.children_ids[nid]);

# =============================================================================
# Proxy management
# =============================================================================

def idToProxy(id):
    """
    Return the proxy that match the given proxy ID.
    """
    return simple.servermanager._getPyProxy(simple.servermanager.ActiveConnection.Session.GetRemoteObject(int(id)))

# --------------------------------------------------------------------------

def getProxyAsPipelineNode(id):
    """
    Create a representation for that proxy so it can be used within a pipeline
    browser.
    """
    pxm = servermanager.ProxyManager()
    proxy = idToProxy(id)
    rep = simple.GetDisplayProperties(proxy)

    pointData = []
    for array in proxy.GetPointDataInformation():
       pointData.append(array.Name)

    cellData = []
    for array in proxy.GetCellDataInformation():
       cellData.append(array.Name)

    return { 'proxy_id'   : proxy.GetGlobalID(),                               \
             'name'       : pxm.GetProxyName("sources", proxy),                \
             'pointData'  : pointData,                                         \
             'cellData'   : cellData,                                          \
             'activeData' : rep.ColorAttributeType + ':' + rep.ColorArrayName, \
             'showScalarBar' : False,                                          \
             'representation': rep.Representation,                             \
             'state'         : getProxyAsState(proxy.GetGlobalID()),             \
             'children'      : [] }

# --------------------------------------------------------------------------

def getProxyAsState(id):
    """
    Return a json representation of the given proxy state.

    Example of the state of the Clip filter
      {
         proxy_id: 234,
         ClipType: {
            proxy_id: 235,
            Normal: [0,0,1],
            Origin: [0,0,0],
            InsideOut: 0
         }
      }
    """
    proxy_id = int(id)
    proxy = idToProxy(proxy_id)
    state = { 'proxy_id': proxy_id , 'type': 'proxy'}
    allowedTypes = [int, float, list]
    if proxy:
       for property in proxy.ListProperties():
          if property in ["Refresh"] or property.__contains__("Info"):
             continue

          if type(proxy.GetProperty(property)) != ProxyProperty:
             if proxy.GetProperty(property).GetData() and type(proxy.GetProperty(property).GetData()) in allowedTypes:
                state[property] = proxy.GetProperty(property).GetData()
             elif proxy.GetProperty(property).GetData():
                state[property] = getProxyAsState(proxy.GetProperty(property).GetData().GetGlobalID())
    return state

# --------------------------------------------------------------------------

def updateProxyProperties(proxy, properties):
   """
   Loop over the properties object and update the mapping properties
   to the given proxy.
   """
   allowedProperties = proxy.ListProperties()
   for key in properties:
      if key in allowedProperties:
         value = properties[key]
         if type(value) == unicode:
            value = str(value)
         proxy.GetProperty(key).SetData(value)

# =============================================================================
# XML and Proxy Definition for GUI generation
# =============================================================================

def getProxyDefinition(id):
   """
   Return a json based structured based on the proxy XML.
   """
   proxy = idToProxy(id)
   xmlElement = servermanager.ActiveConnection.Session.GetProxyDefinitionManager().GetCollapsedProxyDefinition(proxy.GetXMLGroup(), proxy.GetXMLName(), None)
   #FIXME need to parse that xml and generate json annotation
   return {}

# =============================================================================
# File Management
# =============================================================================

def listFiles(pathToList):
    """
    Create a tree structure of the given directory that will be understand by
    the pipelineBrowser widget.
    The provided path should not have a trailing '/'.

    return {
       children: [
           { name: 'fileName.vtk', path: '/full_path/to_file/fileName.vtk' },
           { name: 'directoryName', path: '/full_path/to_file/directoryName', children: [] }
       ]
    }
    """
    global fileList
    nodeTree = {}
    nodeTree[pathToList] = {'children': []}
    for path, directories, files in os.walk(pathToList):
        parent = nodeTree[path]
        for directory in directories:
            child = {'name': directory , 'path': (path + '/' + directory), 'children': []}
            nodeTree[path + '/' + directory] = child
            parent['children'].append(child)
        for filename in files:
            child = {'name': filename, 'path': (path + '/' + filename) }
            nodeTree[path + '/' + filename] = child
            parent['children'].append(child)
    fileList = nodeTree[pathToList]['children']
    return fileList
