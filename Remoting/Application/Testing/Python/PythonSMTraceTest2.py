# This test creates a pipeline, traces the state, clears the proxies,
# then executes the python code to recreate the state, then confirms
# that the correct proxies exist with the correct property values

from paraview.simple import *
from paraview import smstate
from paraview import smtesting
import sys

# Process command line args and get temp dir
smtesting.ProcessCommandLineArguments()
tempDir = smtesting.TempDir

def fail(message):
    raise Exception(message)

def clear_proxies():
    """Method for clearing all user created proxies"""
    groups = ["sources", "representations", "views",
                  "implicit_functions", "piecewise_functions",
                  "lookup_tables", "scalar_bars", "selection_sources"]
    for g in groups:
        for p in servermanager.ProxyManager().GetProxiesInGroup(g).values():
            Delete(p)

########################################################
# Begin build pipeline
def create_pipeline():
    view = CreateRenderView()
    view.Background=[0,0,1]
    sphere = Sphere(guiName="my sphere")
    Show()
    view2 = CreateRenderView() # create a second view for the remaining sources
    glyph = Glyph(guiName="my glyph")
    Show()
    glyph.GlyphType = "Cone"
    glyph.GlyphType.Radius = 0.1
    glyph.GlyphType.Resolution = 15
    clip = Clip(guiName="my clip")
    Show()
    clip.ClipType = "Sphere"
    clip.ClipType.Radius = 0.25
    group = GroupDatasets(Input=[sphere, clip], guiName="my group")
    Show()
    GetDisplayProperties().Representation = "Surface With Edges"
    Render()
create_pipeline()
# End build pipeline
########################################################


# Trace state and grab the trace output string
state = smstate.get_state()
print(state)

# Clear all the proxies
clear_proxies()

# Confirm that all the representations have been removed from the view
if len(GetRepresentations()) or len(GetRenderViews()) or len(GetSources()):
    fail("Not all proxies were cleaned up.")

# Compile the trace code and run it
code = compile(state, "<string>", "exec")
exec(code)

# Get the recreated proxies
view = FindView("RenderView1")
view2 = FindView("RenderView2")
sphere = FindSource("my sphere")
glyph = FindSource("my glyph")
clip = FindSource("my clip")
group = FindSource("my group")


# Test the results
epsilon = 0.000001
if not sphere:
    fail("Could not find sphere")
if glyph.GlyphType.Resolution != 15:
    fail("Glyph cone resolution is incorrect.")
if abs(clip.ClipType.Radius - 0.25) > epsilon:
    fail("Clip sphere radius is incorrect.")
if sphere not in group.Input or clip not in group.Input:
    fail("Group has wrong inputs.")
if GetDisplayProperties(group, view2).Representation != "Surface With Edges":
    fail("Group representation is incorrect")
if abs(view.Background[1] - 0) > epsilon or abs(view.Background[2] - 1) > epsilon:
    fail("View has incorrect background color")
if len(view.Representations) != 1:
    fail("First view has incorrect number of representations.")
if view.Representations[0].Input != sphere:
    fail("First view does not contain sphere.")
