from paraview.simple import *
from paraview import servermanager

import time

# Make sure the test driver know that process has properly started
print ("Process started")
errors = 0

#-------------------- Helpers methods ----------------
def getHost(url):
   return url.split(':')[1][2:]

def getScheme(url):
   return url.split(':')[0]

def getPort(url):
   return int(url.split(':')[2])
#--------------------

print ("Start multi-server testing")

options = servermanager.vtkProcessModule.GetProcessModule().GetOptions()
available_server_urls = options.GetServerURL().split('|')
built_in_connection = servermanager.ActiveConnection

# Test if the built-in connection is here
if (len(servermanager.Connections) != 1):
  errors += 1
  print ("Error pvpython should be connected to a built-in session. Currently connected to ", servermanager.Connections)

url = available_server_urls[0]
print ("Connect to first server ", url)
server1_connection = Connect(getHost(url), getPort(url))

# Test that we have one more connection
if (len(servermanager.Connections) != 2):
  errors += 1
  print ("Error pvpython should be connected to a built-in session + one remote one. Currently connected to ", servermanager.Connections)

url = available_server_urls[1]
print ("Connect to second server ", url)
server2_connection = Connect(getHost(url), getPort(url))

# Test that we have one more connection
if (len(servermanager.Connections) != 3):
  errors += 1
  print ("Error pvpython should be connected to a built-in session + two remote one. Currently connected to ", servermanager.Connections)

url = available_server_urls[2]
print ("Connect to third server ", url)
server3_connection = Connect(getHost(url), getPort(url))

# Test that we have one more connection
if (len(servermanager.Connections) != 4):
  errors += 1
  print ("Error pvpython should be connected to a built-in session + three remote one. Currently connected to ", servermanager.Connections)

print ("Available connections: ", servermanager.Connections)

# Test that last created connection is the active one
if ( servermanager.ActiveConnection != server3_connection):
  errors += 1
  print ("Error Invalid active connection. Expected ", server3_connection, " and got ", servermanager.ActiveConnection)

# ------- Do the proper RemoteSourceProxy testing --------------

# Create a set of sphere across the remote sessions
SetActiveConnection(server1_connection, globals())
rSphere1 = Sphere(ThetaResolution=10, PhiResolution=10)
rSphere1.UpdatePipeline()
size1 = rSphere1.GetDataInformation().GetNumberOfPoints()

SetActiveConnection(server2_connection, globals())
rSphere2 = Sphere(ThetaResolution=11, PhiResolution=11)
rSphere2.UpdatePipeline()
size2 = rSphere2.GetDataInformation().GetNumberOfPoints()

SetActiveConnection(server3_connection, globals())
rSphere3 = Sphere(ThetaResolution=12, PhiResolution=12)
rSphere3.UpdatePipeline()
size3 = rSphere3.GetDataInformation().GetNumberOfPoints()

# Create remote source on the built-in session
SetActiveConnection(built_in_connection, globals())
remoteProxy = RemoteSourceProxy()
remoteProxy.SetExternalProxy(rSphere1, 0)
remoteProxy.UpdatePipeline()

# Test that the data in built-in is the same size as the remote one
remoteProxy.UpdatePipeline()
size = remoteProxy.GetDataInformation().GetNumberOfPoints()
if ( size1 != size ):
   errors += 1
   print ("Error Invalid data size. Expected ", size1, " and got ", size)
else:
   print ("Found size ", size, " for server 1")

# Switch to proxy on server 2 and test size
remoteProxy.SetExternalProxy(rSphere2, 0)
remoteProxy.UpdatePipeline()
size = remoteProxy.GetDataInformation().GetNumberOfPoints()
if ( size2 != size ):
   errors += 1
   print ("Error Invalid data size. Expected ", size2, " and got ", size)
else:
   print ("Found size ", size, " for server 2")

# Switch to proxy on server 3 and test size
remoteProxy.SetExternalProxy(rSphere3, 0)
remoteProxy.UpdatePipeline()
size = remoteProxy.GetDataInformation().GetNumberOfPoints()
if ( size3 != size ):
   errors += 1
   print ("Error Invalid data size. Expected ", size3, " and got ", size)
else:
   print ("Found size ", size, " for server 3")

# Change data size on server 3 and make sure the change get propagated to the built-in
rSphere3.ThetaResolution = 13
rSphere3.PhiResolution = 13
rSphere3.UpdatePipeline()
size3 = rSphere3.GetDataInformation().GetNumberOfPoints()

remoteProxy.UpdatePipeline()
size = remoteProxy.GetDataInformation().GetNumberOfPoints()
if ( size3 != size ):
   errors += 1
   print ("Error Invalid data size. Expected ", size3, " and got ", size)
else:
   print ("Found size ", size, " for server 3 after update")

# Make sure the size is not 0
if ( size == 0 or size1 == 0 or size1 == 0 or size1 == 0):
   errors += 1
   print ("Error Invalid data size. None of them should be 0")

# --------------------------------------------------------------
# Disconnect and quit application...
Disconnect()
print ("Available connections after disconnect: ", servermanager.Connections)
Disconnect()
print ("Available connections after disconnect: ", servermanager.Connections)
Disconnect()
print ("Available connections after disconnect: ", servermanager.Connections)
Disconnect()
print ("Available connections after disconnect: ", servermanager.Connections)

if errors > 0:
  raise RuntimeError ("An error occurred during the execution")
