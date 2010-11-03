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
#include "vtkProcessModule2.h"
#include "vtkPVFileInformation.h"
#include "vtkPVServerOptions.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSourceProxy.h"
#include "vtkPVDataInformation.h"

#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMStringVectorProperty.h"

#include "paraview.h"

#include "vtkSMSessionClient.h"

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  int return_value = EXIT_SUCCESS;
  bool printObject = true;

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

  vtkSMStringVectorProperty *sliProp =
      vtkSMStringVectorProperty::SafeDownCast(
          proxy->GetProperty("ElementBlocksInfo"));
  sliProp->PrintSelf(cout, vtkIndent(0));

  cout << "===========================================" << endl;
  vtkSMStringVectorProperty *arraySelectProp =
      vtkSMStringVectorProperty::SafeDownCast(
          proxy->GetProperty("FaceBlocksInfo"));
  arraySelectProp->PrintSelf(cout, vtkIndent(0));

  cout << "Exiting..." << endl;
  proxy->Delete();
  session->Delete();

  //---------------------------------------------------------------------------
  vtkInitializationHelper::Finalize();
  options->Delete();
  return return_value;
}
