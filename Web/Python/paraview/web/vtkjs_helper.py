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
            newName = "%s (%d)" % (nameToUse, count)
            while newName in duplicates:
                count += 1
                newName = "%s (%d)" % (nameToUse, count)
            nameToUse = newName
        duplicates[nameToUse] = True
        representation = simple.GetRepresentation(val)
        if representation:
            vtkRepInstance = representation.GetClientSideObject()
            if "GetActiveRepresentation" in dir(vtkRepInstance):
                actorRep = vtkRepInstance.GetActiveRepresentation().GetActor()
                actorNameMapping[nameToUse] = actorRep
    return actorNameMapping


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
        if not viewProp.IsA("vtkActor"):
            continue
        if not viewProp.GetVisibility():
            continue
        # The mapping will fail for multiblock that are composed of several blocks
        # Merge block should be used to solve the renaming issue for now
        # as the id is based on the a valid block vs representation.
        strIdx = "%s" % idx
        # Prop is valid if we can find it in the current sources
        for name, actor in names.items():
            if viewProp == actor:
                renameMap[strIdx] = name
                idx += 1
                break

    for volume in volumes:
        if not volume.IsA("vtkVolume"):
            continue
        if not volume.GetVisibility():
            continue
        strIdx = "%s" % idx
        for name, actor in names.items():
            if viewProp == actor:
                renameMap[strIdx] = name
                idx += 1
                break

    return renameMap


# -----------------------------------------------------------------------------


def applyParaViewNaming(directoryPath):
    renameMap = getRenameMap()
    scene = None
    filePath = os.path.join(directoryPath, "index.json")
    with open(filePath) as file:
        scene = json.load(file)
        for item in scene["scene"]:
            if item["name"] in renameMap:
                item["name"] = renameMap[item["name"]]

    with open(filePath, "w") as file:
        file.write(json.dumps(scene, indent=2))
