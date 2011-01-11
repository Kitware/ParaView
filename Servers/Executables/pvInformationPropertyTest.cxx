/*=========================================================================

Program:   ParaView
Module:    pvstatetest.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInitializationHelper.h"
#include "vtkMultiProcessController.h"
#include "vtkNetworkAccessManager.h"
#include "vtkProcessModule.h"
#include "vtkPVFileInformation.h"
#include "vtkPVServerOptions.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSourceProxy.h"
#include "vtkPVDataInformation.h"

#include "vtkPVXMLElement.h"

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  vtkPVServerOptions* options = vtkPVServerOptions::New();
  vtkInitializationHelper::Initialize( argc, argv,
                                       vtkProcessModule::PROCESS_BATCH,
                                       options);

  vtkSMSession* session = vtkSMSession::New();
  cout << "Starting..." << endl;

  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();

  // *******************************************************************
  // Test specific code
  // *******************************************************************
  double value;
  if(options->GetUnknownArgument())
    {
    cout << "Load Proxy definition: " << options->GetUnknownArgument() << endl;
    pxm->LoadConfigurationXML(options->GetUnknownArgument());
    }
  else
    {
    pxm->LoadConfigurationXML("/home/seb/Kitware/Projects/DOE-Collaboration-SBIR-II/code/git/ParaView4/Servers/ServerManager/Testing/Cxx/TestCustomSubProxyDefinition.xml");
    }

  // Update radius value
  cout << "Update radius to 20" << endl;
  vtkSMProxy* proxy = pxm->NewProxy("sources", "SphereSource3");
  vtkSMPropertyHelper(proxy, "Radius").Set(20);
  proxy->UpdateVTKObjects();

  // Read information property
  proxy->UpdatePropertyInformation();
  vtkSMPropertyHelper(proxy, "RadiusReadOnly").Get(&value);
  cout << "Radius read: " << value << endl;

  // Update radius value
  cout << "Update radius to 123.456" << endl;
  vtkSMPropertyHelper(proxy, "Radius").Set(123.456);
  proxy->UpdateVTKObjects();

  // Read information property
  proxy->UpdatePropertyInformation();
  vtkSMPropertyHelper(proxy, "RadiusReadOnly").Get(&value);
  cout << "Radius read: " << value << endl;

  // *******************************************************************

  proxy->Delete();
  session->Delete();
  cout << "Exiting..." << endl;

  vtkInitializationHelper::Finalize();
  options->Delete();
  return EXIT_SUCCESS;
}
