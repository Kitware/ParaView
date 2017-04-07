/*=========================================================================

   Program: ParaView
   Module:    $RCS $

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

#include "pqAboutDialog.h"
#include "ui_pqAboutDialog.h"

#include "QtTestingConfigure.h"
#include "pqApplicationCore.h"
#include "pqOptions.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerResource.h"
#include "vtkNew.h"
#include "vtkPVConfig.h"
#include "vtkPVOpenGLInformation.h"
#include "vtkPVPythonInformation.h"
#include "vtkPVServerInformation.h"
#include "vtkProcessModule.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMViewProxy.h"

#include <QApplication>
#include <QFile>
#include <QHeaderView>

#include <sstream>

//-----------------------------------------------------------------------------
pqAboutDialog::pqAboutDialog(QWidget* Parent)
  : QDialog(Parent)
  , Ui(new Ui::pqAboutDialog())
{
  this->Ui->setupUi(this);
  this->setObjectName("pqAboutDialog");

  QString spashImage = QString(":/%1/SplashImage.img").arg(QApplication::applicationName());
  if (QFile::exists(spashImage))
  {
    this->Ui->Image->setPixmap(QPixmap(spashImage));
  }

  // get extra information and put it in
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pqOptions* opts = pqOptions::SafeDownCast(pm->GetOptions());

  std::ostringstream str;
  vtkIndent indent;
  opts->PrintSelf(str, indent.GetNextIndent());
  str << ends;
  QString info = str.str().c_str();
  int idx = info.indexOf("Runtime information:");
  info = info.remove(0, idx);

  this->Ui->VersionLabel->setText(
    QString("<html><b>Version: <i>%1</i></b></html>")
      .arg(QString(PARAVIEW_VERSION_FULL) + " " + QString(PARAVIEW_BUILD_ARCHITECTURE) + "-bit"));

  this->AddClientInformation();
  this->AddServerInformation();

  // this->Ui->ClientInformation->append("<a
  // href=\"http://www.paraview.org\">www.paraview.org</a>");
  //  this->Ui->ClientInformation->append("<a href=\"http://www.kitware.com\">www.kitware.com</a>");

  // For now, don't add any runtime information, it's
  // incorrect for PV3 (made sense of PV2).
  // this->Ui->Information->append("\n");
  // this->Ui->Information->append(info);
  // this->Ui->ClientInformation->moveCursor(QTextCursor::Start);
  // this->Ui->ClientInformation->viewport()->setBackgroundRole(QPalette::Window);
}

//-----------------------------------------------------------------------------
pqAboutDialog::~pqAboutDialog()
{
  delete this->Ui;
}

//-----------------------------------------------------------------------------
inline void addItem(QTreeWidget* tree, const QString& key, const QString& value)
{
  QTreeWidgetItem* item = new QTreeWidgetItem(tree);
  item->setText(0, key);
  item->setText(1, value);
}
inline void addItem(QTreeWidget* tree, const QString& key, int value)
{
  ::addItem(tree, key, QString("%1").arg(value));
}

//-----------------------------------------------------------------------------
void pqAboutDialog::AddClientInformation()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pqOptions* opts = pqOptions::SafeDownCast(pm->GetOptions());

  QTreeWidget* tree = this->Ui->ClientInformation;

  ::addItem(tree, "Version",
    QString(PARAVIEW_VERSION_FULL) + " " + QString(PARAVIEW_BUILD_ARCHITECTURE) + "-bit");
  ::addItem(tree, "Qt Version", QT_VERSION_STR);

#if defined(PARAVIEW_BUILD_ARCHITECTURE)
  ::addItem(tree, "Architecture", PARAVIEW_BUILD_ARCHITECTURE);
#else
  ::addItem(tree, "Architecture", PARAVIEW_BUILD_ARCHITECTURE);
#endif

  ::addItem(tree, "vtkIdType size", QString("%1bits").arg(8 * sizeof(vtkIdType)));

  vtkNew<vtkPVPythonInformation> pythonInfo;
  pythonInfo->CopyFromObject(NULL);

  ::addItem(tree, "Embedded Python", pythonInfo->GetPythonSupport() ? "On" : "Off");
  if (pythonInfo->GetPythonSupport())
  {
    ::addItem(tree, "Python Library Path", QString::fromStdString(pythonInfo->GetPythonPath()));
    ::addItem(
      tree, "Python Library Version", QString::fromStdString(pythonInfo->GetPythonVersion()));

    ::addItem(tree, "Python Numpy Support", pythonInfo->GetNumpySupport() ? "On" : "Off");
    if (pythonInfo->GetNumpySupport())
    {
      ::addItem(tree, "Python Numpy Path", QString::fromStdString(pythonInfo->GetNumpyPath()));
      ::addItem(
        tree, "Python Numpy Version", QString::fromStdString(pythonInfo->GetNumpyVersion()));
    }

    ::addItem(tree, "Python Matplotlib Support", pythonInfo->GetMatplotlibSupport() ? "On" : "Off");
    if (pythonInfo->GetMatplotlibSupport())
    {
      ::addItem(
        tree, "Python Matplotlib Path", QString::fromStdString(pythonInfo->GetMatplotlibPath()));
      ::addItem(tree, "Python Matplotlib Version",
        QString::fromStdString(pythonInfo->GetMatplotlibVersion()));
    }
  }

#if defined(QT_TESTING_WITH_PYTHON)
  ::addItem(tree, "Python Testing", "On");
#else
  ::addItem(tree, "Python Testing", "Off");
#endif

#if defined(PARAVIEW_USE_MPI)
  ::addItem(tree, "MPI Enabled", "On");
#else
  ::addItem(tree, "MPI Enabled", "Off");
#endif

  ::addItem(tree, "Disable Registry", opts->GetDisableRegistry() ? "On" : "Off");
  ::addItem(tree, "Test Directory", opts->GetTestDirectory());
  ::addItem(tree, "Data Directory", opts->GetDataDirectory());

  vtkNew<vtkPVOpenGLInformation> OpenGLInfo;
  OpenGLInfo->CopyFromObject(NULL);

  ::addItem(tree, "OpenGL Vendor", QString::fromStdString(OpenGLInfo->GetVendor()));
  ::addItem(tree, "OpenGL Version", QString::fromStdString(OpenGLInfo->GetVersion()));
  ::addItem(tree, "OpenGL Renderer", QString::fromStdString(OpenGLInfo->GetRenderer()));
#if QT_VERSION >= 0x050000
  tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
  tree->header()->setResizeMode(QHeaderView::ResizeToContents);
#endif
}

//-----------------------------------------------------------------------------
void pqAboutDialog::AddServerInformation()
{
  QTreeWidget* tree = this->Ui->ServerInformation;
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqServer*> servers = smmodel->findItems<pqServer*>();
  if (servers.size() > 0)
  {
    this->AddServerInformation(servers[0], tree);
#if QT_VERSION >= 0x050000
    tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
    tree->header()->setResizeMode(QHeaderView::ResizeToContents);
#endif
  }
}

//-----------------------------------------------------------------------------
void pqAboutDialog::AddServerInformation(pqServer* server, QTreeWidget* tree)
{
  vtkPVServerInformation* serverInfo = server->getServerInformation();
  if (!server->isRemote())
  {
    ::addItem(tree, "Remote Connection", "No");
    return;
  }

  const pqServerResource& resource = server->getResource();
  QString scheme = resource.scheme();
  bool separate_render_server = (scheme == "cdsrs" || scheme == "cdsrsrc");
  bool reverse_connection = (scheme == "csrc" || scheme == "cdsrsrc");
  ::addItem(tree, "Remote Connection", "Yes");
  ::addItem(tree, "Separate Render Server", separate_render_server ? "Yes" : "No");
  ::addItem(tree, "Reverse Connection", reverse_connection ? "Yes" : "No");

  // TODO: handle separate render server partitions.
  ::addItem(tree, "Number of Processes", server->getNumberOfPartitions());

  ::addItem(tree, "Disable Remote Rendering", serverInfo->GetRemoteRendering() ? "Off" : "On");
  ::addItem(tree, "IceT", serverInfo->GetUseIceT() ? "On" : "Off");

  if (serverInfo->GetTileDimensions()[0] > 0)
  {
    ::addItem(tree, "Tile Display", "On");
    ::addItem(tree, "Tile Dimensions", QString("(%1, %2)")
                                         .arg(serverInfo->GetTileDimensions()[0])
                                         .arg(serverInfo->GetTileDimensions()[1]));
    ::addItem(tree, "Tile Mullions", QString("(%1, %2)")
                                       .arg(serverInfo->GetTileMullions()[0])
                                       .arg(serverInfo->GetTileMullions()[1]));
  }
  else
  {
    ::addItem(tree, "Tile Display", "Off");
  }

  ::addItem(tree, "Write Ogg/Theora Animations", serverInfo->GetOGVSupport() ? "On" : "Off");
  ::addItem(tree, "Write AVI Animations", serverInfo->GetAVISupport() ? "On" : "Off");
  ::addItem(tree, "vtkIdType size", QString("%1bits").arg(serverInfo->GetIdTypeSize()));

  vtkSMSession* session = server->session();
  vtkNew<vtkPVPythonInformation> pythonInfo;
  session->GatherInformation(vtkPVSession::SERVERS, pythonInfo.GetPointer(), 0);
  ::addItem(tree, "Embedded Python", pythonInfo->GetPythonSupport() ? "On" : "Off");
  if (pythonInfo->GetPythonSupport())
  {
    ::addItem(tree, "Python Library Path", QString::fromStdString(pythonInfo->GetPythonPath()));
    ::addItem(
      tree, "Python Library Version", QString::fromStdString(pythonInfo->GetPythonVersion()));

    ::addItem(tree, "Python Numpy Support", pythonInfo->GetNumpySupport() ? "On" : "Off");
    if (pythonInfo->GetNumpySupport())
    {
      ::addItem(tree, "Python Numpy Path", QString::fromStdString(pythonInfo->GetNumpyPath()));
      ::addItem(
        tree, "Python Numpy Version", QString::fromStdString(pythonInfo->GetNumpyVersion()));
    }

    ::addItem(tree, "Python Matplotlib Support", pythonInfo->GetMatplotlibSupport() ? "On" : "Off");
    if (pythonInfo->GetMatplotlibSupport())
    {
      ::addItem(
        tree, "Python Matplotlib Path", QString::fromStdString(pythonInfo->GetMatplotlibPath()));
      ::addItem(tree, "Python Matplotlib Version",
        QString::fromStdString(pythonInfo->GetMatplotlibVersion()));
    }
  }

  vtkNew<vtkPVOpenGLInformation> OpenGLInfo;
  session->GatherInformation(vtkPVSession::RENDER_SERVER, OpenGLInfo.GetPointer(), 0);
  ::addItem(tree, "OpenGL Vendor", QString::fromStdString(OpenGLInfo->GetVendor()));
  ::addItem(tree, "OpenGL Version", QString::fromStdString(OpenGLInfo->GetVersion()));
  ::addItem(tree, "OpenGL Renderer", QString::fromStdString(OpenGLInfo->GetRenderer()));
}
