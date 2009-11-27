/*=========================================================================

   Program: ParaView
   Module:    pqMain.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "pqMain.h"

#include "vtkInitializationHelper.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h" // Required to get build options for paraview
#include "vtkPVMain.h"
#include "vtkToolkits.h" // For VTK_USE_MPI

#include <QApplication>

#include "pqOptions.h"
#include "pqProcessModuleGUIHelper.h"

//----------------------------------------------------------------------------
void ParaViewInitializeInterpreter(vtkProcessModule* pm)
{
  // Initialize built-in wrapper modules.
  vtkInitializationHelper::InitializeInterpretor(pm);
}

vtkPVMain * pqMain::PVMain = NULL;
pqOptions * pqMain::PVOptions = NULL;
pqProcessModuleGUIHelper * pqMain::PVHelper = NULL;
static bool PVOptionsAllocatedInternally = false;

//----------------------------------------------------------------------------
int pqMain::preRun(QApplication& app, pqProcessModuleGUIHelper* helperIn,
  pqOptions * & optionsIn)
{
  PVHelper = helperIn;

  int argc = app.argc();
  char** argv = app.argv();

  vtkPVMain::SetUseMPI(0);  // pvClient never runs with MPI.
  vtkPVMain::Initialize(&argc, &argv); // Perform any initializations.

  PVMain = vtkPVMain::New();

  if (optionsIn == NULL)
    {
    optionsIn = pqOptions::New();
    PVOptionsAllocatedInternally = true;
    // We may define a PQCLIENT enum, if necessary.
    optionsIn->SetProcessType(vtkPVOptions::PVCLIENT);
    }

  PVOptions = optionsIn;
 
  // This creates the Process Module and initializes it.
  int ret = PVMain->Initialize(PVOptions, PVHelper, ParaViewInitializeInterpreter, 
                               argc, argv);

  return ret;
}

//----------------------------------------------------------------------------
int pqMain::Run(pqOptions * options)
{
  vtkProcessModule::GetProcessModule()->SupportMultipleConnectionsOn();
  return PVHelper->Run(options);
}

//----------------------------------------------------------------------------
int pqMain::Run(QApplication& app, pqProcessModuleGUIHelper * helperIn)
{
  int argc = app.argc();
  char** argv = app.argv();
  PVHelper = helperIn;

  vtkPVMain::SetUseMPI(0);  // pvClient never runs with MPI.
  vtkPVMain::Initialize(&argc, &argv); // Perform any initializations.

  PVMain = vtkPVMain::New();
  PVOptions = pqOptions::New();
  // We may define a PQCLIENT enum, if necessary.
  PVOptions->SetProcessType(vtkPVOptions::PVCLIENT);
 
  // This creates the Process Module and initializes it.
  int ret = PVMain->Initialize(PVOptions, PVHelper, ParaViewInitializeInterpreter, 
                               argc, argv);
  if (!ret)
    {
    // Tell process module that we support Multiple connections.
    // This must be set before starting the event loop.
    vtkProcessModule::GetProcessModule()->SupportMultipleConnectionsOn();
    ret = PVHelper->Run(PVOptions);
    }

  PVOptions->Delete();
  PVMain->Delete();
  vtkPVMain::Finalize();
  vtkProcessModule::SetProcessModule(0);
  
  return ret;
}

//----------------------------------------------------------------------------
void pqMain::postRun()
{
  // delete PVMain, as it was allocated in this file/class, rely on caller
  // to properly delete helper
  if (PVMain) 
    {
    PVMain->Delete();
    }

  // check to see if options was allocated in this class
  if (PVOptionsAllocatedInternally)
  {
    PVOptions->Delete();
  }

  vtkPVMain::Finalize();
  vtkProcessModule::SetProcessModule(0);
}
