#==============================================================================
# Copyright (c) 2015,  Kitware Inc., Los Alamos National Laboratory
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this
# list of conditions and the following disclaimer in the documentation and/or other
# materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors may
# be used to endorse or promote products derived from this software without specific
# prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
# OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#==============================================================================
import cinema_store
import paraview
import pv_explorers
from itertools import imap

import numpy as np

def record_visibility():
    proxies = []

    view_info = {}
    view_proxy = paraview.simple.GetActiveView()

    view_info['proxy'] = "__view_info"
    view_info[
        'orientation_axis_visibility'] = view_proxy.OrientationAxesVisibility

    camera = view_proxy.GetActiveCamera()
    view_info['position'] = camera.GetPosition()
    view_info['view_up'] = camera.GetViewUp()
    view_info['focal_point'] = camera.GetFocalPoint()
    proxies.append(view_info)

    source_proxies = paraview.servermanager.ProxyManager().GetProxiesInGroup(
        "sources")

    for key in source_proxies:
        listElt = {}
        proxy = source_proxies[key]
        listElt['proxy'] = proxy
        listElt['visibility'] = None
        listElt['scalar_bar_visibility'] = False
        listElt['color_array_name'] = None
        listElt['color_array_association'] = None
        rep = paraview.simple.GetDisplayProperties(proxy)
        if rep != None:
            listElt['visibility'] = rep.Visibility
            listElt['scalar_bar_visibility'] = rep.IsScalarBarVisible(view_proxy)
            listElt['color_array_name'] = rep.ColorArrayName.GetArrayName()
            listElt['color_array_association'] = rep.ColorArrayName.GetAssociation()
        proxies.append(listElt)
    return proxies

def restore_visibility(proxies):
    view_proxy = paraview.simple.GetActiveView()

    for listElt in proxies:
        if listElt['proxy'] == "__view_info":
            view_proxy.OrientationAxesVisibility = listElt[
                'orientation_axis_visibility']

            camera = view_proxy.GetActiveCamera()
            camera.SetPosition(listElt['position'])
            camera.SetViewUp(listElt['view_up'])
            camera.SetFocalPoint(listElt['focal_point'])
        else:
            proxy = listElt['proxy']
            vis = listElt['visibility']
            if vis != None:
                rep = paraview.simple.GetDisplayProperties(proxy)
                if rep != None:
                    rep.Visibility = listElt['visibility']
                    if listElt['color_array_association']:
                        rep.SetScalarColoring(
                            listElt['color_array_name'],
                            paraview.servermanager.GetAssociationFromString(
                                listElt['color_array_association']))
                    if listElt['scalar_bar_visibility']:
                        rep.SetScalarBarVisibility(view_proxy,
                                            listElt['scalar_bar_visibility'])


def inspect(skip_invisible=True):
    """
    Produces a representation of the pipeline that is easier to work with.
    Thanks Scott Wittenburg and the pv mailing list for this gem
    """
    source_proxies = paraview.servermanager.ProxyManager().GetProxiesInGroup("sources")
    proxies = []
    proxybyId = {}
    for key in source_proxies:
        listElt = {}
        listElt['name'] = key[0]
        listElt['id'] = key[1]
        proxy = source_proxies[key]

        #skip the invisible
        rep = paraview.simple.GetDisplayProperties(proxy)
        if skip_invisible:
            if rep == None:
                #for example, writers in catalyst pipeline
                #todo: is it possible for these to have decendents that are visible?
                continue

        listElt['visibility'] = 0 if (rep == None) else rep.Visibility

        parentId = '0'
        try:
            if hasattr(proxy, 'Input'):
                parentId = proxy.Input.GetGlobalIDAsString()
        except AttributeError:
            parentId = '0'
        listElt['parent'] = parentId
        proxies.append(listElt)
        proxybyId[key[1]] = listElt

    if skip_invisible:
        #reparent upward over invisible parents
        for l in proxies:
            pid = l['parent']
            while pid != '0' and proxybyId[pid]['visibility'] == 0:
                pid = proxybyId[pid]['parent']
            l['parent'] = pid

        #remove invisible proxies themselves
        pxies = []
        for l in proxies:
            if l['visibility'] != 0:
                pxies.append(l)
    else:
        pxies = proxies

    return pxies

def get_pipeline():
    proxies = inspect(skip_invisible=False)
    for proxy in proxies:
        source = paraview.simple.FindSource(proxy['name'])
        numberOfProducers = source.GetNumberOfProducers()
        if proxy['parent'] is '0' and numberOfProducers > 0:
            # this proxy is the result of a merge
            parents = []
            for i in xrange(numberOfProducers):
                parents.append(source.GetProducerProxy(i).GetGlobalIDAsString())
            proxy['parents'] = parents
        else:
            proxy['parents'] = [proxy['parent']]
        del proxy['parent']
    for proxy in proxies:
        proxy['children'] = [p['id'] for p in proxies
                             if proxy['id'] in p['parents']]
    return proxies

def float_limiter(x):
    #a shame, but needed to make sure python, java and (directory/file)name agree
    if isinstance(x, (float)):
        #return '%6f' % x #arbitrarily chose 6 decimal places
        return '%.6e' % x #arbitrarily chose 6 significant digits
    else:
        return x

# Keeps a link between a filter and its explorer-track. Populated in addFilterValue()
# and queried in explore()
explorerDir = {}

def add_filter_value(name, cs, userDefinedValues):
    source = paraview.simple.FindSource(name)

    # plane offset generator (for Slice or Clip)
    def generate_offset_values():
        bounds = source.Input.GetDataInformation().DataInformation.GetBounds()
        minPoint = np.array([bounds[0], bounds[2], bounds[4]])
        maxPoint = np.array([bounds[1], bounds[3], bounds[5]])
        scaleVec = maxPoint - minPoint

        # adjust offset size depending on the plane orientation
        if hasattr(source, 'SliceType'):
            n = source.SliceType.Normal
        elif hasattr(source, 'ClipType'):
            n = source.ClipType.Normal

        sNormal = np.array([n[0] * scaleVec[0], n[1] * scaleVec[1], n[2] * scaleVec[2]])

        steps = 3 # generate N slice offsets
        offsetStep = np.linalg.norm(sNormal) / steps
        values = np.arange(-(steps/2), steps/2) * offsetStep
        return values.tolist()

    # generate values depending on the type of filter
    if isinstance(source, paraview.simple.servermanager.filters.Clip):
        # grab values from ui or generate defaults
        values = userDefinedValues[name] if (name in userDefinedValues) else generate_offset_values()
        if len(values) == 0: values = generate_offset_values()

        # add sublayer and create the appropriate track
        cs.add_control(name, cinema_store.make_parameter(name, values, typechoice='hidden'))
        explorerDir[name] = pv_explorers.Clip(name, source)

    elif isinstance(source, paraview.simple.servermanager.filters.Slice):
        # grab values from ui or generate defaults
        values = userDefinedValues[name] if (name in userDefinedValues) else generate_offset_values()
        if len(values) == 0: values = generate_offset_values()

        # add sublayer and create the appropriate track
        cs.add_control(name, cinema_store.make_parameter(name, values, typechoice='hidden'))
        explorerDir[name] = pv_explorers.Slice(name, source)

    elif isinstance(source, paraview.simple.servermanager.filters.Contour):

        def generate_contour_values():
            # grab values from ui or generate defaults
            vRange = source.Input.GetDataInformation().DataInformation.GetPointDataInformation().GetArrayInformation(0).GetComponentRange(0)
            return np.linspace(vRange[0], vRange[1], 5).tolist() # generate 5 contour values

        values = userDefinedValues[name] if (name in userDefinedValues) else generate_contour_values()
        if len(values) == 0: values = generate_contour_values()

        # add sublayer and create the appropriate track
        cs.add_control(name, cinema_store.make_parameter(name, values, typechoice='hidden'))
        explorerDir[name] = pv_explorers.Contour(name, source)

def filter_has_parameters(name):
    source = paraview.simple.FindSource(name)
    return any(imap(lambda filter: isinstance(source, filter),
                    [paraview.simple.servermanager.filters.Clip,
                     paraview.simple.servermanager.filters.Slice,
                     paraview.simple.servermanager.filters.Contour]))

def add_control_and_colors(name, cs):
    source = paraview.simple.FindSource(name)
    #make up list of color options
    fields = {'depth':'depth','luminance':'luminance'}
    ranges = {}
    defaultName = None
    view_proxy = paraview.simple.GetActiveView()
    rep = paraview.simple.GetRepresentation(source, view_proxy)
    if rep.Representation != 'Outline':
        cda = source.GetCellDataInformation()
        for a in range(0, cda.GetNumberOfArrays()):
            arr = cda.GetArray(a)
            arrName = arr.GetName()
            if not arrName == "Normals":
                for i in range(0, arr.GetNumberOfComponents()):
                    fName = arrName+"_"+str(i)
                    fields[fName] = 'value'
                    ranges[fName] = arr.GetRange(i)
                    if defaultName == None:
                        defaultName = fName
        pda = source.GetPointDataInformation()
        for a in range(0, pda.GetNumberOfArrays()):
            arr = pda.GetArray(a)
            arrName = arr.GetName()
            if not arrName == "Normals":
                for i in range(0, arr.GetNumberOfComponents()):
                    fName = arrName+"_"+str(i)
                    fields[fName] = 'value'
                    ranges[fName] = arr.GetRange(i)
                    if defaultName == None:
                        defaultName = fName
    if defaultName == None:
        fields['white']='rgb'
        defaultName='white'
    cparam = cinema_store.make_field("color"+name, fields, default=defaultName, valueRanges=ranges)
    cs.add_field("color"+name,cparam,'vis',[name])

def make_cinema_store(proxies, ocsfname, forcetime=False, _userDefinedValues={}):
    """
    Takes in the pipeline, structured as a tree, and makes a cinema store definition
    containing all the parameters we might will vary.
    """

    if "phi" in _userDefinedValues:
        phis = _userDefinedValues["phi"]
    else:
        #phis = [0,45,90,135,180,225,270,315,360]
        phis = [0,180,360]

    if "theta" in _userDefinedValues:
        thetas = _userDefinedValues["theta"]
    else:
        #thetas = [0,20,40,60,80,100,120,140,160,180]
        thetas = [0,90,180]

    tvalues = []
    cs = cinema_store.FileStore(ocsfname)
    try:
        cs.load()
        tprop = cs.get_parameter('time')
        tvalues = tprop['values']
        #start with clean slate, other than time
        cs = cinema_store.FileStore(ocsfname)
    except IOError, KeyError:
        pass

    cs.add_metadata({'type':'composite-image-stack'})
    cs.add_metadata({'store_type':'FS'})
    cs.add_metadata({'version':'0.0'})
    pipeline = get_pipeline()
    cs.add_metadata({'pipeline':pipeline})

    vis = [proxy['name'] for proxy in proxies]
    cs.add_layer("vis",cinema_store.make_parameter('vis', vis))

    for proxy in proxies:
        proxy_name = proxy['name']
        add_filter_value(proxy_name,cs,_userDefinedValues)
        dependency_set = set([proxy['id']])
        repeat = True
        while repeat:
            repeat = False
            deps = set(proxy['id'] for proxy in proxies if proxy['parent'] in dependency_set)
            if deps - dependency_set:
                dependency_set = dependency_set.union(deps)
                repeat = True
        dependency_list = [proxy['name'] for proxy in proxies if proxy['id'] in dependency_set]
        cs.assign_parameter_dependence(proxy_name,'vis',dependency_list)
        add_control_and_colors(proxy_name,cs)
        cs.assign_parameter_dependence("color"+proxy_name,'vis',[proxy_name])

    fnp = ""
    if forcetime:
        #time specified, use it, being careful to append if already a list
        tvalues.append(forcetime)
        tprop = cinema_store.make_parameter('time', tvalues)
        cs.add_parameter('time', tprop)
        fnp = fnp+"{time}_"
    else:
        #time not specified, try and make them automatically
        times = paraview.simple.GetAnimationScene().TimeKeeper.TimestepValues
        if not times:
            pass
        else:
            prettytimes = [float_limiter(t) for t in times]
            cs.add_parameter("time", cinema_store.make_parameter('time', prettytimes))
            fnp = fnp+"{time}_"
    cs.add_parameter("phi", cinema_store.make_parameter('phi', phis))
    cs.add_parameter("theta", cinema_store.make_parameter('theta', thetas))
    fnp = fnp+"{phi}_{theta}.png"

    cs.filename_pattern = fnp
    return cs


def testexplore(cs):
    """
    For debugging, takes in the cinema store and prints out everything that we'll take snapshots off
    """
    import explorers
    import copy
    class printer(explorers.Explorer):
        def execute(self, desc):
            p = copy.deepcopy(desc)
            x = 'phi'
            if x in p.keys():
                print x, ":", desc[x], ",",
                del p[x]
            x = 'theta'
            if x in p.keys():
                print x, ":", desc[x], ",",
                del p[x]
            for x in sorted(p.keys()):
                print x, ":", p[x], ",",
            print

    params = cs.parameter_list.keys()
    e = printer(cs, params, [])
    e.explore()


def explore(cs, proxies, iSave=True, currentTime=None):
    """
    Takes in the store, which contains only the list of parameters,
    """
#    import pv_explorers
    import explorers

    view_proxy = paraview.simple.GetActiveView()
    dist = paraview.simple.GetActiveCamera().GetDistance()

    #associate control points wlth parameters of the data store
    cam = pv_explorers.Camera([0,0,0], [0,1,0], dist, view_proxy)

    params = cs.parameter_list.keys()

    tracks = []
    tracks.append(cam)

    cols = []

    ctime_float=None
    if currentTime:
        ctime_float = float(currentTime['time'])

    #hide all annotations
    view_proxy.OrientationAxesVisibility = 0

    for x in proxies:
        name = x['name']
        for y in params:

            if (y in explorerDir) and (name == y):
                #print "name in ExplorerDir: ", y, ", ", explorerDir[y]
                tracks.append(explorerDir[y])

            if name in y:
                #print "N", name
                #print "X", x
                #print "Y", y

                #visibility of the layer
                sp = paraview.simple.FindSource(name)
                rep = paraview.simple.GetRepresentation(sp, view_proxy)

                #hide all annotations
                if rep.LookupTable:
                    rep.SetScalarBarVisibility(view_proxy, False)
                tc1 = pv_explorers.SourceProxyInLayer(name, rep)
                lt = explorers.Layer('vis', [tc1])
                tracks.append(lt)

                #fields for the layer
                cC = pv_explorers.ColorList()
                cC.AddDepth('depth')
                cC.AddLuminance('luminance')
                sp.UpdatePipeline(ctime_float)
                cda = sp.GetCellDataInformation()

                numVals = 0
                if rep.Representation != 'Outline':
                    for a in range(0, cda.GetNumberOfArrays()):
                        arr = cda.GetArray(a)
                        arrName = arr.GetName()
                        if not arrName == "Normals":
                            for i in range(0,arr.GetNumberOfComponents()):
                                numVals+=1
                                cC.AddValueRender(arrName+"_"+str(i),
                                                True,
                                                arrName,
                                                i, arr.GetRange(i))
                    pda = sp.GetPointDataInformation()
                    for a in range(0, pda.GetNumberOfArrays()):
                        arr = pda.GetArray(a)
                        arrName = arr.GetName()
                        if not arrName == "Normals":
                            for i in range(0,arr.GetNumberOfComponents()):
                                numVals+=1
                                cC.AddValueRender(arrName+"_"+str(i),
                                                False,
                                                arrName,
                                                i, arr.GetRange(i))
                if numVals == 0:
                    cC.AddSolidColor('white', [1,1,1])
                col = pv_explorers.Color("color"+name, cC, rep)
                tracks.append(col)
                cols.append(col)

    e = pv_explorers.ImageExplorer(cs, params,
                                   tracks,
                                   view_proxy,
                                   iSave)

    for c in cols:
        c.imageExplorer = e

    times = paraview.simple.GetAnimationScene().TimeKeeper.TimestepValues
    if not times:
        e.explore(currentTime)
    else:
        for t in times:
            view_proxy.ViewTime=t
            e.explore({'time':float_limiter(t)})

def record(csname="/tmp/test_pv/info.json"):
    paraview.simple.Render()
    view = paraview.simple.GetActiveView()
    camera = view.GetActiveCamera()

    pxystate = record_visibility()

    view.LockBounds = 1

    p = inspect()
    cs = make_cinema_store(p, csname)
    #if test:
    #    testexplore(cs)
    #else:
    explore(cs, p)

    view.LockBounds = 0

    restore_visibility(pxystate)
    cs.save()

def export_scene(baseDirName, viewSelection, trackSelection):
    '''This explores a set of user-defined views and tracks. export_scene is
    called from vtkCinemaExport.  The expected order of parameters is as follows:

    - viewSelection (following the format defined in Wrapping/Python/paraview/cpstate.py):

    Directory  of the form {'ViewName' : [parameters], ...}, with parameters defined in the
    order:  Image filename, freq, fittoscreen, magnification, width, height, cinema).

    - trackSelection:

    Directory of the form {'TrackName' : [v1, v2, v3], ...}

    Note:  baseDirName is used as the parent directory of the database generated for
    each view in viewSelection. 'Image filename' is used as the database directory name.
    '''

    import paraview.simple as pvs

    # save initial state
    initialView = pvs.GetActiveView()
    pvstate = record_visibility()

    atLeastOneViewExported = False
    for viewName, viewParams in viewSelection.iteritems():

        # check if this view was selected to export as spec b
        cinemaParams = viewParams[6]
        if len(cinemaParams) == 0:
            print "Skipping view: Not selected to export as cinema spherical."
            continue

        # get the view and save the initial status
        view = pvs.FindView(viewName)
        pvs.SetActiveView(view)
        view.ViewSize = [viewParams[4], viewParams[5]]
        pvs.Render() # fully renders the scene (if not, some faces might be culled)
        view.LockBounds = 1

        #writeFreq = viewParams[1] # TODO where to get the timestamp in this case?
        #if (writeFreq and timestamp % writeFreq == 0):

        #magnification = viewParams[3] # Not used in cinema (TODO hide in UI)

        fitToScreen = viewParams[2]
        if fitToScreen != 0:
            if view.IsA("vtkSMRenderViewProxy") == True:
                view.ResetCamera()
            elif view.IsA("vtkSMContextViewProxy") == True:
                view.ResetDisplay()
            else:
                print ' do not know what to do with a ', view.GetClassName()

        userDefValues = {}
        if "theta" in cinemaParams:
            userDefValues["theta"] = cinemaParams["theta"]

        if "phi" in cinemaParams:
            userDefValues["phi"] = cinemaParams["phi"]

        userDefValues.update(trackSelection)

        # generate file path
        import os.path
        viewFileName = viewParams[0]
        viewDirName = viewFileName[0:viewFileName.rfind("_")] #strip _num.ext
        filePath = os.path.join(baseDirName, viewDirName, "info.json")

        p = inspect()
        cs = make_cinema_store(p, filePath, forcetime = False,
          _userDefinedValues = userDefValues)

        explore(cs, p)

        view.LockBounds = 0
        cs.save()
        atLeastOneViewExported = True

    if not atLeastOneViewExported:
        print "No view was selected to export as cinema spherical."
        return

    # restore initial state
    pvs.SetActiveView(initialView)
    restore_visibility(pvstate)
