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

import numpy as np

def record_visibility():
    source_proxies = paraview.servermanager.ProxyManager().GetProxiesInGroup("sources")
    proxies = []
    for key in source_proxies:
        listElt = {}
        proxy = source_proxies[key]
        listElt['proxy'] = proxy
        listElt['visibility'] = None
        rep = paraview.simple.GetDisplayProperties(proxy)
        if rep != None:
            listElt['visibility'] = rep.Visibility
        proxies.append(listElt)
    return proxies

def restore_visibility(proxies):
    for listElt in proxies:
        proxy = listElt['proxy']
        vis = listElt['visibility']
        if vis != None:
            rep = paraview.simple.GetDisplayProperties(proxy)
            if rep != None:
                rep.Visibility = listElt['visibility']

def inspect():
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
        if rep == None:
            #for example, writers in catalyst pipeline
            #todo: is it possible for these to have decendents that are visible?
            continue

        listElt['visibility'] = rep.Visibility

        parentId = '0'
        try:
            if hasattr(proxy, 'Input'):
                parentId = proxy.Input.GetGlobalIDAsString()
        except AttributeError:
            parentId = '0'
        listElt['parent'] = parentId
        proxies.append(listElt)
        proxybyId[key[1]] = listElt

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

    return pxies

def munch_tree(proxies):
    """
    Takes a representation of the pipeline and returns it in a set of tree levels.
    TODO: multirooted and branching pipeline are fine, but making no effort to deal
    with merging (e.g. probe with source) or cycles (e.g. nothing?)
    """
    levels = []
    pids = ['0']
    while len(proxies):
        pidsNext = []
        parents = []
        children = []
        for listElt in proxies:
            if listElt['parent'] in pids:
                parents.append(listElt)
                pidsNext.append(listElt['id'])
            else:
                children.append(listElt)
        levels.append(parents)
        pids = pidsNext
        proxies = children
    return levels

def float_limiter(x):
    #a shame, but needed to make sure python, java and (directory/file)name agree
    if isinstance(x, (float)):
        #return '%6f' % x #arbitrarily chose 6 decimal places
        return '%.6e' % x #arbitrarily chose 6 significant digits
    else:
        return x

# Keeps a link between a filter and its explorer-track. Populated in addFilterValueSublayer()
# and queried in explore()
explorerDir = {}

def addFilterValueSublayer(name, parentLayer, cs, userDefinedValues):
    source = paraview.simple.FindSource(name)

    # plane offset generator (for Slice or Clip)
    def generateOffsetValues():
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

        steps = 5 # generate 5 slice offsets
        offsetStep = np.linalg.norm(sNormal) / steps
        values = np.arange(-(steps/2), steps/2) * offsetStep
        return values.tolist()

    # generate values depending on the type of filter
    if isinstance(source, paraview.simple.servermanager.filters.Clip):
        # grab values from ui or generate defaults
        values = userDefinedValues["clip"] if ("clip" in userDefinedValues) else generateOffsetValues()

        # add sublayer and create the appropriate track
        cs.add_sublayer(name, cinema_store.make_parameter(name, values), parentLayer, name)
        cs.assign_parameter_dependence("color" + name, name, values)
        explorerDir[name] = pv_explorers.Clip(name, source)

    elif isinstance(source, paraview.simple.servermanager.filters.Slice):
        # grab values from ui or generate defaults
        values = userDefinedValues["slice"] if ("slice" in userDefinedValues) else generateOffsetValues()

        # add sublayer and create the appropriate track
        cs.add_sublayer(name, cinema_store.make_parameter(name, values), parentLayer, name)
        cs.assign_parameter_dependence("color" + name, name, values)
        explorerDir[name] = pv_explorers.Slice(name, source)

    elif isinstance(source, paraview.simple.servermanager.filters.Contour):

        def generateContourValues():
            # grab values from ui or generate defaults
            vRange = source.Input.GetDataInformation().DataInformation.GetPointDataInformation().GetArrayInformation(0).GetComponentRange(0)
            return np.linspace(vRange[0], vRange[1], 5).tolist() # generate 5 contour values

        values = userDefinedValues["contour"] if ("contour" in userDefinedValues) else generateContourValues()

        # add sublayer and create the appropriate track
        cs.add_sublayer(name, cinema_store.make_parameter(name, values), parentLayer, name)
        cs.assign_parameter_dependence("color" + name, name, values)
        explorerDir[name] = pv_explorers.Contour(name, source)

def make_cinema_store(levels, ocsfname, forcetime=False, _userDefinedValues={}):
    """
    Takes in the pipeline, structured as a tree, and makes a cinema store definition
    containing all the parameters we might will vary.
    """

    if "phi" in _userDefinedValues:
        phis = _userDefinedValues["phi"]
    else:
        phis = [0,45,90,135,180,225,270,315,360]
        #phis = [0,180,360]

    if "theta" in _userDefinedValues:
        thetas = _userDefinedValues["theta"]
    else:
        thetas = [0,45,90,135,180]
        #thetas = [0,90,180]

    def add_control_and_colors(snames, layername):
        """
        helper that adds a layer for the visibility setting (on/off)
        and sublayer for the visible fields of an object.
        """
        for s in snames:
            #a sublayer for the visibility of each object
            vparam = cinema_store.make_parameter(layername+s, ["ON","OFF"])
            cs.add_sublayer(layername+s, vparam, layername, s)

            #make up list of color options
            fields = {'depth':'depth','luminance':'luminance'}
            ranges = {}
            sp = paraview.simple.FindSource(s)
            defaultName = None
            cda = sp.GetCellDataInformation()
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
            pda = sp.GetPointDataInformation()
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
            cparam = cinema_store.make_field("color"+s, fields, default=defaultName, valueRanges=ranges)
            cs.add_field("color"+s,cparam,layername+s,"ON")
            #remember my parameter name for my dependees
            objhomes[asdict[s]['id']] = layername+s

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
    lcnt = 0
    objhomes = {}
    objnames = {}
    for level in levels:
        families = {}
        for listElt in level:
            objnames[listElt['id']] = listElt['name']
            #who are my siblings? - I can share a sub_layer with them.
            pid = listElt['parent']
            siblings = []
            if pid in families:
               siblings = families[pid]
            siblings.append(listElt)
            families[pid] = siblings

        #add layer containing all objects that share a common dependency
        for parent, siblings in families.iteritems():
            #print parent, siblings
            asdict = {}

            layername = "layer"+str(lcnt)
            snames = []
            for s in siblings:
                name = s['name']
                asdict[name] = s
                snames.append(name)
                addFilterValueSublayer(name, layername, cs, _userDefinedValues)

            param = cinema_store.make_parameter(layername, snames)
            if parent == '0':
                cs.add_layer(layername, param)
                add_control_and_colors(snames, layername)
            else:
                #look up name of my parent's parameter
                #print "SEEK OBJHOME of " + parent, "is",
                parentlayername = objhomes[parent]
                #print parentlayername
                cs.add_sublayer(layername, param, parentlayername, "OFF")
                add_control_and_colors(snames, layername)
            lcnt = lcnt + 1;

    fnp = ""
    if forcetime:
        #time specified, use it, being careful to append if already a list
        tvalues.append(forcetime)
        tprop = cinema_store.make_parameter('time', tvalues)
        cs.add_parameter('time', tprop)
        fnp = fnp+"{time}_"
    else:
        #time not specified, try and make them automaticvally
        times = paraview.simple.GetAnimationScene().TimeKeeper.TimestepValues
        if not times:
            pass
        else:
            cs.add_parameter("time", cinema_store.make_parameter('time', prettytimes))
            fnp = fnp+"{time}_"
    cs.add_parameter("phi", cinema_store.make_parameter('phi', phis))
    cs.add_parameter("theta", cinema_store.make_parameter('theta', thetas))
    fnp = fnp+"{phi}_{theta}/{layer0}.png"

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

            if name in y and 'layer' in y:
                #print "N", name
                #print "X", x
                #print "Y", y

                #visibility of the layer
                sp = paraview.simple.FindSource(name)
                rep = paraview.simple.GetRepresentation(sp, view_proxy)

                #hide all annotations
                rep.SetScalarBarVisibility(view_proxy, False)
                tc1 = pv_explorers.SourceProxyInLayer("ON", rep)
                lt = explorers.Layer(y, [tc1])
                tracks.append(lt)

                #fields for the layer
                cC = pv_explorers.ColorList()
                cC.AddDepth('depth')
                cC.AddLuminance('luminance')
                sp.UpdatePipeline(ctime_float)
                cda = sp.GetCellDataInformation()

                numVals = 0
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
    pxystate = record_visibility()

    view.LockBounds = 1

    p = inspect()
    l = munch_tree(p)
    cs = make_cinema_store(l, csname)
    #if test:
    #    testexplore(cs)
    #else:
    explore(cs, p)

    view.LockBounds = 0

    restore_visibility(pxystate)
    cs.save()
