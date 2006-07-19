/*=========================================================================

   Program: ParaView
   Module:    pqProcessModuleGUIHelper.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#include "pqApplicationCore.h"
#include "pqEventPlayer.h"
#include "pqEventPlayerXML.h"
#include "pqOptions.h"
#include "pqOutputWindowAdapter.h"
#include "pqOutputWindow.h"
#include "pqTestUtility.h"

#include <pqObjectNaming.h>

#include <vtkObjectFactory.h>
#include <vtkProcessModule.h>
#include <vtkSMApplication.h>
#include <vtkSMProperty.h>
#include <vtkSmartPointer.h>

#include <QApplication>
#include <QWidget>
////////////////////////////////////////////////////////////////////////////
// pqProcessModuleGUIHelper::pqImplementation

class pqProcessModuleGUIHelper::pqImplementation
{
public:
  pqImplementation() :
    OutputWindowAdapter(vtkSmartPointer<pqOutputWindowAdapter>::New()),
    OutputWindow(0),
    SMApplication(vtkSMApplication::New()),
    ApplicationCore(0),
    Window(0),
    EnableProgress(false)
  {
    // Redirect Qt debug output to VTK ...
    qInstallMsgHandler(QtMessageOutput);
  }
  
  ~pqImplementation()
  {
    this->SMApplication->Finalize();
    this->SMApplication->Delete();
    delete this->Window;
    delete this->OutputWindow;
    delete this->ApplicationCore;
  }

  /// Routes Qt debug output through the VTK output window mechanism
  static void QtMessageOutput(QtMsgType type, const char *msg)
  {
    switch(type)
      {
      case QtDebugMsg:
        vtkOutputWindow::GetInstance()->DisplayText(msg);
        break;
      case QtWarningMsg:
        vtkOutputWindow::GetInstance()->DisplayWarningText(msg);
        break;
      case QtCriticalMsg:
        vtkOutputWindow::GetInstance()->DisplayErrorText(msg);
        break;
      case QtFatalMsg:
        vtkOutputWindow::GetInstance()->DisplayErrorText(msg);
        break;
      }
  }

  /// Converts VTK debug output into Qt signals
  vtkSmartPointer<pqOutputWindowAdapter> OutputWindowAdapter;
  /// Displays VTK debug output in a console window
  pqOutputWindow* OutputWindow;

  vtkSMApplication* SMApplication;
  pqApplicationCore* ApplicationCore;
  QWidget* Window;
  bool EnableProgress;
};

////////////////////////////////////////////////////////////////////////////
// pqProcessModuleGUIHelper

vtkCxxRevisionMacro(pqProcessModuleGUIHelper, "1.1");
//-----------------------------------------------------------------------------
pqProcessModuleGUIHelper::pqProcessModuleGUIHelper() :
  Implementation(new pqImplementation())
{
}

//-----------------------------------------------------------------------------
pqProcessModuleGUIHelper::~pqProcessModuleGUIHelper()
{
  delete this->Implementation;
}

void pqProcessModuleGUIHelper::disableOutputWindow()
{
  this->Implementation->OutputWindowAdapter = vtkSmartPointer<pqOutputWindowAdapter>::New();
  vtkOutputWindow::SetInstance(this->Implementation->OutputWindowAdapter);
}

//-----------------------------------------------------------------------------
int pqProcessModuleGUIHelper::RunGUIStart(int argc, char** argv, 
  int vtkNotUsed(numServerProcs), int vtkNotUsed(myId))
{
  this->Implementation->SMApplication->Initialize();
  vtkSMProperty::SetCheckDomains(0);
  this->Implementation->SMApplication->ParseConfigurationFiles();
  
  if (!this->InitializeApplication(argc, argv))
    {
    return 1;
    }
  
  int status = 1;
  if (this->Implementation->Window)
    {
    this->Implementation->Window->show();
    
    // Check is options specified to run tests.
    pqOptions* options = pqOptions::SafeDownCast(
      vtkProcessModule::GetProcessModule()->GetOptions());
    
    int dont_start_event_loop = 0;
    if (options)
      {
      if (options->GetTestFileName())
        {
        pqEventPlayer player;
        pqTestUtility::Setup(player);

        pqEventPlayerXML xml_player;
        status = !xml_player.playXML(player, options->GetTestFileName());
        }

      if (options->GetBaselineImage())
        {
        status = !this->compareView(options->GetBaselineImage(),
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
      QCoreApplication* app = QApplication::instance();
      status = app->exec();
      }
    }
  this->FinalizeApplication();
  
  // If there were any errors from Qt / VTK, ensure that we return an error code
  if(!status && this->Implementation->OutputWindowAdapter->getErrorCount())
    {
    status = 1;
    }
  
  return status;
}

//-----------------------------------------------------------------------------
int pqProcessModuleGUIHelper::InitializeApplication(int vtkNotUsed(argc), 
           char** vtkNotUsed(argv))
{
  this->Implementation->ApplicationCore = new pqApplicationCore();
  
  // Redirect VTK debug output to a Qt window ...
  this->Implementation->OutputWindow = new pqOutputWindow(0);
  this->Implementation->OutputWindow->connect(this->Implementation->OutputWindowAdapter, 
    SIGNAL(displayText(const QString&)), SLOT(onDisplayText(const QString&)));
  this->Implementation->OutputWindow->connect(this->Implementation->OutputWindowAdapter, 
    SIGNAL(displayErrorText(const QString&)), SLOT(onDisplayErrorText(const QString&)));
  this->Implementation->OutputWindow->connect(this->Implementation->OutputWindowAdapter, 
    SIGNAL(displayWarningText(const QString&)), SLOT(onDisplayWarningText(const QString&)));
  this->Implementation->OutputWindow->connect(this->Implementation->OutputWindowAdapter, 
    SIGNAL(displayGenericWarningText(const QString&)), SLOT(onDisplayGenericWarningText(const QString&)));
  vtkOutputWindow::SetInstance(Implementation->OutputWindowAdapter);
  
  this->Implementation->Window = this->CreateMainWindow();

  return 1;
}

//-----------------------------------------------------------------------------
void pqProcessModuleGUIHelper::FinalizeApplication()
{
  delete this->Implementation->Window;
  this->Implementation->Window = 0;
}

//-----------------------------------------------------------------------------
void pqProcessModuleGUIHelper::SendPrepareProgress()
{
  this->Implementation->EnableProgress = true;
  this->Implementation->ApplicationCore->prepareProgress();
}

//-----------------------------------------------------------------------------
void pqProcessModuleGUIHelper::SendCleanupPendingProgress()
{
  this->Implementation->EnableProgress = false;
  this->Implementation->ApplicationCore->cleanupPendingProgress();
}

//-----------------------------------------------------------------------------
void pqProcessModuleGUIHelper::SetLocalProgress(const char* name, 
  int progress)
{
  if (!this->Implementation->EnableProgress)
    {
    return;
    }
  this->Implementation->ApplicationCore->sendProgress(name, progress);
  //cout << (name? name : "(null)") << " : " << progress << endl;
  // Here we would call something like
  // this->Window->SetProgress(name, progress). 
  // Then the Window can update the progress bar, or something.
}

//-----------------------------------------------------------------------------
void pqProcessModuleGUIHelper::ExitApplication()
{
  QCoreApplication* app = QApplication::instance();
  if(app)
    {
    app->exit();
    }
}

//-----------------------------------------------------------------------------
void pqProcessModuleGUIHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  //os << indent << "Application: " << this->Implementation->Application << endl;
}

//-----------------------------------------------------------------------------
bool pqProcessModuleGUIHelper::compareView(
  const QString& vtkNotUsed(referenceImage), 
  double vtkNotUsed(threshold), ostream& vtkNotUsed(output), 
  const QString& vtkNotUsed(tempDirectory))
{
  return false;
}
//-----------------------------------------------------------------------------
QWidget* pqProcessModuleGUIHelper::GetMainWindow()
{
  return this->Implementation->Window;
}


