r"""web_helper is a module that provides access to functions that helps to build
new protocols and process ParaView data structure into web friendly ones.
"""

import types
import sys
import os
import traceback

# import paraview modules.
import paraview

from paraview import simple, servermanager
from paraview.servermanager import ProxyProperty, InputProperty
from paraview.modules.vtkRemotingViews import vtkSMPVRepresentationProxy

PY3 = False
if sys.version_info >= (3,):
    xrange = range
    PY3 = True

from vtkmodules.web import buffer

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
        if pid in self.children_ids:
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

    def isEmpty(self):
      return len(self.parent_ids) == 1

    # --------------------------------------------------------------------------

    def getRootNode(self, view = None):
        """
        Create a tree structure of the pipeline with the current proxy state.
        """
        self.root_node['children'] = []
        self.__fill_children(self.root_node, self.children_ids['0'], view)
        return self.root_node

    # --------------------------------------------------------------------------

    def __fill_children(self, nodeToFill, childrenIds, view = None):
        for id in childrenIds:
            node = getProxyAsPipelineNode(id, view)
            nid = str(node['proxy_id'])

            if 'children' in nodeToFill:
                nodeToFill['children'].append(node)
            else:
                nodeToFill['children'] = [ node ]

            if nid in self.children_ids:
                self.__fill_children(node, self.children_ids[nid]);

# =============================================================================
# Proxy management
# =============================================================================

def idToProxy(id):
    """
    Return the proxy that match the given proxy ID.
    """
    remoteObject = simple.servermanager.ActiveConnection.Session.GetRemoteObject(int(id))
    if remoteObject:
        return simple.servermanager._getPyProxy(remoteObject)
    return None

# --------------------------------------------------------------------------

def getParentProxyId(proxy):
    """
    Return '0' if the given proxy has no Input otherwise will return
    the parent proxy id as a String.
    """
    if proxy and proxy.GetProperty("Input"):
      parentProxy = proxy.GetProperty("Input").GetProxy(0)
      if parentProxy:
        return parentProxy.GetGlobalIDAsString()
    return '0'

# --------------------------------------------------------------------------

def getProxyAsPipelineNode(id, view=None):
    """
    Create a representation for that proxy so it can be used within a pipeline
    browser.
    """
    pxm = servermanager.ProxyManager()
    proxy = idToProxy(id)
    rep = simple.GetDisplayProperties(proxy)
    nbActiveComp = 1

    pointData = []
    searchArray = ('POINTS' == rep.ColorArrayName[0]) and (len(rep.ColorArrayName[1]) > 0)

    if servermanager.ActiveConnection.GetNumberOfDataPartitions() > 1:
        info = {                  \
        'lutId': 'vtkProcessId_1', \
        'name': 'vtkProcessId',     \
        'size': 1,                   \
        'range': [0, servermanager.ActiveConnection.GetNumberOfDataPartitions()-1] }
        pointData.append(info)

    # FIXME seb
    # dataInfo = rep.GetRepresentedDataInformation()
    # pointData = dataInfo.GetPointDataInformation()
    # cellData = dataInfo.GetCellDataInformation()
    # for idx in pointData.GetNumberOfArrays():
    #     info = pointData.GetArrayInformation(idx)
    #     nbComponents = info.GetNumberOfComponents()
    #     if searchArray and array.Name == rep.ColorArrayName:
    #         nbActiveComp = nbComponents
    #     rangeOn = (nbComponents == 3 if -1 else 0)
    #     info = {                                      \
    #     'lutId': info.GetName() + '_' + str(nbComponents), \
    #     'name': info.GetName,                             \
    #     'size': nbComponents,                            \
    #     'range': info.GetRange(rangeOn) }
    #     pointData.append(info)

    for array in proxy.GetPointDataInformation():
        nbComponents = array.GetNumberOfComponents()
        if searchArray and array.Name == rep.ColorArrayName[1]:
            nbActiveComp = nbComponents
        rangeOn = (nbComponents == 1 if 0 else -1)
        info = {                                      \
        'lutId': array.Name + '_' + str(nbComponents), \
        'name': array.Name,                             \
        'size': nbComponents,                            \
        'range': array.GetRange(rangeOn) }
        pointData.append(info)

    cellData = []
    searchArray = ('CELLS' == rep.ColorArrayName[0]) and (len(rep.ColorArrayName[1]) > 0)
    for array in proxy.GetCellDataInformation():
        nbComponents = array.GetNumberOfComponents()
        if searchArray and array.Name == rep.ColorArrayName[1]:
            nbActiveComp = nbComponents
        rangeOn = (nbComponents == 1 if 0 else -1)
        info = {                                      \
        'lutId': array.Name + '_' + str(nbComponents), \
        'name': array.Name,                             \
        'size': nbComponents,                            \
        'range': array.GetRange(rangeOn) }
        cellData.append(info)

    state = getProxyAsState(proxy.GetGlobalID())
    showScalarbar = 1 if view and vtkSMPVRepresentationProxy.IsScalarBarVisible(rep.SMProxy, view.SMProxy) else 0

    repName = 'Hide'
    if rep.Visibility == 1:
        repName = rep.Representation

    return { 'proxy_id'  : proxy.GetGlobalID(),                               \
             'name'      : pxm.GetProxyName("sources", proxy),                \
             'bounds'    : proxy.GetDataInformation().GetBounds(),            \
             'pointData' : pointData,                                         \
             'cellData'  : cellData,                                          \
             'activeData': str(rep.ColorArrayName[0]) + ':' + str(rep.ColorArrayName[1]), \
             'diffuseColor'  : str(rep.DiffuseColor),                         \
             'showScalarBar' : showScalarbar,                                 \
             'representation': repName,                                       \
             'state'         : state,                                         \
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
    state = { 'proxy_id': proxy_id , 'type': 'proxy', 'domains': getProxyDomains(proxy_id)}
    properties = {}
    allowedTypes = [int, float, list, str]
    if proxy:
        for property in proxy.ListProperties():
            propertyName = proxy.GetProperty(property).Name
            if propertyName in ["Refresh", "Input"] or propertyName.__contains__("Info"):
                continue

            data = proxy.GetProperty(property).GetData()
            if type(data) in allowedTypes:
                properties[propertyName] = data
                continue

            # Not a simple property
            # Need more investigation
            prop = proxy.GetProperty(property)
            pythonProp = servermanager._wrap_property(proxy, prop)
            proxyList = []
            try:
                proxyList = pythonProp.Available
            except:
                pass
            if len(proxyList) and prop.GetNumberOfProxies() == 1:
                listdomain = prop.FindDomain("vtkSMProxyListDomain")
                if listdomain:
                    proxyPropertyValue = prop.GetProxy(0)
                    for i in xrange(listdomain.GetNumberOfProxies()):
                        if listdomain.GetProxy(i) == proxyPropertyValue:
                            properties[propertyName] = proxyList[i]
                            # Add selected proxy in list of prop to edit
                            properties[propertyName + '_internal'] = getProxyAsState(listdomain.GetProxy(i).GetGlobalID())

            elif type(prop) == ProxyProperty:
                try:
                    subProxyId = proxy.GetProperty(property).GetData().GetGlobalID()
                    properties[propertyName] = getProxyAsState(subProxyId)
                except:
                    print ("Error on", property, propertyName)
                    print ("Skip property: ", str(type(data)))
                    print (data)
    state['properties'] = properties;
    return state

# --------------------------------------------------------------------------

def updateProxyProperties(proxy, properties):
   """
   Loop over the properties object and update the mapping properties
   to the given proxy.
   """
   try:
       allowedProperties = proxy.ListProperties()
       for key in properties:
          validKey = servermanager._make_name_valid(key)
          if validKey in allowedProperties:
             value = removeUnicode(properties[key])
             property = servermanager._wrap_property(proxy, proxy.GetProperty(validKey))
             if property.FindDomain("vtkSMProxyListDomain") and len(value) == 1 and type(value[0]) == str:
                 try:
                    idx = property.GetAvailable().index(value[0])
                    proxyToSet = servermanager._getPyProxy(property.FindDomain("vtkSMProxyListDomain").GetProxy(idx))
                    property.SetData(proxyToSet)
                 except:
                    traceback.print_stack()
                    pass
             elif value == 'vtkProcessId':
                property.SetElement(0, value)
             else:
                property.SetData(value)
   except:
        traceback.print_stack()

# --------------------------------------------------------------------------

def removeUnicode(value):
    # python 3 is using str everywhere already.
    if PY3: return value
    if type(value) == unicode:
        return str(value)
    if type(value) == list:
        result = []
        for v in value:
            result.append(removeUnicode(v))
        return result
    return value

# =============================================================================
# XML and Proxy Definition for GUI generation
# =============================================================================

def getProxyDomains(id):
   """
   Return a json based structured based on the proxy XML.
   """
   jsonDefinition = {}
   proxy = idToProxy(id)
   xmlElement = servermanager.ActiveConnection.Session.GetProxyDefinitionManager().GetCollapsedProxyDefinition(proxy.GetXMLGroup(), proxy.GetXMLName(), None)
   nbChildren = xmlElement.GetNumberOfNestedElements()
   for i in range(nbChildren):
       xmlChild = xmlElement.GetNestedElement(i)
       name = xmlChild.GetName()
       if name.__contains__('Property'):
           propName = xmlChild.GetAttribute('name')
           jsonDefinition[propName] = extractProperty(proxy, xmlChild)
           jsonDefinition[propName]['order'] = i

   # Look for proxy properties and their domain
   orderIndex = nbChildren
   for property in proxy.ListProperties():
       if property == 'Input':
           continue
       if type(proxy.GetProperty(property)) == ProxyProperty:
           try:
               subProxyId = proxy.GetProperty(property).GetData().GetGlobalID()
               subDomain = getProxyDomains(subProxyId)
               for key in subDomain:
                   jsonDefinition[key] = subDomain[key]
                   jsonDefinition[key]['order'] = orderIndex
                   orderIndex = orderIndex + 1
           except:
               print ("(Def) Error on", property, ", skipping it...")
               #print ("(Def) Skip property: ", str(type(data)))

   return jsonDefinition

def extractProperty(proxy, xmlPropertyElement):
    propInfo = {}
    propInfo['name'] = xmlPropertyElement.GetAttribute('name')
    propInfo['label'] = xmlPropertyElement.GetAttribute('label')
    if xmlPropertyElement.GetAttribute('number_of_elements') != None:
        propInfo['size'] = xmlPropertyElement.GetAttribute('number_of_elements')
    propInfo['type'] = xmlPropertyElement.GetName()[:-14]
    propInfo['panel_visibility'] = xmlPropertyElement.GetAttribute('panel_visibility')
    propInfo['number_of_elements'] = xmlPropertyElement.GetAttribute('number_of_elements')
    propInfo['domains'] = []
    if xmlPropertyElement.GetAttribute('default_values') != None:
        propInfo['default_values'] = xmlPropertyElement.GetAttribute('default_values')
    nbChildren = xmlPropertyElement.GetNumberOfNestedElements()
    for i in range(nbChildren):
        xmlChild = xmlPropertyElement.GetNestedElement(i)
        name = xmlChild.GetName()
        if name.__contains__('Domain'):
            propInfo['domains'].append(extractDomain(proxy, propInfo['name'], xmlChild))
    return propInfo

def extractDomain(proxy, propertyName, xmlDomainElement):
    domainObj = {}
    name = xmlDomainElement.GetName()
    domainObj['type'] = name[:-6]

    # Handle Range
    if name.__contains__('RangeDomain'):
        if xmlDomainElement.GetAttribute('min') != None:
            domainObj['min'] = xmlDomainElement.GetAttribute('min')
        if xmlDomainElement.GetAttribute('max') != None:
            domainObj['max'] = xmlDomainElement.GetAttribute('max')

    # Handle Enum
    if name.__contains__('EnumerationDomain'):
        domainObj['enum'] = []
        nbChildren = xmlDomainElement.GetNumberOfNestedElements()
        for i in range(nbChildren):
            xmlChild = xmlDomainElement.GetNestedElement(i)
            if xmlChild.GetName() == "Entry":
                domainObj['enum'].append({'text': xmlChild.GetAttribute('text'), 'value': xmlChild.GetAttribute('value')})

    # Handle ArrayListDomain
    if name.__contains__('ArrayListDomain'):
        dataType = xmlDomainElement.GetAttribute('attribute_type')
        if dataType == 'Scalars':
            domainObj['nb_components'] = 1
        elif dataType == 'Vectors':
            domainObj['nb_components'] = 3
        else:
            domainObj['nb_components'] = -1

    # Handle ProxyListDomain
    if name.__contains__('ProxyListDomain'):
        domainObj['list'] = proxy.GetProperty(propertyName).Available

    # Handle Bounds
    if name.__contains__('BoundsDomain'):
        for attrName in ['default_mode', 'mode', 'scale_factor']:
            try:
                attrValue = xmlDomainElement.GetAttribute(attrName)
                if attrValue:
                    domainObj[attrName] = attrValue
            except:
                pass

    return domainObj

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
    if pathToList[-1] == '/':
       pathToList = pathToList[:-1]
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


# =============================================================================
# Apply domains
# =============================================================================

def apply_domains(parentProxy, proxy_id):
    """
    Handle bounds domain
    """
    proxy = idToProxy(proxy_id)

    # Call recursively on each sub-proxy if any
    for property_name in proxy.ListProperties():
        prop = proxy.GetProperty(property_name)
        if prop.IsA('vtkSMProxyProperty'):
            try:
                if len(prop.Available) and prop.GetNumberOfProxies() == 1:
                    listdomain = prop.FindDomain("vtkSMProxyListDomain")
                    if listdomain:
                        for i in xrange(listdomain.GetNumberOfProxies()):
                            internal_proxy = listdomain.GetProxy(i)
                            apply_domains(parentProxy, internal_proxy.GetGlobalIDAsString())
            except:
                exc_type, exc_obj, exc_tb = sys.exc_info()
                print ("Unexpected error:", exc_type, " line: " , exc_tb.tb_lineno)

    # Reset all properties to leverage domain capabilities
    for prop_name in proxy.ListProperties():
        try:
            prop = proxy.GetProperty(prop_name)
            iter = prop.NewDomainIterator()
            iter.Begin()
            while not iter.IsAtEnd():
                domain = iter.GetDomain()
                iter.Next()

                try:
                    if domain.IsA('vtkSMBoundsDomain'):
                        domain.SetDomainValues(parentProxy.GetDataInformation().GetBounds())
                except AttributeError as attrErr:
                    print ('Caught exception setting domain values in apply_domains:')
                    print (attrErr)

            prop.ResetToDefault()

            # Need to UnRegister to handle the ref count from the NewDomainIterator
            iter.UnRegister(None)
        except:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            print ("Unexpected error:", exc_type, " line: " , exc_tb.tb_lineno)

    proxy.UpdateVTKObjects()
