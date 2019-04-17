"""
Module that looks at a ParaView pipeline and automatically creates a cinema
store that ranges over all of the variables that we know how to control and
later show.
"""
from __future__ import absolute_import

from ...database import store, file_store
from ...adaptors import explorers
from ...adaptors.paraview import pv_explorers
from ...adaptors.paraview import progress

try:
    from itertools import imap
except ImportError:
    # python 3
    imap = map
import math
import os.path
import paraview
import copy


def record_visibility():
    """
    at start of run, record the current paraview state so we can return to it
    """
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
        # TODO: Find a way to record widget visibility so it can be restored
        paraview.simple.Hide3DWidgets(proxy=proxy)
        listElt['proxy'] = proxy
        listElt['visibility'] = None
        listElt['scalar_bar_visibility'] = False
        listElt['color_array_name'] = None
        listElt['color_array_association'] = None
        rep = paraview.servermanager.GetRepresentation(proxy, view_proxy)
        if rep is not None:
            listElt['visibility'] = rep.Visibility
            listElt['scalar_bar_visibility'] = rep.IsScalarBarVisible(
                view_proxy)
            listElt['color_array_name'] = rep.ColorArrayName.GetArrayName()
            listElt['color_array_association'] = \
                rep.ColorArrayName.GetAssociation()
        proxies.append(listElt)
    return proxies


def max_bounds():
    """ returns conservative min and max (over x y and z) bounds """
    source_proxies = paraview.servermanager.ProxyManager().GetProxiesInGroup(
        "sources")

    minb = 0
    maxb = -1
    for key in source_proxies:
        proxy = source_proxies[key]
        bounds = proxy.GetDataInformation().GetBounds()
        if bounds[0] < minb:
            minb = bounds[0]
        if bounds[2] < minb:
            minb = bounds[2]
        if bounds[4] < minb:
            minb = bounds[4]
        if bounds[1] > maxb:
            maxb = bounds[1]
        if bounds[3] > maxb:
            maxb = bounds[3]
        if bounds[5] > maxb:
            maxb = bounds[5]
    db = maxb-minb
    minb = minb-db
    maxb = maxb+db
    return minb, maxb


def restore_visibility(proxies):
    """at end of run, return to a previously recorded paraview state"""
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
            if vis is not None:
                rep = paraview.servermanager.GetRepresentation(
                    proxy, view_proxy)
                if rep is not None:
                    rep.Visibility = listElt['visibility']
                    if listElt['color_array_association']:
                        rep.SetScalarColoring(
                            listElt['color_array_name'],
                            paraview.servermanager.GetAssociationFromString(
                                listElt['color_array_association']))
                    if listElt['scalar_bar_visibility']:
                        rep.SetScalarBarVisibility(
                            view_proxy, listElt['scalar_bar_visibility'])


def inspect(skip_invisible=True):
    """
    Produces a representation of the pipeline that is easier to work with.
    Thanks Scott Wittenburg and the pv mailing list for this gem
    """
    source_proxies = paraview.servermanager.ProxyManager().GetProxiesInGroup(
        "sources")
    view_proxy = paraview.simple.GetActiveView()
    proxies = []
    proxybyId = {}
    for key in source_proxies:
        listElt = {}
        listElt['name'] = key[0]
        listElt['id'] = key[1]
        proxy = source_proxies[key]

        # skip the invisible
        rep = paraview.servermanager.GetRepresentation(proxy, view_proxy)
        if skip_invisible:
            if rep is None:
                # for example, writers in catalyst pipeline
                continue

        listElt['visibility'] = 0 if rep is None else rep.Visibility

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
        # reparent upward over invisible parents
        for l in proxies:
            pid = l['parent']
            if pid not in proxybyId:
                pid = '0'
            while pid != '0' and proxybyId[pid]['visibility'] == 0:
                pid = proxybyId[pid]['parent']
            l['parent'] = pid

        # remove invisible proxies themselves
        pxies = []
        for l in proxies:
            if l['visibility'] != 0:
                pxies.append(l)
    else:
        pxies = proxies

    return pxies


def get_pipeline():
    """sanitizes the pipeline graph"""
    proxies = inspect(skip_invisible=False)
    for proxy in proxies:
        source = paraview.simple.FindSource(proxy['name'])
        numberOfProducers = source.GetNumberOfProducers()
        if proxy['parent'] is '0' and numberOfProducers > 0:
            # this proxy is the result of a merge
            parents = []
            for i in xrange(numberOfProducers):
                parents.append(
                    source.GetProducerProxy(i).GetGlobalIDAsString())
            proxy['parents'] = parents
        else:
            proxy['parents'] = [proxy['parent']]
        del proxy['parent']
    for proxy in proxies:
        proxy['children'] = [p['id'] for p in proxies
                             if proxy['id'] in p['parents']]
    return proxies


def float_limiter(x):
    """a shame, but needed to make sure python, javascript and
    (directory/file)name agree. TODO: This can go away now that
    we use name=index instead of name=value filenames."""
    if isinstance(x, (float)):
        return '%.6e' % x  # arbitrarily chose 6 significant digits
    else:
        return x


# Keeps a link between a filter and its explorer-track.
# Populated in addFilterValue()
# and queried in explore()
explorerDir = {}


def add_filter_value(name, cs, userDefinedValues):
    """creates controls for the filters that we know how to manipulate"""
    source = paraview.simple.FindSource(name)

    # Determine the filter type
    valueType = None
    if isinstance(source, paraview.simple.servermanager.filters.Clip):
        valueType = 'OffsetValues'
        explorerDir[name] = pv_explorers.Clip(name, source)
    elif isinstance(source, paraview.simple.servermanager.filters.Slice):
        valueType = 'SliceOffsetValues'
        explorerDir[name] = pv_explorers.Slice(name, source)
    elif isinstance(source, paraview.simple.servermanager.filters.Contour):
        valueType = 'Isosurfaces'
        explorerDir[name] = pv_explorers.Contour(name, source)

    if valueType is not None:
        # grab values from ui
        values = []
        if (source in userDefinedValues):
            if (valueType in userDefinedValues[source]):
                values = userDefinedValues[source][valueType]

        if len(values) == 0:
            # nothing asked for just leave as is
            return False

        # add sublayer and create the appropriate track
        cs.add_control(
            name,
            store.make_parameter(name, values, typechoice='hidden'))
        return True
    else:
        return False


def filter_has_parameters(name):
    """see if this proxy is one we know how to make controls for"""
    source = paraview.simple.FindSource(name)
    return any(imap(lambda filter: isinstance(source, filter),
                    [paraview.simple.servermanager.filters.Clip,
                     paraview.simple.servermanager.filters.Slice,
                     paraview.simple.servermanager.filters.Contour]))


def add_control_and_colors(name, cs, userDefined, arrayRanges, disableValues):
    """add parameters that change the settings and color of a filter"""
    source = paraview.simple.FindSource(name)
    # make up list of color options
    fields = {'depth': 'depth'}
    if not disableValues:
        fields['luminance'] = 'luminance'
    defaultName = None
    view_proxy = paraview.simple.GetActiveView()
    rep = paraview.servermanager.GetRepresentation(source, view_proxy)

    # select value arrays
    if rep.Representation != 'Outline':
        defaultName = add_customized_array_selection(
            name, source, fields, userDefined, arrayRanges, disableValues)

    if defaultName is None:
        fields['white'] = 'rgb'
        defaultName = 'white'

    cparam = store.make_field(
        "color"+name, fields, default=defaultName, valueRanges=arrayRanges)
    cs.add_field("color"+name, cparam, 'vis', [name])


def add_customized_array_selection(
        sourceName, source, fields, userDefined, arrayRanges, disableValues):
    def isArrayNotSelected(aName, arrays):
        return (aName not in arrays)

    defaultName = None
    if (source not in userDefined):
        return defaultName

    if ("arraySelection" not in userDefined[source]):
        return defaultName

    arrayNames = userDefined[source]["arraySelection"]

    cda = source.GetCellDataInformation()
    pda = source.GetPointDataInformation()
    for datainfo in [cda, pda]:
        for a in range(0, datainfo.GetNumberOfArrays()):
            arr = datainfo.GetArray(a)
            arrName = arr.GetName()

            if isArrayNotSelected(arrName, arrayNames):
                continue
            for i in range(0, arr.GetNumberOfComponents()):
                fName = arrName+"_"+str(i)
                if disableValues:
                    fields[fName] = 'lut'
                else:
                    fields[fName] = 'value'
                extend_range(arrayRanges, fName, list(arr.GetRange(i)))
                if defaultName is None:
                    defaultName = fName

    return defaultName


def range_epsilon(minmax):
    """
    ensure that min and max have some separation to assist rendering
    """
    if minmax[0] == minmax[1]:
        epsilon = minmax[0]*1E-6
        if epsilon == 0.0:
            epsilon = 1E-6
        minV = minmax[0] - epsilon
        maxV = minmax[1] + epsilon
        return [minV, maxV]
    return minmax


def extend_range(arrayRanges, name, minmax):
    """
    This updates the data ranges in the data base meta file.
    Throughout a time varying data export ranges will vary.
    Here we accumulate them as we go so that by the end we get
    the min and max values over for each array component over
    all time.

    This version happens in catalyst, where we recreate the
    database file every timestep.
    """
    adjustedMinMax = range_epsilon(minmax)
    if name in arrayRanges:
        temporalMinMax = list(arrayRanges[name])
        if adjustedMinMax[0] < temporalMinMax[0]:
            temporalMinMax[0] = adjustedMinMax[0]
        if adjustedMinMax[1] > temporalMinMax[1]:
            temporalMinMax[1] = adjustedMinMax[1]
        arrayRanges[name] = temporalMinMax
    else:
        arrayRanges[name] = adjustedMinMax


def update_all_ranges(cs, arrayRanges):
    """
    This updates the data ranges in the data base meta file.
    Throughout a time varying data export ranges will vary.
    Here we accumulate them as we go so that by the end we get
    the min and max values over for each array component over
    all time.

    This version happens in paraview export for time varying data
    where we recreate the database file once, and afterward only
    output new rasters.
    """
    plist = cs.parameter_list
    for pname, param in plist.items():
        if 'valueRanges' in param:
            # now we know it is a color type parameter
            filtername = pname[5:]  # get the filter name for this parameter
            proxy = paraview.simple.FindSource(filtername)  # get the filter
            if proxy is not None:
                for name, vrange in param['valueRanges'].items():
                    # iterate over all the colors choiced to get each array
                    # component
                    lrange = list(vrange)
                    compidx = name.rfind('_')  # get array name and component
                    aname = name[:compidx]
                    component = int(name[compidx+1:])

                    cai = proxy.GetCellDataInformation().GetArray(aname)
                    pai = proxy.GetPointDataInformation().GetArray(aname)
                    for arrinfo in [cai, pai]:
                        if arrinfo:
                            updated = False
                            drange = range_epsilon(arrinfo.GetRange(component))
                            if drange[0] < vrange[0]:
                                updated = True
                                lrange[0] = drange[0]
                            if drange[1] > vrange[1]:
                                updated = True
                                lrange[1] = drange[1]
                            if updated:
                                param['valueRanges'][name] = lrange


def optimize_traversal_order(params):
    """
    An optimization that reduces generation time by better amortizing
    setup costs. With this rendering state changes (cheap) are late
    and in particular camera (~free) is the innermost loop.

    It also makes generation easier to watch. With camera innermost
    it iunderstand and prevents rapidly cycling
    between bright colors.
    """
    try:
        params.remove("vis")
        params.append("vis")
    except (ValueError):
        pass

    constant = copy.deepcopy(params)
    for x in constant:
        if x.find("color") == 0:
            params.remove(x)
            params.append(x)

    try:
        params.remove("phi")
        params.append("phi")
        params.remove("theta")
        params.append("theta")
    except (ValueError):
        pass

    try:
        params.remove("pose")
        params.append("pose")
    except (ValueError):
        pass

    return params


def make_cinema_store(proxies,
                      ocsfname,
                      view,
                      forcetime=False,
                      userDefined={},
                      specLevel="A",
                      camType='phi-theta',
                      arrayRanges={},
                      extension=".png",
                      disableValues=False):

    """
    Takes in the pipeline, structured as a tree, and makes a cinema store
    definition containing all the parameters we will vary.
    """

    phis = userDefined.get('phi', [0, 180, 360])
    thetas = userDefined.get('theta', [0, 90, 180])
    rolls = userDefined.get('roll', [0, 45, 90, 135, 180, 225, 270, 315])

    if camType == 'static' or camType == 'phi-theta':
        rolls = [0]

    tvalues = []
    cs = file_store.FileStore(ocsfname)

    try:
        cs.load()
        tprop = cs.get_parameter('time')
        tvalues = tprop['values']

        # start with clean slate, other than time
        cs = file_store.FileStore(ocsfname)
    except (IOError, KeyError):
        pass

    eye_values = cs.metadata.get('camera_eye', [])
    at_values = cs.metadata.get('camera_at', [])
    up_values = cs.metadata.get('camera_up', [])
    nearfar_values = cs.metadata.get('camera_nearfar', [])
    viewangle_values = cs.metadata.get('camera_angle', [])

    cs.add_metadata({'store_type': 'FS'})
    if specLevel == "A":
        cs.add_metadata({'type': 'parametric-image-stack'})
        cs.add_metadata({'version': '0.0'})
    if specLevel == "B":
        cs.add_metadata({'type': 'composite-image-stack'})
        cs.add_metadata({'version': '0.2'})
    pipeline = get_pipeline()
    cs.add_metadata({'pipeline': pipeline})
    cs.add_metadata({'camera_model': camType})
    cs.add_metadata({'camera_eye': eye_values})
    cs.add_metadata({'camera_at': at_values})
    cs.add_metadata({'camera_up': up_values})
    cs.add_metadata({'camera_nearfar': nearfar_values})
    cs.add_metadata({'camera_angle': viewangle_values})

    vis = [proxy['name'] for proxy in proxies]
    if specLevel != "A":
        cs.add_layer("vis", store.make_parameter('vis', vis))

    pnames = []
    for proxy in proxies:
        proxy_name = proxy['name']
        ret = add_filter_value(proxy_name, cs, userDefined)
        if specLevel == "A" and ret:
            pnames.append(proxy_name)
        dependency_set = set([proxy['id']])
        repeat = True
        while repeat:
            repeat = False
            deps = set(proxy['id'] for proxy in proxies
                       if proxy['parent'] in dependency_set)
            if deps - dependency_set:
                dependency_set = dependency_set.union(deps)
                repeat = True
        dependency_list = [proxy['name'] for proxy in proxies
                           if proxy['id'] in dependency_set]
        if specLevel != "A":
            cs.assign_parameter_dependence(proxy_name, 'vis', dependency_list)
            add_control_and_colors(proxy_name, cs, userDefined, arrayRanges,
                                   disableValues)
            cs.assign_parameter_dependence(
                "color"+proxy_name, 'vis', [proxy_name])

    fnp = ""
    if forcetime is not False:
        # time specified, use it, being careful to append if already a list
        tvalues.append(forcetime)
        tprop = store.make_parameter('time', tvalues)
        cs.add_parameter('time', tprop)
        fnp = "{time}"
    else:
        # time not specified, try and make them automatically
        times = paraview.simple.GetAnimationScene().TimeKeeper.TimestepValues
        if times:
            prettytimes = [float_limiter(t) for t in times]
            cs.add_parameter(
                "time", store.make_parameter('time', prettytimes))
            fnp = "{time}"

    if camType == "phi-theta":
        bestp = phis[len(phis)//2]
        bestt = thetas[len(thetas)//2]
        cs.add_parameter(
            "phi",
            store.make_parameter('phi', phis,
                                 default=bestp))
        cs.add_parameter(
            "theta",
            store.make_parameter('theta', thetas,
                                 default=bestt))
        if fnp == "":
            fnp = "{phi}/{theta}"
        else:
            fnp = fnp + "/{phi}/{theta}"

    elif camType != "static":
        # for AER and YPR, make up a set of view matrices corresponding
        # to the requested number of samples in each axis
        def MatrixMul(mtx_a, mtx_b):
            tpos_b = zip(*mtx_b)
            rtn = [[sum(ea*eb for ea, eb in zip(a, b)) for b in tpos_b]
                   for a in mtx_a]
            return rtn

        poses = []  # holds phi, theta and roll angle tuples
        matrices = []  # holds corresponding transform matrices

        v = rolls[0]
        rolls = []
        if v < 2:
            rolls.append(0)
        else:
            j = -180
            while j < 180:
                rolls.append(j)
                j = j+360/v

        v = thetas[0]
        thetas = []
        if v < 2:
            thetas.append(0)
        else:
            j = -90
            while j <= 90:
                thetas.append(j)
                j = j+180/v

        for r in rolls:
            for t in thetas:
                v = phis[0]
                if v < 2:
                    poses.append((0, t, r))
                else:
                    # sample longitude less frequently toward the pole
                    increment_Scale = math.cos(math.pi*t/180.0)
                    if increment_Scale == 0:
                        increment_Scale = 1
                    # increment_Scale = 1  # for easy comparison
                    p = -180
                    while p < 180:
                        poses.append((p, t, r))
                        p = p+360/(v*increment_Scale)

        # default is one closest to 0,0,0
        dist = math.sqrt((poses[0][0]*poses[0][0]) +
                         (poses[0][1]*poses[0][1]) +
                         (poses[0][2]*poses[0][2]))
        default_mat = 0
        for i in poses:
            p, t, r = i
            cP = math.cos(-math.pi*(p/180.0))  # phi is right to left
            sP = math.sin(-math.pi*(p/180.0))
            cT = math.cos(-math.pi*(t/180.0))  # theta is up down
            sT = math.sin(-math.pi*(t/180.0))
            cR = math.cos(-math.pi*(r/180.0))  # roll is around gaze direction
            sR = math.sin(-math.pi*(r/180.0))
            rY = [[cP, 0, sP], [0, 1, 0], [-sP, 0, cP]]  # x,z interchange
            rX = [[1, 0, 0], [0, cT, -sT], [0, sT, cT]]  # y,z interchange
            rZ = [[cR, -sR, 0], [sR, cR, 0], [0, 0, 1]]  # x,y interchange
            m1 = [[1, 0, 0],  [0, 1, 0],  [0, 0, 1]]
            m2 = MatrixMul(m1, rY)
            m3 = MatrixMul(m2, rX)
            m4 = MatrixMul(m3, rZ)
            matrices.append(m4)
            newdist = math.sqrt(p*p+t*t+r*r)
            if newdist < dist:
                default_mat = m4
                dist = newdist

        cs.add_parameter("pose",
                         store.make_parameter('pose', matrices,
                                              default=default_mat))
        fnp = fnp+"{pose}"

    if specLevel == "A":
        for pname in pnames:
            if fnp == "":
                fnp = "{"+pname+"}"
            else:
                fnp = fnp+"/{"+pname+"}"

    if fnp == "":
        fnp = "image"

    cs.filename_pattern = fnp+extension
    return cs


def track_source(proxy, eye, at, up):
    """
    an animation mode that follows a specific object
    input camera position is in eye, at, up
    returns same, moved to follow the input proxy
    """
    # code duplicated from vtkPVCameraCueManipulator
    if proxy is None:
        return eye, at, up

    info = proxy.GetDataInformation()
    bounds = info.GetBounds()

    center = [(bounds[0] + bounds[1]) * 0.5,
              (bounds[2] + bounds[3]) * 0.5,
              (bounds[4] + bounds[5]) * 0.5]

    ret_eye = [center[0] + (eye[0] - at[0]),
               center[1] + (eye[1] - at[1]),
               center[2] + (eye[2] - at[2])]

    ret_at = [center[0], center[1], center[2]]
    return ret_eye, ret_at, up


def project_to_at(eye, fp, cr):
    """project center of rotation onto focal point to keep gaze direction the same
    while allowing both translate and zoom in and out to work"""
    d_fp = [fp[0]-eye[0], fp[1]-eye[1], fp[2]-eye[2]]
    d_cr = [cr[0]-eye[0], cr[1]-eye[1], cr[2]-eye[2]]
    num = (d_fp[0]*d_cr[0] + d_fp[1]*d_cr[1] + d_fp[2]*d_cr[2])
    den = (d_fp[0]*d_fp[0] + d_fp[1]*d_fp[1] + d_fp[2]*d_fp[2])
    if den == 0:
        return cr
    rat = num/den
    p_fp = [rat*d_fp[0], rat*d_fp[1], rat*d_fp[2]]
    at = [p_fp[0]+eye[0], p_fp[1]+eye[1], p_fp[2]+eye[2]]
    return at


def explore(cs, proxies, iSave=True, currentTime=None, userDefined={},
            specLevel="A",
            camType='phi-theta',
            tracking={},
            floatValues=True,
            arrayRanges={},
            disableValues=False,
            progressObject=None):
    """
    Runs a pipeline through all the changes we know how to make and saves off
    images into the store for each one.
    """

    view_proxy = paraview.simple.GetActiveView()
    dist = paraview.simple.GetActiveCamera().GetDistance()

    # associate control points with parameters of the data store
    params = list(cs.parameter_list.keys())
    tracks = []
    if camType == "phi-theta":
        up = [math.fabs(x) for x in view_proxy.CameraViewUp]
        uppest = 0
        if up[1] > up[uppest]:
            uppest = 1
        if up[2] > up[uppest]:
            uppest = 2
        cinup = [0, 0, 0]
        cinup[uppest] = 1

        eye = [x for x in view_proxy.CameraPosition]
        _fp = [x for x in view_proxy.CameraFocalPoint]
        _cr = [x for x in view_proxy.CenterOfRotation]
        at = project_to_at(eye, _fp, _cr)

        cam = pv_explorers.Camera(
            at, cinup, dist, view_proxy)
        tracks.append(cam)
    elif camType != 'static':
        cam = pv_explorers.PoseCamera(view_proxy, camType, cs)
        tracks.append(cam)

    cols = []

    ctime_float = None
    if currentTime:
        ctime_float = float(currentTime['time'])

    # hide all annotations
    view_proxy.OrientationAxesVisibility = 0

    for x in proxies:
        name = x['name']
        for y in params:

            if (y in explorerDir) and (name == y):
                # print ("name in ExplorerDir: ", y, ", ", explorerDir[y])
                tracks.append(explorerDir[y])

            if name in y:
                # visibility of the layer
                sp = paraview.simple.FindSource(name)
                if specLevel != "A":
                    rep = paraview.servermanager.GetRepresentation(
                        sp, view_proxy)

                    # hide all annotations
                    if rep.LookupTable:
                        rep.SetScalarBarVisibility(view_proxy, False)
                    tc1 = pv_explorers.SourceProxyInLayer(name, rep, sp)
                    lt = explorers.Layer('vis', [tc1])
                    tracks.append(lt)

                    # fields for the layer
                    cC = pv_explorers.ColorList()
                    cC.AddDepth('depth')
                    if not disableValues:
                        cC.AddLuminance('luminance')

                sp.UpdatePipeline(ctime_float)

                if specLevel != "A":
                    numVals = 0
                    if rep.Representation != 'Outline':
                        numVals = explore_customized_array_selection(
                            name, sp, cC, userDefined, disableValues)

                    if numVals == 0:
                        cC.AddSolidColor('white', [1, 1, 1])
                    col = pv_explorers.Color("color"+name, cC, rep)
                    tracks.append(col)
                    cols.append(col)

    params = optimize_traversal_order(params)

    explo = pv_explorers.ImageExplorer(cs, params,
                                       tracks,
                                       view_proxy,
                                       iSave)
    explo.enableFloatValues(floatValues)

    for c in cols:
        c.imageExplorer = explo

    eye_values = cs.metadata['camera_eye']
    at_values = cs.metadata['camera_at']
    up_values = cs.metadata['camera_up']
    nearfar_values = cs.metadata['camera_nearfar']
    viewangle_values = cs.metadata['camera_angle']

    eye = [x for x in view_proxy.CameraPosition]
    _fp = [x for x in view_proxy.CameraFocalPoint]
    _cr = [x for x in view_proxy.CenterOfRotation]
    at = project_to_at(eye, _fp, _cr)
    up = [x for x in view_proxy.CameraViewUp]
    times = paraview.simple.GetAnimationScene().TimeKeeper.TimestepValues
    if currentTime:
        times = False

    cam = paraview.simple.GetActiveCamera()

    # if tracking is turned on, find out how to move
    tracked_source = None
    if 'object' in tracking:
        # for now, just emulate animation's best mode with a mode that follows
        # an object
        objname = tracking['object']
        tracked_source = paraview.simple.FindSource(objname)
        if tracked_source is None:
            name_upper = objname[0].upper() + objname[1:]
            tracked_source = paraview.simple.FindSource(name_upper)

    if not times:
        eye, at, up = track_source(tracked_source, eye, at, up)
        try:
            tprop = cs.get_parameter('time')
            tvalues = tprop['values']
            for i in range(len(tvalues)):
                eye_values.append([x for x in eye])
                at_values.append([x for x in at])
                up_values.append([x for x in up])
                nearfar_values.append([x for x in cam.GetClippingRange()])
                viewangle_values.append(cam.GetViewAngle())
        except (KeyError):
            eye_values.append([x for x in eye])
            at_values.append([x for x in at])
            up_values.append([x for x in up])
            nearfar_values.append([x for x in cam.GetClippingRange()])
            viewangle_values.append(cam.GetViewAngle())
        cs.add_metadata({'camera_eye': eye_values})
        cs.add_metadata({'camera_at': at_values})
        cs.add_metadata({'camera_up': up_values})
        cs.add_metadata({'camera_nearfar': nearfar_values})
        cs.add_metadata({'camera_angle': viewangle_values})
        explo.explore(currentTime, progressObject)
    else:
        for t in times:
            view_proxy.ViewTime = t
            paraview.simple.Render(view_proxy)
            minbds, maxbds = max_bounds()
            view_proxy.MaxClipBounds = [
                minbds, maxbds, minbds, maxbds, minbds, maxbds]
            eye, at, up = track_source(tracked_source, eye, at, up)
            eye_values.append([x for x in eye])
            at_values.append([x for x in at])
            up_values.append([x for x in up])
            nearfar_values.append([x for x in cam.GetClippingRange()])
            viewangle_values.append(cam.GetViewAngle())

            cs.add_metadata({'camera_eye': eye_values})
            cs.add_metadata({'camera_at': at_values})
            cs.add_metadata({'camera_up': up_values})
            cs.add_metadata({'camera_nearfar': nearfar_values})
            cs.add_metadata({'camera_angle': viewangle_values})

            update_all_ranges(cs, arrayRanges)
            explo.explore({'time': float_limiter(t)}, progressObject)

    return cs.get_new_files()


def explore_customized_array_selection(
        sourceName, source, colorList, userDefined, disableValues):
    def isArrayNotSelected(aName, arrays):
        return (aName not in arrays)

    numVals = 0
    if (source not in userDefined):
        return numVals

    if ("arraySelection" not in userDefined[source]):
        return numVals

    arrayNames = userDefined[source]["arraySelection"]

    cda = source.GetCellDataInformation()
    pda = source.GetPointDataInformation()

    for datainfo, datatype in zip([cda, pda], ['CELLS', 'POINTS']):
        for a in range(0, datainfo.GetNumberOfArrays()):
            arr = datainfo.GetArray(a)
            arrName = arr.GetName()

            if isArrayNotSelected(arrName, arrayNames):
                continue
            for i in range(0, arr.GetNumberOfComponents()):
                numVals += 1
                if disableValues:
                    nforcomp = str(i)
                    if arr.GetNumberOfComponents() < 4:
                        nforcomp = 'X'
                        if i == 1:
                            nforcomp = 'Y'
                        if i == 2:
                            nforcomp = 'Z'
                    colorList.AddLUT(datatype, arrName+"_"+str(i), nforcomp)
                else:
                    isfield = (datatype == 'CELLS')
                    colorList.AddValueRender(
                        arrName + "_" + str(i),
                        isfield,
                        arrName,
                        i, arr.GetRange(i))

    return numVals


def export_scene(baseDirName, viewSelection, trackSelection, arraySelection, forcetime=False):
    '''
    This explores a set of user-defined views and tracks. export_scene is
    called from vtkCinemaExport.  The expected order of parameters is as
    follows:

    - viewSelection (following the format defined in
      Wrapping/Python/paraview/cpstate.py):

    Directory  of the form {'ViewName' : [parameters], ...}, with parameters
    defined in the order: Image filename, freq, fittoscreen, magnification,
    width, height, cinema).

    - trackSelection:

    Directory of the form {'FilterName' : [v1, v2, v3], ...}

    - arraySelection:

    Directory of the form {'FilterName' : ['arrayName1', 'arrayName2', ...],
                           ... }

    Note:  baseDirName is used as the parent directory of the database
    generated for each view in viewSelection. 'Image filename' is used as the
    database directory name.
    '''
    # save initial state
    initialView = paraview.simple.GetActiveView()
    pvstate = record_visibility()

    # a conservative global bounds for consistent z scaling
    minbds, maxbds = max_bounds()

    atLeastOneViewExported = False
    cinema_dirs = []
    new_files = {}
    for viewName, viewParams in viewSelection.iteritems():

        extension = os.path.splitext(viewParams[0])[1]

        # check if this view was selected to export as spec b
        cinemaParams = viewParams[6]
        if len(cinemaParams) == 0:
            print ("Skipping view: Not selected to export to cinema")
            continue

        camType = "none"
        if "camera" in cinemaParams and cinemaParams["camera"] != "none":
            camType = cinemaParams["camera"]
        if camType == "none":
            print ("Skipping view: Not selected to export to cinema.")
            continue

        specLevel = "A"
        if "composite" in cinemaParams and cinemaParams["composite"] is True:
            specLevel = "B"

        # get the view and save the initial status
        view = paraview.simple.FindView(viewName)
        paraview.simple.SetActiveView(view)
        view.ViewSize = [viewParams[4], viewParams[5]]

        view.MaxClipBounds = [minbds, maxbds, minbds, maxbds, minbds, maxbds]
        view.LockBounds = 1

        fitToScreen = viewParams[2]
        if fitToScreen != 0:
            if view.IsA("vtkSMContextViewProxy") is True:
                view.ResetDisplay()
            elif view.IsA("vtkSMRenderViewProxy") is False:
                print (' do not know what to do with a ', view.GetClassName())

        userDefValues = prepare_selection(trackSelection, arraySelection)
        if "theta" in cinemaParams:
            userDefValues["theta"] = cinemaParams["theta"]

        if "phi" in cinemaParams:
            userDefValues["phi"] = cinemaParams["phi"]

        if "roll" in cinemaParams:
            userDefValues["roll"] = cinemaParams["roll"]

        tracking_def = {}
        if "tracking" in cinemaParams:
            tracking_def = cinemaParams['tracking']

        # generate file path
        viewFileName = viewParams[0]
        viewDirName = viewFileName[0:viewFileName.rfind("_")]  # strip _num.ext
        filePath = os.path.join(baseDirName, viewDirName, "info.json")
        cinema_dirs.append(viewDirName)

        p = inspect()

        arrayRanges = {}
        disableValues = cinemaParams.get('noValues', False)

        cs = make_cinema_store(p, filePath, view, forcetime=forcetime,
                               userDefined=userDefValues,
                               specLevel=specLevel,
                               camType=camType,
                               arrayRanges=arrayRanges,
                               extension=extension,
                               disableValues=disableValues)
        enableFloatVal = cinemaParams.get('floatValues', False)

        pm = paraview.servermanager.vtkProcessModule.GetProcessModule()
        pid = pm.GetPartitionId()

        progObj = progress.ProgressObject()
        progObj.StartEvent()
        currentTime = None
        if forcetime is not False:
            currentTime = {'time':forcetime}
        explore(cs, p, iSave=(pid == 0),
                currentTime=currentTime,
                userDefined=userDefValues,
                specLevel=specLevel,
                camType=camType,
                tracking=tracking_def, floatValues=enableFloatVal,
                arrayRanges=arrayRanges, disableValues=disableValues,
                progressObject=progObj)
        progObj.EndEvent()

        new_files[viewName] = cs.get_new_files()

        view.LockBounds = 0

        if pid == 0:
            cs.save()
        atLeastOneViewExported = True

    if not atLeastOneViewExported:
        print ("No view was selected to export to cinema.")
        return

    make_workspace_file(baseDirName, cinema_dirs)

    # restore initial state
    paraview.simple.SetActiveView(initialView)
    restore_visibility(pvstate)
    return new_files


def prepare_selection(trackSelection, arraySelection):
    '''
    The rest of pv_introspect expects to receive user-defined values in the
    structure:

    {proxy_reference: {'ControlName': [value_1, value_2, ..., value_n],
                       'arraySelection': ['ArrayName_1', ..., 'ArrayName_n']}}

    This structure is necessary for catalyst to correctly reference the created
    proxies.  Although this is not necessary in the menu->export case (proxies
    could be accessed by name directly), we comply for compatibility.
    '''
    userDef = {}
    for name, values in trackSelection.iteritems():
        source = paraview.simple.FindSource(name)
        if (source is None):
            # Following the smtrace.py convention pqCinemaTrackSelection passes
            # lower-case-initial names, here the method tries to resolve the
            # upper-case-initial version of the name. Caveat: breaks if the
            # user re-names two items such that the only difference is the
            # first letter's capitalization (which would be confusing anyway).
            name_upper = name[0].upper() + name[1:]
            source = paraview.simple.FindSource(name_upper)

        if (source):
            options = userDef[source] if (source in userDef) else {}

            # Assumption: only a single 'ControlName' is supported per filter.
            # (the control name will need to be included in the ui query when
            # giving support to more parameters).
            controlName = ""
            if ("servermanager.Slice" in source.__class__().__str__() and
                "Plane object" in source.__getattribute__(
                    "SliceType").__str__()):
                controlName = "SliceOffsetValues"
            elif ("servermanager.Clip" in source.__class__().__str__() and
                  "Plane object" in
                  source.__getattribute__("ClipType").__str__()):
                controlName = "OffsetValues"
            elif ("servermanager.Contour" in source.__class__().__str__()):
                controlName = "Isosurfaces"

            if len(controlName) > 0:
                options[controlName] = values
                userDef[source] = options

    for name, arrayNames in arraySelection.iteritems():
        source = paraview.simple.FindSource(name)
        if (source is None):
            # Following the smtrace.py convention pqCinemaTrackSelection passes
            # lower-case-initial names, here the method tries to resolve the
            # upper-case-initial version of the name. Caveat: breaks if the
            # user re-names two items such that the only difference is the
            # first letter's capitalization (which would be confusing anyway).
            name_upper = name[0].upper() + name[1:]
            source = paraview.simple.FindSource(name_upper)

        if (source):
            options = userDef[source] if (source in userDef) else {}
            options["arraySelection"] = arrayNames
            userDef[source] = options

    return userDef


def make_workspace_file(basedirname, cinema_dirs):
    """
    This writes out the top level json file that says that there are
    child cinema stores inside. The viewer sees this and opens up
    in the children in separate panels.
    """
    pm = paraview.servermanager.vtkProcessModule.GetProcessModule()
    pid = pm.GetPartitionId()
    if len(cinema_dirs) > 1 and pid == 0:
        workspace = open(basedirname + 'info.json', 'w')
        workspace.write('{\n')
        workspace.write('    "metadata": {\n')
        workspace.write('        "type": "workbench"\n')
        workspace.write('    },\n')
        workspace.write('    "runs": [\n')
        for i in range(0, len(cinema_dirs)):
            workspace.write('        {\n')
            workspace.write('        "title": "%s",\n' % cinema_dirs[i])
            workspace.write('        "description": "%s",\n' % cinema_dirs[i])
            workspace.write('        "path": "%s"\n' % cinema_dirs[i])
            if i+1 < len(cinema_dirs):
                workspace.write('        },\n')
            else:
                workspace.write('        }\n')
                workspace.write('    ]\n')
                workspace.write('}\n')
                workspace.close()
