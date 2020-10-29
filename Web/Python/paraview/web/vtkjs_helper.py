from paraview import simple
import os, json

# -----------------------------------------------------------------------------

def getAllNames():
    actorNameMapping = {}
    srcs = simple.GetSources()
    duplicates = {}
    for key, val in srcs.items():
        # Prevent name duplication
        nameToUse = key[0]
        if nameToUse in duplicates:
            count = 1
            newName = '%s (%d)' % (nameToUse, count)
            while newName in duplicates:
                count += 1
                newName = '%s (%d)' % (nameToUse, count)
            nameToUse = newName
        duplicates[nameToUse] = True
        representation = simple.GetRepresentation(val)
        if representation:
            vtkRepInstance = representation.GetClientSideObject()
            if 'GetActiveRepresentation' in dir(vtkRepInstance):
                actorRep = vtkRepInstance.GetActiveRepresentation().GetActor()
                actorNameMapping[nameToUse] = actorRep
    return actorNameMapping


# -----------------------------------------------------------------------------

def findName(names, actor, defaultName):
    for name in names:
        if actor == names[name]:
            return name
    return defaultName


# -----------------------------------------------------------------------------

def getRenameMap():
    renameMap = {}
    names = getAllNames()
    view = simple.GetActiveView()
    renderer = view.GetClientSideObject().GetRenderer()
    viewProps = renderer.GetViewProps()
    volumes = renderer.GetVolumes()
    idx = 1
    for viewProp in viewProps:
        if not viewProp.IsA('vtkActor'):
            continue
        if not viewProp.GetVisibility():
            continue
        bounds = viewProp.GetBounds()
        if bounds[0] > bounds[1]:
            continue
        # The mapping will fail for multiblock that are composed of several blocks
        # Merge block should be used to solve the renaming issue for now
        # as the id is based on the a valid block vs representation.
        strIdx = '%s' % idx
        renameMap[strIdx] = findName(names, viewProp, strIdx)
        idx += 1
    for volume in volumes:
        if not volume.IsA('vtkVolume'):
            continue
        if not volume.GetVisibility():
            continue
        bounds = volume.GetBounds()
        if bounds[0] > bounds[1]:
            continue
        strIdx = '%s' % idx
        renameMap[strIdx] = findName(names, volume, strIdx)
        idx += 1

    return renameMap


# -----------------------------------------------------------------------------

def applyParaViewNaming(directoryPath):
    renameMap = getRenameMap()
    scene = None
    filePath = os.path.join(directoryPath, 'index.json')
    with open(filePath) as file:
        scene = json.load(file)
        for item in scene['scene']:
            if item['name'] in renameMap:
                item['name'] = renameMap[item['name']]

    with open(filePath, 'w') as file:
        file.write(json.dumps(scene, indent=2))
