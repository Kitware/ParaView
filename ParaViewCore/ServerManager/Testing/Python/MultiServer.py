# Import simple and validate expected behavior on creating several server
# connection as well as switching from one to another.

import sys
import SMPythonTesting
SMPythonTesting.ProcessCommandLineArguments()

# Test fails method
def failed(msg):
   print "Error: %s" % msg
   sys.exit(1)

# ---- Start testing ----
from paraview.simple import *
from paraview import servermanager

try:
  Connect()
  failed("This should have complained that you can not create another connection.")
except:
  print "1) Without enabling multi-server, the user is not allowed to reconnect. <== OK"

# keep ref to connections
firstConnection = servermanager.ActiveConnection

# ---- Enable multi-server ----
enableMultiServer()
Connect()

if len(servermanager.MultiServerConnections) != 2:
   failed("We should have 2 connections instead of %s" % str(servermanager.MultiServerConnections))
print "2) We have two server connections. <== OK"

# keep ref to connections
secondConnection = servermanager.ActiveConnection

sphere2 = Sphere()

switchActiveConnection(firstConnection, globals())
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
if len(servermanager.MultiServerConnections) != 1:
   failed("We should have 1 connection left instead of %s" % str(servermanager.MultiServerConnections))

servermanager.Disconnect()
if len(servermanager.MultiServerConnections) != 0:
   failed("We should have 0 connection left instead of %s" % str(servermanager.MultiServerConnections))

# Even if we don't have any connection, this should not failed
servermanager.Disconnect()

print "3) Test passed !!! YAY"
