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
  // Default settings for sources.
  "sources" : {
    // A sphere source
    "SphereSource" : {
      /* New comment
         New comment 2 */
      "Radius" : %4.2f,
      // Theta resolution


      // Theta resolution 2
      "ThetaResolution" : %d,
      // Center comment
      "Center" : [%3.1f, %3.1f, %3.1f]
    }
  }
}
""" % (trueRadius, trueThetaResolution, trueCenter[0], trueCenter[1], trueCenter[2])

settings = paraview.servermanager.vtkSMSettings.GetInstance()
settings.ClearAllSettings()
settings.AddCollectionFromString(settingsString, 2000000.0)
settings.AddCollectionFromString("{}", 1500000.0)

def check_comment(expected, settings, field):
  actual = settings.GetSettingDescription(field)
  if actual != expected:
    print("Comment for %s was not what was expected" % field)
    print("expected: %s" % expected)
    print("actual  : %s" % actual)
    sys.exit(-1)

check_comment("// Default settings for sources.", settings, ".sources")
check_comment("// A sphere source", settings, ".sources.SphereSource")
check_comment("/* New comment\n         New comment 2 */", settings, ".sources.SphereSource.Radius")
check_comment("// Theta resolution\n// Theta resolution 2", settings, ".sources.SphereSource.ThetaResolution")
check_comment("// Center comment", settings, ".sources.SphereSource.Center")

# Check the setting directly
settingPath = ".sources.SphereSource.Radius"
settingValue = settings.GetSettingAsDouble(settingPath, 0, 0.0)
if settingValue != trueRadius:
  print("Direct access of setting value %s failed" % settingPath)
  sys.exit(-1)

settingValue = settings.GetSettingAsDouble(settingPath, 0, 0.0)
if settingValue != trueRadius:
  print("Direct access of setting value %s as vector element failed" % settingPath)
  sys.exit(-1)

settingPath = ".sources.SphereSource.ThetaResolution"
settingValue = settings.GetSettingAsInt(settingPath, 0, 0)
if settingValue != trueThetaResolution:
  print("Direct access of setting value %s failed" % settingPath)
  sys.exit(-1)

settingPath = ".sources.SphereSource.Center"
if settings.GetSettingNumberOfElements(settingPath) != len(trueCenter):
  print("Number of elements query failed")
  sys.exit(-1)

for i in range(3):
  settingValue = settings.GetSettingAsDouble(settingPath, i, 0.0)
  if settingValue != trueCenter[i]:
    print("Direct access of setting value %s[%i] failed" % (settingPath, i))
    sys.exit(-1)

# The sphere source should pick up the settings from the user settings string
s = pxm.NewProxy("sources", "SphereSource")
settings.GetProxySettings(s);

radiusProperty = s.GetProperty("Radius")
radius = radiusProperty.GetElement(0)
if radius != trueRadius:
  print("Radius property does not match setting value")
  s.FastDelete()
  sys.exit(-1)
s.FastDelete()

#
#
# Now clear out the settings and test setting values
#
#
settings.ClearAllSettings()
if settings.HasSetting(".sources.SphereSource"):
  print("Setting '.sources.SphereSource' should have been cleared")
  sys.exit(-1)

settings.SetSetting(".sources.SphereSource.ints", 5)
settings.SetSetting(".sources.SphereSource.ints", 1, 2)
settings.SetSetting(".sources.SphereSource.double", 5.0)
settings.SetSetting(".sources.SphereSource.strings", "five")
settings.SetSetting(".sources.SphereSource.strings", "one")
settings.SetSetting(".sources.SphereSource.strings", 1, "two")
settings.SetSetting(".sources.SphereSource.strings", 1, "three")

if settings.GetSettingAsInt(".sources.SphereSource.ints", 0, 0) != 5:
  print("Setting '.sources.SphereSource.ints[0]' should have value 5")
  sys.exit(-1)

if settings.GetSettingAsInt(".sources.SphereSource.ints", 1, 0) != 2:
  print("Setting '.sources.SphereSource.ints[1]' should have value 2")
  sys.exit(-1)

if settings.GetSettingAsDouble(".sources.SphereSource.double", 0, 0.0) != 5.0:
  print("Setting '.sources.SphereSource.double' should have value 5.0")
  sys.exit(-1)

if settings.GetSettingAsString(".sources.SphereSource.strings", 0, "") != "one":
  print("Setting '.sources.SphereSource.strings[0]' should have value 'one'")
  sys.exit(-1)

if settings.GetSettingAsString(".sources.SphereSource.strings", 1, "") != "three":
  print("Setting '.sources.SphereSource.strings[1]' should have value 'three'")
  sys.exit(-1)
