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

settings = paraview.servermanager.vtkSMSettings.GetInstance()
settings.SetUserSettingsFromString(settingsString)
settings.SetSiteSettingsFromString("{}")

# Check the setting directly
settingPath = ".sources.SphereSource.Radius"
settingValue = settings.GetSettingAsDouble(settingPath, 0, 0.0)
if settingValue != trueRadius:
  print "Direct access of setting value", settingPath, "failed"
  sys.exit(-1)

settingValue = settings.GetSettingAsDouble(settingPath, 0, 0.0)
if settingValue != trueRadius:
  print "Direct access of setting value", settingPath, "as vector element failed"
  sys.exit(-1)

settingPath = ".sources.SphereSource.ThetaResolution"
settingValue = settings.GetSettingAsInt(settingPath, 0, 0)
if settingValue != trueThetaResolution:
  print "Direct access of setting value", settingPath, "failed"
  sys.exit(-1)

settingPath = ".sources.SphereSource.Center"
if settings.GetSettingNumberOfElements(settingPath) != len(trueCenter):
  print "Number of elements query failed"
  sys.exit(-1)

for i in xrange(3):
  settingValue = settings.GetSettingAsDouble(settingPath, i, 0.0)
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

#
#
# Now clear out the settings and test setting values
#
#
settings.SetUserSettingsFromString(None)
settingsString = settings.GetUserSettingsAsString()
if settingsString != "{}\n":
  print "User settings set from None should be {}"
  sys.exit(-1)
settings.SetSiteSettingsFromString(None)
settingsString = settings.GetSiteSettingsAsString()
if settingsString != "{}\n":
  print "Site settings set from None should be {}"
  sys.exit(-1)

settings.SetUserSettingsFromString("")
settingsString = settings.GetUserSettingsAsString()
if settingsString != "{}\n":
  print "User settings set from empty string should be {}"
  sys.exit(-1)
settings.SetSiteSettingsFromString("")
settingsString = settings.GetSiteSettingsAsString()
if settingsString != "{}\n":
  print "Site settings set from empty string should be {}"
  sys.exit(-1)

settings.SetUserSettingsFromString("{}")
settingsString = settings.GetUserSettingsAsString()
if settingsString != "{}\n":
  print "User settings set from {} should be {}"
  sys.exit(-1)
settings.SetSiteSettingsFromString("{}")
settingsString = settings.GetSiteSettingsAsString()
if settingsString != "{}\n":
  print "Site settings set from {} should be {}"
  sys.exit(-1)

if settings.HasSetting(".sources.SphereSource"):
  print "Setting '.sources.SphereSource' should have been cleared"
  sys.exit(-1)

settings.SetSetting(".sources.SphereSource.ints", 5)
settings.SetSetting(".sources.SphereSource.ints", 1, 2)
settings.SetSetting(".sources.SphereSource.double", 5.0)
settings.SetSetting(".sources.SphereSource.strings", "five")
settings.SetSetting(".sources.SphereSource.strings", "one")
settings.SetSetting(".sources.SphereSource.strings", 1, "two")
settings.SetSetting(".sources.SphereSource.strings", 1, "three")
print settings

if settings.GetSettingAsInt(".sources.SphereSource.ints", 0, 0) != 5:
  print "Setting '.sources.SphereSource.ints[0]' should have value 5"
  sys.exit(-1)

if settings.GetSettingAsInt(".sources.SphereSource.ints", 1, 0) != 2:
  print "Setting '.sources.SphereSource.ints[1]' should have value 2"
  sys.exit(-1)

if settings.GetSettingAsDouble(".sources.SphereSource.double", 0, 0.0) != 5.0:
  print "Setting '.sources.SphereSource.double' should have value 5.0"
  sys.exit(-1)

if settings.GetSettingAsString(".sources.SphereSource.strings", 0, "") != "one":
  print "Setting '.sources.SphereSource.strings[0]' should have value 'one'"
  sys.exit(-1)

if settings.GetSettingAsString(".sources.SphereSource.strings", 1, "") != "three":
  print "Setting '.sources.SphereSource.strings[1]' should have value 'three'"
  sys.exit(-1)
