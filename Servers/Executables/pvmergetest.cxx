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
#include "vtkSMRenderViewProxy.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkPVXMLElement.h"

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

  vtkPVXMLElement* def = pxm->GetProxyDefinitionManager()->GetCollapsedProxyDefinition("representations", "GeometryRepresentation");
  def->PrintXML();

  cout << "Exiting..." << endl;
  session->Delete();

  //---------------------------------------------------------------------------
  vtkInitializationHelper::Finalize();
  options->Delete();
  return return_value;
}
