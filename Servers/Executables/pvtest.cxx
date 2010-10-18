/*=========================================================================

Program:   ParaView
Module:    pvserver.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInitializationHelper.h"
#include "vtkProcessModule2.h"
#include "vtkPVFileInformation.h"
#include "vtkPVServerOptions.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSourceProxy.h"
#include "vtkPVDataInformation.h"

#include "paraview.h"

#include "vtkSMSessionClient.h"

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  int return_value = EXIT_SUCCESS;
  bool printObject = false;

  vtkPVServerOptions* options = vtkPVServerOptions::New();
  vtkInitializationHelper::Initialize(argc, argv,
    vtkProcessModule2::PROCESS_BATCH, options);
  //---------------------------------------------------------------------------

  vtkSMSession* session = NULL;
  vtkSMProxy* proxy = NULL;
  if(options->GetUnknownArgument())
    {
    // We have a remote URL to use
    session = vtkSMSessionClient::New();
    vtkSMSessionClient::SafeDownCast(session)->Connect(options->GetUnknownArgument());
    }
  else
    {
    // We are in built-in mode
    session = vtkSMSession::New();
    }

  cout << "Starting..." << endl;

  vtkSMProxyManager* pxm = session->GetProxyManager();

  for(int i=0;i<10;i++)
    {
    cout << " Processing loop: " << i << endl;

    proxy = pxm->NewProxy("misc", "FileInformationHelper");
    vtkSMPropertyHelper(proxy, "Path").Set("/tmp");
    vtkSMPropertyHelper(proxy, "SpecialDirectories").Set(0);
    proxy->UpdateVTKObjects();

    vtkPVFileInformation* info = vtkPVFileInformation::New();
    proxy->GatherInformation(info);
    if(printObject) info->Print(cout);
    info->Delete();
    proxy->Delete();

    proxy = pxm->NewProxy("sources", "SphereSource");
    vtkSMPropertyHelper(proxy, "PhiResolution").Set(20);
    vtkSMPropertyHelper(proxy, "ThetaResolution").Set(20);
    proxy->UpdateVTKObjects();

    vtkSMSourceProxy* shrink =
        vtkSMSourceProxy::SafeDownCast(
            pxm->NewProxy("filters", "ProcessIdScalars"));
    vtkSMPropertyHelper(shrink, "Input").Set(proxy);
    shrink->UpdateVTKObjects();
    shrink->UpdatePipeline();

    shrink->GetDataInformation(0);
    if(printObject) shrink->GetDataInformation(0)->Print(cout);

    vtkSMSourceProxy* writer =
        vtkSMSourceProxy::SafeDownCast(
            pxm->NewProxy("writers", "PDataSetWriter"));
    vtkSMPropertyHelper(writer, "Input").Set(shrink);
    vtkSMPropertyHelper(writer, "FileName").Set("/tmp/foo.vtk");
    writer->UpdateVTKObjects();
    writer->UpdatePipeline();

    // Test session proxy/pmobj
    vtkSMSession *session = proxy->GetSession();
    vtkTypeUInt32 proxyID = proxy->GetGlobalID();
    vtkTypeUInt32 shrinkID = shrink->GetGlobalID();
    vtkTypeUInt32 writerID = writer->GetGlobalID();
    cout << "Session RemoteObject registration test: " << endl;
    if(proxy == session->GetRemoteObject(proxyID))
      {
      cout << " - proxy registered OK" << endl;
      }
    else
      {
      cout << " - proxy registered KO ***ERROR***" << endl;
      }
    if(shrink == session->GetRemoteObject(shrinkID))
      {
      cout << " - shrink registered OK" << endl;
      }
    else
      {
      cout << " - shrink registered KO ***ERROR***" << endl;
      }
    if(writer == session->GetRemoteObject(writerID))
      {
      cout << " - writer registered OK" << endl;
      }
    else
      {
      cout << " - writer registered KO ***ERROR***" << endl;
      }

    writer->Delete();
    proxy->Delete();
    shrink->Delete();

    // Test session proxy/pmobj unregister
//    cout << "Session RemoteObject unregistration test: " << endl;
    cout << " Results: " << proxyID << " " << shrinkID << " "<< writerID << endl;
    if(0 == session->GetRemoteObject(proxyID))
      {
      cout << " - proxy unregistered OK" << endl;
      }
    else
      {
      cout << " - proxy unregistered KO ***ERROR***" << endl;
      }
    if(0 == session->GetRemoteObject(shrinkID))
      {
      cout << " - shrink unregistered OK" << endl;
      }
    else
      {
      cout << " - shrink unregistered KO ***ERROR***" << endl;
      }
    if(0 == session->GetRemoteObject(writerID))
      {
      cout << " - writer unregistered OK" << endl;
      }
    else
      {
      cout << " - writer unregistered KO ***ERROR***" << endl;
      }
    }

  cout << "Exiting..." << endl;
  session->Delete();

  //---------------------------------------------------------------------------
  vtkInitializationHelper::Finalize();
  options->Delete();
  return return_value;
}
