/*=========================================================================

   Program:   ParaQ
   Module:    pqProcessModuleGUIHelper.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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
#include "pqProcessModuleGUIHelper.h"

#include "pqEventPlayer.h"
#include "pqEventPlayerXML.h"
#include "pqMainWindow.h"
#include "pqOptions.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMApplication.h"
#include "vtkSMProperty.h"

#include <pqObjectNaming.h>

#include <QApplication>

vtkCxxRevisionMacro(pqProcessModuleGUIHelper, "1.2");
//-----------------------------------------------------------------------------
pqProcessModuleGUIHelper::pqProcessModuleGUIHelper()
{
  this->SMApplication = vtkSMApplication::New();
  this->Application = 0;
  this->Window = 0;
}

//-----------------------------------------------------------------------------
pqProcessModuleGUIHelper::~pqProcessModuleGUIHelper()
{
  this->SMApplication->Finalize();
  this->SMApplication->Delete();
  if (this->Window)
    {
    delete this->Window;
    }
  if (this->Application)
    {
    delete this->Application;
    }
}

//-----------------------------------------------------------------------------
int pqProcessModuleGUIHelper::RunGUIStart(int argc, char** argv, 
  int vtkNotUsed(numServerProcs), int vtkNotUsed(myId))
{
  this->SMApplication->Initialize();
  vtkSMProperty::SetCheckDomains(0);
  this->SMApplication->ParseConfigurationFiles();
  
  if (!this->InitializeApplication(argc, argv))
    {
    return 1;
    }
  
  int status = 1;
  if (this->Application && this->Window)
    {
    this->Window->show();
    
    // Check is options specified to run tests.
    pqOptions* options = pqOptions::SafeDownCast(
      vtkProcessModule::GetProcessModule()->GetOptions());
    
    int dont_start_event_loop = 0;
    if (options)
      {
      if(options->GetTestUINames())
        {
        status = !pqObjectNaming::Validate(*this->Window);
        }

      if (options->GetTestFileName())
        {
        pqEventPlayer player(*this->Window);
        player.addDefaultWidgetEventPlayers();
        pqEventPlayerXML xml_player;
        status = !xml_player.playXML(player, options->GetTestFileName());
        }

      if (options->GetBaselineImage())
        {
        status = !this->Window->compareView(options->GetBaselineImage(),
          options->GetImageThreshold(), cout, options->GetTestDirectory());
        dont_start_event_loop = 1;
        }
      if (options->GetExitBeforeEventLoop())
        {
        dont_start_event_loop = 1;
        }
      }
    
    if (!dont_start_event_loop )
      {
      // Starts the event loop.
      status = this->Application->exec();
      }
    }
  this->FinalizeApplication();
  return status;
}

//-----------------------------------------------------------------------------
int pqProcessModuleGUIHelper::InitializeApplication(int argc, char** argv)
{
  this->Application = new QApplication(argc, argv);
  
/** \todo Figure-out how to export Qt's resource symbols from a DLL, so we can use them here */
#if !(defined(WIN32) && defined(PARAQ_BUILD_SHARED_LIBS))
  Q_INIT_RESOURCE(pqWidgets);
#endif
  this->Window = this->CreateMainWindow();

  return 1;
}

//-----------------------------------------------------------------------------
void pqProcessModuleGUIHelper::FinalizeApplication()
{
  delete this->Window;
  this->Window = NULL;
}

//-----------------------------------------------------------------------------
void pqProcessModuleGUIHelper::SendPrepareProgress()
{
}

//-----------------------------------------------------------------------------
void pqProcessModuleGUIHelper::SendCleanupPendingProgress()
{
}

//-----------------------------------------------------------------------------
void pqProcessModuleGUIHelper::SetLocalProgress(const char* vtkNotUsed(name), 
  int vtkNotUsed(progress))
{
  // Here we would call something like
  // this->Window->SetProgress(name, progress). 
  // Then the Window can update the progress bar, or something.
}

//-----------------------------------------------------------------------------
void pqProcessModuleGUIHelper::ExitApplication()
{
  if (this->Application)
    {
    this->Application->exit();
    }
}

//-----------------------------------------------------------------------------
void pqProcessModuleGUIHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Application: " << this->Application << endl;
}
