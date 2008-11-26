/*=========================================================================

   Program: ParaView
   Module:    ProcessModuleGUIHelper.cxx

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

#include "Config.h"
#include "ProcessModuleGUIHelper.h"
#include "OverView.h"

#include <QApplication>
#include <QTimer>
#include <QBitmap>
#include "MainWindow.h"

#include <pqApplicationCore.h>
#include <pqPluginManager.h>
#include <vtkObjectFactory.h>

#include "vtkPVConfig.h"

vtkStandardNewMacro(ProcessModuleGUIHelper);
vtkCxxRevisionMacro(ProcessModuleGUIHelper, "1.2");

//-----------------------------------------------------------------------------
ProcessModuleGUIHelper::ProcessModuleGUIHelper()
{
  QPixmap pix(":/OverViewClient/SplashScreen");
  this->Splash = new QSplashScreen(pix);
  this->Splash->setMask(pix.createMaskFromColor(QColor(Qt::transparent)));
  this->Splash->setAttribute(Qt::WA_DeleteOnClose);
  this->Splash->setWindowFlags(Qt::WindowStaysOnTopHint);
  this->Splash->setFont(QFont("Helvetica", 12, QFont::Bold));
  this->Splash->showMessage(
    QString("%1 %2").arg(OverView::GetBrandedApplicationTitle()).arg(OverView::GetBrandedFullVersion()),
    Qt::AlignBottom | Qt::AlignRight,
    QColor(OverView::GetBrandedSplashTextColor()));
  this->Splash->show();
}

//-----------------------------------------------------------------------------
ProcessModuleGUIHelper::~ProcessModuleGUIHelper()
{
}

void ProcessModuleGUIHelper::SetConfiguredPlugins(const QStringList& configured_plugins)
{
  this->ConfiguredPlugins = configured_plugins;
}

//-----------------------------------------------------------------------------
QWidget* ProcessModuleGUIHelper::CreateMainWindow()
{
  pqApplicationCore::instance()->setApplicationName(OverView::GetBrandedApplicationTitle() + " " + OverView::GetBrandedVersion());
  pqApplicationCore::instance()->setOrganizationName("Sandia National Laboratories");
  QWidget* w = new MainWindow();
  QTimer::singleShot(3500, this->Splash, SLOT(close()));

  for(vtkIdType i = 0; i < this->ConfiguredPlugins.size(); ++i)
    {
    const QString plugin = QApplication::applicationDirPath() + "/" + this->ConfiguredPlugins[i];

    cerr << "Loading startup plugin: " << plugin.toAscii().data() << " ... ";
    QString error_message;
    if(pqPluginManager::NOTLOADED == pqApplicationCore::instance()->getPluginManager()->loadExtension(0, plugin, &error_message))
      {
      cerr << "failed: " << error_message.toAscii().data() << endl;
      }
    else
      {
      cerr << "succeeded" << endl;
      }
    }

  return w;
}

//-----------------------------------------------------------------------------
void ProcessModuleGUIHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
bool ProcessModuleGUIHelper::compareView(const QString& ReferenceImage,
  double Threshold, ostream& Output, const QString& TempDirectory)
{
  if(MainWindow* const main_window = qobject_cast<MainWindow*>(this->GetMainWindow()))
  {
    return main_window->compareView(ReferenceImage, Threshold, Output, TempDirectory);
  }
  
  return false;
}

