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

def make_cinema_store(levels, ocsfname, _phis=None, _thetas=None):
    """
    Takes in the pipeline, structured as a tree, and makes a cinema store definition
    containing all the parameters we might will vary.
    """
    phis = _phis
    if phis==None:
        phis = [0,45,90,135,180,225,270,315,360]
        phis = [0,180,360]
    thetas = _thetas
    if thetas==None:
        thetas = [0,20,40,60,80,100,120,140,160,180]
        thetas = [0,90,180]

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

    cs = cinema_store.FileStore(ocsfname)
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
                asdict[s['name']] = s
                snames.append(s['name'])
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
    times = paraview.simple.GetAnimationScene().TimeKeeper.TimestepValues
    if not times:
        pass
    else:
        prettytimes = [float_limiter(x) for x in times]
        cs.add_parameter("time", cinema_store.make_parameter('time', prettytimes))
        fnp = fnp+"{time}_"
    cs.add_parameter("phi", cinema_store.make_parameter('phi', phis))
    cs.add_parameter("theta", cinema_store.make_parameter('theta', thetas))
    fnp = fnp+"{phi}_{theta}/{layer0}.png"

    cs.filename_pattern = fnp
    cs.save()
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


def explore(cs, proxies):
    """
    Takes in the store, which contains only the list of parameters,
    """
    import pv_explorers
    import explorers

    print "PROXIES:"
    for p in proxies:
        print " ", p

    view_proxy = paraview.simple.GetActiveView()
    dist = paraview.simple.GetActiveCamera().GetDistance()

    #associate control points wlth parameters of the data store
    cam = pv_explorers.Camera([0,0,0], [0,1,0], dist, view_proxy)

    cs.load()
    params = cs.parameter_list.keys()
    print "PARAMS:"
    for p in sorted(params):
        print " ", p

    tracks = []
    tracks.append(cam)

    cols = []

    #hide all annotations
    view_proxy.OrientationAxesVisibility = 0

    for x in proxies:
        name = x['name']
        for y in params:
            if name in y and 'layer' in y:
                print "N", name
                print "X", x,
                print "Y", y

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
                                   view_proxy)

    for c in cols:
        c.imageExplorer = e

    times = paraview.simple.GetAnimationScene().TimeKeeper.TimestepValues
    if not times:
        e.explore()
    else:
        for t in times:
            view_proxy.ViewTime=t
            e.explore({'time':float_limiter(t)})

def record(csname="/tmp/test_pv/info.json",
        sfname="/Users/demarle/Desktop/cinema/example_pipeline.pvsm",
        test=False):
    if sfname!=None:
        paraview.simple.LoadState(sfname)
    paraview.simple.Render()
    view = paraview.simple.GetActiveView()
    view.LockBounds = 1
    p = inspect()
    l = munch_tree(p)
    cs = make_cinema_store(l, csname)
    if test:
        testexplore(cs)
    else:
        explore(cs, p)
    view.LockBounds = 0
