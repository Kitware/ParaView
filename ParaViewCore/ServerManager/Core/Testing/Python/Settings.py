"""This test is for the settings manager API."""

import sys
import paraview
paraview.compatibility.major = 3
paraview.compatibility.minor = 4
from paraview import servermanager

def Error(message):
  raise "ERROR: %s" % message

session = paraview.servermanager.vtkSMSession()
pxm = session.GetSessionProxyManager()

# Create settings from JSON description
trueRadius = 2.25
trueThetaResolution = 32
trueCenter = (1.0, 2.0, 4.0)
settingsString = """
{
  "sources" : {
    "SphereSource" : {
      "Radius" : %4.2f,
      "ThetaResolution" : %d,
      "Center" : [%3.1f, %3.1f, %3.1f]
    }
  }
}
""" % (trueRadius, trueThetaResolution, trueCenter[0], trueCenter[1], trueCenter[2])
print settingsString
settings = paraview.servermanager.vtkSMSettings.GetInstance()
settings.SetUserSettingsString(settingsString)
settings.SetSiteSettingsString("{}")

# Check the setting directly
settingPath = ".sources.SphereSource.Radius"
settingValue = settings.GetScalarSettingAsDouble(settingPath, 0.0)
if settingValue != trueRadius:
  print "Direct access of setting value", settingPath, "failed"
  sys.exit(-1)

settingValue = settings.GetVectorSettingAsDouble(settingPath, 0, 0.0)
if settingValue != trueRadius:
  print "Direct access of setting value", settingPath, "as vector element failed"
  sys.exit(-1)

settingPath = ".sources.SphereSource.ThetaResolution"
settingValue = settings.GetScalarSettingAsInt(settingPath, 0)
if settingValue != trueThetaResolution:
  print "Direct access of setting value", settingPath, "failed"
  sys.exit(-1)

settingPath = ".sources.SphereSource.Center"
if settings.GetNumberOfElements(settingPath) != len(trueCenter):
  print "Number of elements query failed"
  sys.exit(-1)

for i in xrange(3):
  settingValue = settings.GetVectorSettingAsDouble(settingPath, i, 0.0)
  if settingValue != trueCenter[i]:
    print "Direct access of setting value", settingPath, "[", i, "] failed"
    sys.exit(-1)

# The sphere source should pick up the settings from the user settings string
s = pxm.NewProxy("sources", "SphereSource")
settings.GetProxySettings(s);

radiusProperty = s.GetProperty("Radius")
radius = radiusProperty.GetElement(0)
if radius != trueRadius:
  print "Radius property does not match setting value"
  sys.exit(-1)
