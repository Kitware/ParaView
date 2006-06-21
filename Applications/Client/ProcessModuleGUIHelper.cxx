/*=========================================================================

   Program:   ParaQ
Module:    ProcessModuleGUIHelper.cxx

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

#include "ProcessModuleGUIHelper.h"

#include "vtkObjectFactory.h"
#include "MainWindow.h"

#include "pqApplicationCore.h"


vtkStandardNewMacro(ProcessModuleGUIHelper);
vtkCxxRevisionMacro(ProcessModuleGUIHelper, "1.4");
//-----------------------------------------------------------------------------
ProcessModuleGUIHelper::ProcessModuleGUIHelper()
{
}

//-----------------------------------------------------------------------------
ProcessModuleGUIHelper::~ProcessModuleGUIHelper()
{
}

//-----------------------------------------------------------------------------
QWidget* ProcessModuleGUIHelper::CreateMainWindow()
{
  pqApplicationCore::instance()->setApplicationName("ParaView3.0");
  pqApplicationCore::instance()->setOrganizationName("Kitware");
  pqMainWindow* const window = new MainWindow();
  window->resize(900, 675);
  return window;
}

//-----------------------------------------------------------------------------
void ProcessModuleGUIHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
bool ProcessModuleGUIHelper::compareView(const QString& ReferenceImage, double Threshold, ostream& Output, const QString& TempDirectory)
{
  QWidget * widget=this->GetMainWindow();
  pqMainWindow* mainWindow=qobject_cast<pqMainWindow*>(widget);
  if(mainWindow)
  {
    return mainWindow->compareView(ReferenceImage,Threshold,Output,TempDirectory);
  }
  return false;
}


