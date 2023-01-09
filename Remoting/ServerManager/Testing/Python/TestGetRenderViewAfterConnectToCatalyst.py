from paraview import servermanager, simple
s = servermanager.ConnectToCatalyst()
assert s
r = simple.GetRenderView()
assert r
