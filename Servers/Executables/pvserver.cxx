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
#include "vtkPVConfig.h" // Required to get build options for paraview
#include "vtkInitializationHelper.h"
#include "vtkPVServerOptions.h"
#include "vtkSMSession.h"

#include "paraview.h"

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  // Init current process type
  vtkPVServerOptions* options = vtkPVServerOptions::New();
  vtkInitializationHelper::Initialize( argc, argv,
                                       vtkProcessModule::PROCESS_SERVER,
                                       options );

  // Start ParaView processing loop with an automatic session connect call
  int ret_val = ParaView::RunAndConnect();

  // Exit application
  vtkInitializationHelper::Finalize();
  options->Delete();
  return ret_val;
}
