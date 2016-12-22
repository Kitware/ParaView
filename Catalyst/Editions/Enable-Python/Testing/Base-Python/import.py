import paraview.simple

pxm = paraview.simple.servermanager.ProxyManager()
proxy = pxm.NewProxy('testcase', 'None')
print(proxy)
