// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInitializationHelper.h"
#include "vtkNew.h"
#include "vtkPVTestUtilities.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSettings.h"
#include "vtkSmartPointer.h"

#include <vtk_jsoncpp.h>

#include "vtksys/SystemTools.hxx"

namespace utils
{
//---------------------------------------------------------------------------
// Add Application and User priority settings for SphereSource
bool InitSettings()
{
  vtkSMSettings* settings = vtkSMSettings::GetInstance();
  settings->ClearAllSettings();

  const char* settingString = "{\n"
                              "  \"sources\" : {\n"
                              "    \"SphereSource\" : {\n"
                              "      \"Radius\" : 4.25,\n"
                              "      \"Center\" : [1, 2, 3]\n"
                              "    }\n"
                              "  }\n"
                              "}\n";

  settings->AddCollectionFromString(settingString, vtkSMSettings::GetApplicationPriority());

  const char* higherPrioritySettingsString = "{\n"
                                             "  \"sources\" : {\n"
                                             "    \"SphereSource\" : {\n"
                                             "      \"Radius\" : 1.0,\n"
                                             "      \"ThetaResolution\" : 42.0\n"
                                             "    }\n"
                                             "  }\n"
                                             "}\n";

  settings->AddCollectionFromString(higherPrioritySettingsString, vtkSMSettings::GetUserPriority());

  return true;
}

//---------------------------------------------------------------------------
bool TestPriority(vtkSMParaViewPipelineController* controller, vtkSMProxy* sphere)
{
  InitSettings();
  controller->PreInitializeProxy(sphere);
  controller->PostInitializeProxy(sphere);

  vtkSMSettings* settings = vtkSMSettings::GetInstance();
  double lowerThanApp = vtkSMSettings::GetApplicationPriority() - 1;
  if (settings->HasSetting("sources.SphereSource.Radius", lowerThanApp))
  {
    cerr << "Should not have settings with lower priority than ApplicationPriority\n";
    return false;
  }

  if (settings->HasSetting(
        "sources.SphereSource.ThetaResolution", vtkSMSettings::GetApplicationPriority()))
  {
    cerr << "Application priority should not have ThetaResolution\n";
    return false;
  }

  if (!settings->HasSetting("sources.SphereSource.Center", vtkSMSettings::GetApplicationPriority()))
  {
    cerr << "Application priority should have Center\n";
    return false;
  }

  if (!settings->HasSetting("sources.SphereSource.ThetaResolution"))
  {
    cerr << "User priority should have ThetaResolution\n";
    return false;
  }

  if (!settings->HasSetting("sources.SphereSource.Radius"))
  {
    cerr << "Default (User) priority should have Radius\n";
    return false;
  }

  // Check the sphere radius. It should be set from the settings.
  vtkSMDoubleVectorProperty* radiusProperty =
    vtkSMDoubleVectorProperty::SafeDownCast(sphere->GetProperty("Radius"));
  if (radiusProperty)
  {
    if (radiusProperty->GetElement(0) != 1.0)
    {
      cerr << "Failed at " << __LINE__ << endl;
      return false;
    }

    // Check the sphere center. It should be set from the settings.
    vtkSMDoubleVectorProperty* centerProperty =
      vtkSMDoubleVectorProperty::SafeDownCast(sphere->GetProperty("Center"));
    if (centerProperty->GetElement(0) != 1.0 || centerProperty->GetElement(1) != 2.0 ||
      centerProperty->GetElement(2) != 3.0)
    {
      cerr << "Failed at " << __LINE__ << endl;
      return false;
    }
  }
  else
  {
    cerr << "Could not get Radius property\n";
  }

  return true;
}

//---------------------------------------------------------------------------
// Test saving different number of repeatable property values
// and test saving Proxy properties.
bool TestProxyProperty(
  vtkSMSession* session, vtkSMParaViewPipelineController* controller, vtkSMProxy* sphere)
{
  InitSettings();
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
  vtkSmartPointer<vtkSMProxy> contour;
  contour.TakeReference(pxm->NewProxy("filters", "Contour"));
  controller->PreInitializeProxy(contour);
  vtkSMPropertyHelper(contour, "Input").Set(sphere);
  controller->PostInitializeProxy(contour);

  vtkSMDoubleVectorProperty* contourValuesProperty =
    vtkSMDoubleVectorProperty::SafeDownCast(contour->GetProperty("ContourValues"));
  if (!contourValuesProperty)
  {
    std::cerr << "No contour values property in GenericContour\n";
    return false;
  }

  vtkSMSettings* settings = vtkSMSettings::GetInstance();
  // Double vector property resize
  contourValuesProperty->SetNumberOfElements(1);
  contourValuesProperty->SetElement(0, -1.0);
  settings->SetProxySettings(contour);

  contourValuesProperty->SetNumberOfElements(2);
  contourValuesProperty->SetElement(0, -2.0);
  contourValuesProperty->SetElement(1, -3.0);
  settings->SetProxySettings(contour);

  auto contourLocatorProperty = vtkSMProxyProperty::SafeDownCast(contour->GetProperty("Locator"));
  if (!contourLocatorProperty)
  {
    std::cerr << "No contour locator property in GenericContour" << std::endl;
    return false;
  }

  auto proxyListDomain = contourLocatorProperty->FindDomain<vtkSMProxyListDomain>();
  vtkSMProxy* locator0 = proxyListDomain->GetProxy(0);
  vtkSMProxy* locator1 = proxyListDomain->GetProxy(1);
  contourLocatorProperty->SetProxy(0, locator1);
  settings->SetProxySettings(contour);
  contourLocatorProperty->SetProxy(0, locator0);

  settings->GetProxySettings(contour);
  contour->ResetPropertiesToDefault();

  if (strcmp(contourLocatorProperty->GetProxy(0)->GetXMLName(), locator0->GetXMLName()) != 0)
  {
    std::cerr << "Wrong selected locator. Has " << contourLocatorProperty->GetProxy(0)->GetXMLName()
              << " instead of " << locator0->GetXMLName() << std::endl;
    std::cerr << *settings << std::endl;
    return false;
  }

  return true;
}

//---------------------------------------------------------------------------
bool TestJSONSerDes(vtkSMProxy* sphere)
{
  InitSettings();
  vtkSMPropertyHelper(sphere, "Radius").Set(12);
  Json::Value state = vtkSMSettings::SerializeAsJSON(sphere);

  vtkSMPropertyHelper(sphere, "Radius").Set(1);
  if (!vtkSMSettings::DeserializeFromJSON(sphere, state) ||
    vtkSMPropertyHelper(sphere, "Radius").GetAsInt() != 12)
  {
    cerr << "Failed to DeserializeFromJSON." << endl;
    return false;
  }

  return true;
}

//---------------------------------------------------------------------------
bool TestSaveLoadFile(vtkPVTestUtilities* utility)
{
  InitSettings();
  char* cpath = utility->GetTempFilePath("user-settings.json");
  std::string filePath(cpath);
  delete[] cpath;
  vtksys::SystemTools::RemoveFile(filePath);

  vtkSMSettings* settings = vtkSMSettings::GetInstance();
  settings->SaveSettingsToFile(filePath); //, vtkSMSettings::GetUserPriority());
  settings->ClearAllSettings();
  if (!settings->AddCollectionFromFile(filePath, vtkSMSettings::GetUserPriority()))
  {
    cerr << "Failed to load collection from file\n";
    return false;
  }

  if (!settings->HasSetting("sources.SphereSource.Radius"))
  {
    cerr << "Failed to load UserSettings: SphereSource.Radius not found!\n";
    return false;
  }

  if (settings->HasSetting("sources.SphereSource.Center"))
  {
    cerr << "Wrong setting loaded: has SphereSource.Center but it should not.\n";
    return false;
  }

  return true;
}
};

//----------------------------------------------------------------------------
int DoTests(vtkPVTestUtilities* utility)
{
  vtkNew<vtkSMSession> session;
  vtkNew<vtkSMParaViewPipelineController> controller;
  if (!controller->InitializeSession(session))
  {
    cerr << "Failed to initialize ParaView session." << endl;
    return EXIT_FAILURE;
  }

  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
  vtkSmartPointer<vtkSMProxy> sphere;
  sphere.TakeReference(pxm->NewProxy("sources", "SphereSource"));

  if (!utils::TestPriority(controller, sphere))
  {
    return EXIT_FAILURE;
  }

  // NOTE: Contour property is not available in all editions, so it's possible the
  // Contour filter is not defined. Handle that case.
  if (pxm->HasDefinition("filters", "Contour"))
  {
    if (!utils::TestProxyProperty(session, controller, sphere))
    {
      return EXIT_FAILURE;
    }
  }

  if (!utils::TestJSONSerDes(sphere))
  {
    return EXIT_FAILURE;
  }

  if (!utils::TestSaveLoadFile(utility))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

extern int TestSettings(int argc, char* argv[])
{
  vtkNew<vtkPVTestUtilities> testUtilities;
  testUtilities->Initialize(argc, argv);

  vtkInitializationHelper::Initialize(argv[0], vtkProcessModule::PROCESS_CLIENT);

  int ret = DoTests(testUtilities);

  // avoid writing our test settings on disk.
  vtkSMSettings* settings = vtkSMSettings::GetInstance();
  settings->ClearAllSettings();
  vtkInitializationHelper::Finalize();
  return ret;
}
