/*=========================================================================

Program:   ParaView
Module:    TestSubProxy.cxx

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
#include "vtkProcessModule2.h"
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
//  if(argc != 2)
//    {
//    cerr << "This executable expect the path to the XML definition file where"
//         << endl << "the proxy sources/SphereSource3 is defined." << endl;
//    return EXIT_FAILURE;
//    }

  vtkPVServerOptions* options = vtkPVServerOptions::New();
  bool success = true;
  vtkInitializationHelper::Initialize(argc, argv,
    vtkProcessModule2::PROCESS_BATCH, options);
  if (!success)
    {
    return -1;
    }

  int ret_value = 0;

  vtkProcessModule2* pm = vtkProcessModule2::GetProcessModule();
  vtkMultiProcessController* controller = pm->GetGlobalController();
  if (controller->GetLocalProcessId() > 0)
    {
    // satellites never wait for client connections. They simply have 1 session
    // instance and then they simply listen for the root node to issue requests
    // for actions.
    vtkSMSession* session = vtkSMSession::New();
    controller->ProcessRMIs();
    session->Delete();
    }
  else
    {
    vtkSMSession* session = vtkSMSession::New();
    cout << "Starting..." << endl;

    vtkSMProxyManager* pxm = session->GetProxyManager();

    // *******************************************************************
    // Test specific code
    // *******************************************************************
    double value, valueA, valueB;
    pxm->LoadConfigurationXML("/home/seb/Kitware/Projects/DOE-Collaboration-SBIR-II/code/git/ParaView4/Servers/ServerManager/Testing/Cxx/TestCustomSubProxyDefinition.xml");

    // Create proxy and change main radius value
    vtkSMProxy* proxy = pxm->NewProxy("sources", "SphereSource3");
    vtkSMPropertyHelper(proxy, "Radius").Set(20);
    proxy->UpdateVTKObjects();

    // Read radius info
    vtkSMPropertyHelper(proxy, "Radius").Get(&value);
    vtkSMPropertyHelper(proxy, "RadiusA").Get(&valueA);
    vtkSMPropertyHelper(proxy, "RadiusB").Get(&valueB);
    cout << "Radius: " << value << " A: " << valueA << " B: " << valueB << endl;

    // Update radius for sub proxy A
    cout << "Update A radius to 123.456" << endl;
    vtkSMPropertyHelper(proxy, "RadiusA").Set(123.456);
    proxy->UpdateVTKObjects();

    // Read radius info
    vtkSMPropertyHelper(proxy, "Radius").Get(&value);
    vtkSMPropertyHelper(proxy, "RadiusA").Get(&valueA);
    vtkSMPropertyHelper(proxy, "RadiusB").Get(&valueB);
    cout << "Radius: " << value << " A: " << valueA << " B: " << valueB << endl;

//    vtkSmartPointer<vtkPVXMLElement> root = vtkSmartPointer<vtkPVXMLElement>::New();
//    proxy->SaveState(root);
//    root->PrintXML();

    // Update radius for sub proxy B
    cout << "Update A radius to 654.321" << endl;
    vtkSMPropertyHelper(proxy, "RadiusB").Set(654.321);
    proxy->UpdateVTKObjects();

    // Read radius info
    vtkSMPropertyHelper(proxy, "Radius").Get(&value);
    vtkSMPropertyHelper(proxy, "RadiusA").Get(&valueA);
    vtkSMPropertyHelper(proxy, "RadiusB").Get(&valueB);
    cout << "Radius: " << value << " A: " << valueA << " B: " << valueB << endl;

    // *******************************************************************

    proxy->Delete();
    session->Delete();
    cout << "Exiting..." << endl;

    }
  vtkInitializationHelper::Finalize();
  options->Delete();
  return ret_value;
}
