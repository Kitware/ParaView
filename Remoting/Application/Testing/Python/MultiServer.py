# Import simple and validate expected behavior on creating several server
# connection as well as switching from one to another.

import sys
from paraview import smtesting
smtesting.ProcessCommandLineArguments()

# Test fails method
def failed(msg):
   print("Error: %s" % msg)
   sys.exit(1)

# ---- Start testing ----
from paraview.simple import *
from paraview import servermanager

servermanager.vtkProcessModule.GetProcessModule().SetMultipleSessionsSupport(True)

# keep ref to connections
firstConnection = servermanager.ActiveConnection

Connect()

if len(servermanager.Connections) != 2:
   failed("We should have 2 connections instead of %s" % str(servermanager.Connections))
print("1) We have two server connections. <== OK")

# keep ref to connections
secondConnection = servermanager.ActiveConnection

sphere2 = Sphere()

SetActiveConnection(firstConnection, globals())
sphere1 = Sphere()

# make sure proxy are related to right session/connection
if sphere1.GetSession() != firstConnection.Session:
   failed("Invalid session in Sphere1")

if sphere2.GetSession() != secondConnection.Session:
   failed("Invalid session in Sphere2")

if servermanager.ActiveConnection != firstConnection:
   failed("Invalid ActiveConnection")

# Disconnect
servermanager.Disconnect()
if len(servermanager.Connections) != 1:
   failed("We should have 1 connection left instead of %s" % str(servermanager.Connections))

servermanager.Disconnect()
if len(servermanager.Connections) != 0:
   failed("We should have 0 connection left instead of %s" % str(servermanager.Connections))

# Even if we don't have any connection, this should not failed
servermanager.Disconnect()

print("3) Test passed !!! YAY")
