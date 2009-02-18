/*=========================================================================

   Program: ParaView
   Module:    pqClientProcessModuleGUIHelper.cxx

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

#include "pqClientProcessModuleGUIHelper.h"

#include <QTimer>
#include <QBitmap>
#include "pqClientMainWindow.h"

#include <pqApplicationCore.h>
#include <vtkObjectFactory.h>

#include "vtkPVConfig.h"

vtkStandardNewMacro(pqClientProcessModuleGUIHelper);
vtkCxxRevisionMacro(pqClientProcessModuleGUIHelper, "1.2");

//-----------------------------------------------------------------------------
pqClientProcessModuleGUIHelper::pqClientProcessModuleGUIHelper()
{
  QPixmap pix(":/pqWidgets/Icons/PVSplashScreen.png");
  this->Splash = new QSplashScreen(pix, Qt::SplashScreen|Qt::WindowStaysOnTopHint);
  this->Splash->setMask(pix.createMaskFromColor(QColor(Qt::transparent)));
  this->Splash->setAttribute(Qt::WA_DeleteOnClose);
  this->Splash->show();
}

//-----------------------------------------------------------------------------
pqClientProcessModuleGUIHelper::~pqClientProcessModuleGUIHelper()
{
}

//-----------------------------------------------------------------------------
QWidget* pqClientProcessModuleGUIHelper::CreateMainWindow()
{
  pqApplicationCore::instance()->setApplicationName("ParaView" PARAVIEW_VERSION);
  pqApplicationCore::instance()->setOrganizationName("ParaView");
  QWidget* w = new pqClientMainWindow();
  QTimer::singleShot(10, this->Splash, SLOT(close()));
  return w;
}

//-----------------------------------------------------------------------------
void pqClientProcessModuleGUIHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
bool pqClientProcessModuleGUIHelper::compareView(const QString& ReferenceImage,
  double Threshold, ostream& Output, const QString& TempDirectory)
{
  if(pqClientMainWindow* const main_window = qobject_cast<pqClientMainWindow*>(this->GetMainWindow()))
  {
    return main_window->compareView(ReferenceImage, Threshold, Output, TempDirectory);
  }
  
  return false;
}


//-----------------------------------------------------------------------------
int pqClientProcessModuleGUIHelper::RunGUIStart(int argc, char** argv,
  int vtkNotUsed(numServerProcs), int vtkNotUsed(myId))
{
  int not_used_numServerProcs = 0;
  int not_used_myId = 0;
  return pqProcessModuleGUIHelper::RunGUIStart(argc, argv, not_used_numServerProcs, not_used_myId);
}
