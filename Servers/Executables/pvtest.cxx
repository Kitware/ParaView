/*=========================================================================

Program:   ParaView
Module:    pvtest.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInitializationHelper.h"
#include "vtkProcessModule.h"
#include "vtkPVFileInformation.h"
#include "vtkPVServerOptions.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSourceProxy.h"
#include "vtkPVDataInformation.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSessionCore.h"
#include "vtkSMProxyIterator.h"

#include "paraview.h"

#include "vtkSMSessionClient.h"

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  int return_value = EXIT_SUCCESS;
  bool printObject = false;

  vtkPVServerOptions* options = vtkPVServerOptions::New();
  vtkInitializationHelper::Initialize(argc, argv,
    vtkProcessModule::PROCESS_CLIENT, options);
  //---------------------------------------------------------------------------

  vtkSMSession* session = NULL;
  vtkSMProxy* proxy = NULL;

  //session = vtkSMSessionClient::New();
  //vtkSMSessionClient::SafeDownCast(session)->Connect("cs://localhost:11111");
  session = vtkSMSession::New();

  cout << "Starting..." << endl;

  vtkSMProxyManager* pxm = session->GetProxyManager();
  vtkProcessModule::GetProcessModule()->RegisterSession(session);

  //================= View test =======================

  vtkSMProxy* sphere = pxm->NewProxy("sources", "SphereSource");
  vtkSMPropertyHelper(sphere, "PhiResolution").Set(20);
  vtkSMPropertyHelper(sphere, "ThetaResolution").Set(20);
  sphere->UpdateVTKObjects();

  vtkSMProxy* cone = pxm->NewProxy("sources", "ConeSource");
  vtkSMPropertyHelper(cone, "Resolution").Set(20);
  cone->UpdateVTKObjects();

  vtkSMProxy* repr = pxm->NewProxy("representations", "GeometryRepresentation");
  vtkSMPropertyHelper(repr, "Input").Set(sphere);
  vtkSMPropertyHelper(repr, "Representation").Set("Wireframe");
  //vtkSMPropertyHelper(repr, "Representation").Set("Surface");
  repr->UpdateVTKObjects();

  vtkSMProxy* coneRepr = pxm->NewProxy("representations", "GeometryRepresentation");
  vtkSMPropertyHelper(coneRepr, "Input").Set(cone);
  vtkSMPropertyHelper(coneRepr, "Representation").Set("Wireframe");
  //vtkSMPropertyHelper(coneRepr, "Representation").Set("Surface");
  coneRepr->UpdateVTKObjects();

  vtkSMProxy* view = pxm->NewProxy("views", "RenderView");
  vtkSMPropertyHelper(view, "Representations").Add(repr);
  view->UpdateVTKObjects();

  vtkSMRenderViewProxy::SafeDownCast(view)->StillRender();
  vtkSMRenderViewProxy::SafeDownCast(view)->ResetCamera();
  vtkSMRenderViewProxy::SafeDownCast(view)->StillRender();
  sleep(1);

  vtkSMPropertyHelper(view, "Representations").Add(coneRepr);
  view->UpdateVTKObjects();
  vtkSMPropertyHelper(repr, "Representation").Set("Surface");
  repr->UpdateVTKObjects();

  vtkSMRenderViewProxy::SafeDownCast(view)->StillRender();
  vtkSMRenderViewProxy::SafeDownCast(view)->ResetCamera();
  vtkSMRenderViewProxy::SafeDownCast(view)->StillRender();
  sleep(1);

  vtkSMPropertyHelper(view, "Representations").Remove(repr);
  view->UpdateVTKObjects();

  vtkSMRenderViewProxy::SafeDownCast(view)->StillRender();
  vtkSMRenderViewProxy::SafeDownCast(view)->ResetCamera();
  vtkSMRenderViewProxy::SafeDownCast(view)->StillRender();
  sleep(1);

  vtkSMPropertyHelper(view, "Representations").Remove(coneRepr);
  view->UpdateVTKObjects();

  vtkSMRenderViewProxy::SafeDownCast(view)->StillRender();
  vtkSMRenderViewProxy::SafeDownCast(view)->ResetCamera();
  vtkSMRenderViewProxy::SafeDownCast(view)->StillRender();
  sleep(1);

  view->Delete();
  sphere->Delete();
  repr->Delete();

  // ====== End of view test =======

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

    vtkSMProxy* proxy = pxm->NewProxy("sources", "SphereSource");
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
      return_value = EXIT_FAILURE;
      }
    if(shrink == session->GetRemoteObject(shrinkID))
      {
      cout << " - shrink registered OK" << endl;
      }
    else
      {
      cout << " - shrink registered KO ***ERROR***" << endl;
      return_value = EXIT_FAILURE;
      }
    if(writer == session->GetRemoteObject(writerID))
      {
      cout << " - writer registered OK" << endl;
      }
    else
      {
      cout << " - writer registered KO ***ERROR***" << endl;
      return_value = EXIT_FAILURE;
      }

    pxm->RegisterProxy("test", "source", proxy);
    pxm->RegisterProxy("test", "shrink", shrink);
    pxm->RegisterProxy("test", "writer", writer);
    vtkSMProxyIterator *iter = vtkSMProxyIterator::New();
    iter->Begin();
    while(!iter->IsAtEnd())
      {
      cout << "Iterate over proxy " << iter->GetGroup() << ":" << iter->GetKey() << endl;
      iter->Next();
      }
    iter->Delete();

    pxm->UnRegisterProxy(proxy);
    pxm->UnRegisterProxy(shrink);
    pxm->UnRegisterProxy(writer);

    writer->Delete();
    proxy->Delete();
    shrink->Delete();

    // Test session proxy/pmobj unregister
    cout << "Session RemoteObject unregistration test: " << endl;
    if(0 == session->GetRemoteObject(proxyID))
      {
      cout << " - proxy unregistered OK" << endl;
      }
    else
      {
      cout << " - proxy unregistered KO ***ERROR***" << endl;
      return_value = EXIT_FAILURE;
      }
    if(0 == session->GetRemoteObject(shrinkID))
      {
      cout << " - shrink unregistered OK" << endl;
      }
    else
      {
      cout << " - shrink unregistered KO ***ERROR***" << endl;
      return_value = EXIT_FAILURE;
      }
    if(0 == session->GetRemoteObject(writerID))
      {
      cout << " - writer unregistered OK" << endl;
      }
    else
      {
      cout << " - writer unregistered KO ***ERROR***" << endl;
      return_value = EXIT_FAILURE;
      }
    }



  cout << "Exiting..." << endl;
  session->Delete();

  //---------------------------------------------------------------------------
  vtkInitializationHelper::Finalize();
  options->Delete();
  return return_value;
}
