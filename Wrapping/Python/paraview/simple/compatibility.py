import paraview

VERSION = paraview.compatibility.GetVersion()


def GetPaletteName(paletteName):
    name = paletteName

    if VERSION <= (5, 10):
        if paletteName == "DefaultBackground" or paletteName == "GrayBackground":
            name = "BlueGrayBackground"
        elif paletteName == "PrintBackground":
            name = "WhiteBackground"
    if VERSION <= (5, 11):
        if paletteName == "WarmGrayBackground":
            name = "DarkGrayBackground"
    if VERSION <= (5, 12):
        if paletteName == "DefaultBackground":
            name = "BlueGrayBackground"

    return name
