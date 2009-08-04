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

#include "vtkPVConfig.h"

#include "Config.h"
#include "MainWindow.h"
#include "OverView.h"
#include "ProcessModuleGUIHelper.h"

#include <QApplication>
#include <QBitmap>
#include <QFileInfo>
#include <QMainWindow>
#include <QTimer>

#include <pqApplicationCore.h>
#include <pqPluginManager.h>
#include <vtkObjectFactory.h>

vtkStandardNewMacro(ProcessModuleGUIHelper);
vtkCxxRevisionMacro(ProcessModuleGUIHelper, "1.12");

//-----------------------------------------------------------------------------
ProcessModuleGUIHelper::ProcessModuleGUIHelper()
{
  if(QFileInfo(":/OverViewClient/SplashScreen").exists())
    {
    QPixmap pix(":/OverViewClient/SplashScreen");
    this->Splash = new QSplashScreen(pix);
    this->Splash->setMask(pix.createMaskFromColor(QColor(Qt::transparent)));
    this->Splash->setAttribute(Qt::WA_DeleteOnClose);
    this->Splash->setWindowFlags(Qt::SplashScreen|Qt::WindowStaysOnTopHint);
    this->Splash->setFont(QFont("Helvetica", 12, QFont::Bold));
    this->Splash->showMessage(
      QString("%1 %2").arg(OverView::GetBrandedApplicationTitle()).arg(OverView::GetBrandedFullVersion()),
      Qt::AlignBottom | Qt::AlignRight,
      QColor(OverView::GetBrandedSplashTextColor()));
    this->Splash->show();
    }
}

//-----------------------------------------------------------------------------
ProcessModuleGUIHelper::~ProcessModuleGUIHelper()
{
}

void ProcessModuleGUIHelper::SetWindowType(const QString& window_type)
{
  this->WindowType = window_type;
}

void ProcessModuleGUIHelper::SetConfiguredPlugins(const QStringList& configured_plugins)
{
  this->ConfiguredPlugins = configured_plugins;
}

QWidget* ProcessModuleGUIHelper::GetUserInterface()
{
  return this->UserInterface;
}

//-----------------------------------------------------------------------------
QWidget* ProcessModuleGUIHelper::CreateMainWindow()
{
  pqApplicationCore::instance()->setApplicationName(OverView::GetBrandedApplicationTitle() + " " + OverView::GetBrandedVersion());
  pqApplicationCore::instance()->setOrganizationName("Sandia National Laboratories");

  if(this->WindowType == "QMainWindow")
    {
    this->UserInterface = new QMainWindow();
    }
  else
    {
    this->UserInterface = new MainWindow();
    }
  
  QTimer::singleShot(2500, this->Splash, SLOT(close()));

  for(vtkIdType i = 0; i < this->ConfiguredPlugins.size(); ++i)
    {
    QString plugin = QApplication::applicationDirPath() + "/" + OverView::GetBrandedApplicationTitle() + "-startup/" + this->ConfiguredPlugins[i];

    // We have created a sophisticated new linking capability, one so powerful and subtle that I've decided to name it in my own honor.
    // We use this to facilitate running brands out of the build directory.
    QFile timlink(plugin + ".timlink");
    if(timlink.exists())
      {
      timlink.open(QIODevice::ReadOnly);
      plugin = timlink.readAll();
      }

    cerr << "Loading configured plugin: " << plugin.toAscii().data() << " ... ";
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

  return this->UserInterface;
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
/*
  if(MainWindow* const main_window = qobject_cast<MainWindow*>(this->GetMainWindow()))
  {
    return main_window->compareView(ReferenceImage, Threshold, Output, TempDirectory);
  }
*/ 
  return false;
}

