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
#include "paraview.h"
#include "vtkInitializationHelper.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkPVFileInformation.h"
#include "vtkPVServerOptions.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMMessage.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionClient.h"
#include "vtkSMSession.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  int return_value = EXIT_SUCCESS;
  bool printObject = true;

  vtkPVServerOptions* options = vtkPVServerOptions::New();
  vtkInitializationHelper::Initialize(argc, argv,
    vtkProcessModule::PROCESS_BATCH, options);
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

  proxy = pxm->NewProxy("internal_sources", "ExodusIIReaderCore");
  vtkSMPropertyHelper(proxy, "FileName").Set("/home/seb/Kitware/Projects/ParaView3/code/git/ParaViewData/Data/can.ex2");
  proxy->UpdateVTKObjects();
  proxy->UpdatePropertyInformation();

  // Test Read-only property with helpers
  vtkSMDoubleVectorProperty *timeProp =
      vtkSMDoubleVectorProperty::SafeDownCast(
          proxy->GetProperty("TimeRange"));
  cout << "TimeRange: " << timeProp->GetElement(0) << " to "
       << timeProp->GetElement(1) << endl;

  // ----------------------------------------------------

  vtkSMDoubleVectorProperty *timeStepsProp =
      vtkSMDoubleVectorProperty::SafeDownCast(
          proxy->GetProperty("TimestepValues"));
  cout << "TimeSteps: " ;
  for(vtkIdType i=0;i<timeStepsProp->GetNumberOfElements();i++)
    {
    cout << " " << timeStepsProp->GetElement(i);
    }
  cout << endl;

  // ----------------------------------------------------

  vtkSMStringVectorProperty *silProp =
      vtkSMStringVectorProperty::SafeDownCast(
          proxy->GetProperty("ElementBlocksInfo"));
  cout << "Nb ElementBlocksInfo: " << silProp->GetNumberOfElements() << endl;
  for(vtkIdType i=0;i<silProp->GetNumberOfElements();i++)
    {
    cout << "  - " << silProp->GetElement(i) << endl;
    }

  vtkSMStringVectorProperty *arraySelectProp =
      vtkSMStringVectorProperty::SafeDownCast(
          proxy->GetProperty("NodeSetInfo"));
  cout << "Nb NodeSetInfo: " << arraySelectProp->GetNumberOfElements() << endl;
  for(vtkIdType i=0;i<arraySelectProp->GetNumberOfElements();i+=2)
    {
    cout << "  - " << (strcmp("0",arraySelectProp->GetElement(i+1)) == 0 ? "[ ]" : "[X]") << " " << arraySelectProp->GetElement(i) << endl;
    }

  proxy->Delete();
  cout << "===========================================" << endl;
  proxy = pxm->NewProxy("sources", "SESAMEReader");
  vtkSMPropertyHelper(proxy, "FileName").Set("/home/seb/Kitware/Projects/ParaView3/code/git/ParaViewData/Data/SESAME301");
  proxy->UpdateVTKObjects();
  proxy->UpdatePropertyInformation();

  vtkSMIntVectorProperty *tableIdsProps =
      vtkSMIntVectorProperty::SafeDownCast(proxy->GetProperty("TableIds"));
  tableIdsProps->PrintSelf(cout, vtkIndent(5));

  cout << "Exiting..." << endl;
  session->Delete();

  //---------------------------------------------------------------------------
  vtkInitializationHelper::Finalize();
  options->Delete();
  return return_value;
}
