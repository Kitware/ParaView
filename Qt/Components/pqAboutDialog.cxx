// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqAboutDialog.h"
#include "ui_pqAboutDialog.h"

#include "QtTestingConfigure.h"
#include "pqApplicationCore.h"
#include "pqCoreConfiguration.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerResource.h"
#include "vtkNew.h"
#include "vtkPVOpenGLInformation.h"
#include "vtkPVPythonInformation.h"
#include "vtkPVRenderingCapabilitiesInformation.h"
#include "vtkPVServerInformation.h"
#include "vtkPVVersion.h"
#include "vtkProcessModule.h"
#include "vtkRemotingCoreConfiguration.h"
#include "vtkSMPTools.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMViewProxy.h"
#include "vtkVersion.h"
#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include <QApplication>
#include <QClipboard>
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
  // hide the Context Help item (it's a "?" in the Title Bar for Windows, a menu item for Linux)
  this->setWindowFlags(this->windowFlags().setFlag(Qt::WindowContextHelpButtonHint, false));

  QString spashImage = QString(":/%1/SplashImage.img").arg(QApplication::applicationName());
  if (QFile::exists(spashImage))
  {
    this->Ui->Image->setPixmap(QPixmap(spashImage));
  }

  // get extra information and put it in
  this->Ui->VersionLabel->setText(QString("<html><b>%1: <i>%2</i></b></html>")
                                    .arg(tr("Version"))
                                    .arg(QString(PARAVIEW_VERSION_FULL)));
  this->AddClientInformation();
  this->AddServerInformation();
}

//-----------------------------------------------------------------------------
pqAboutDialog::~pqAboutDialog()
{
  delete this->Ui;
}

//-----------------------------------------------------------------------------
namespace
{
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
}

//-----------------------------------------------------------------------------
void pqAboutDialog::AddClientInformation()
{

  QTreeWidget* tree = this->Ui->ClientInformation;

  ::addItem(tree, tr("Version"), QString(PARAVIEW_VERSION_FULL));
  ::addItem(tree, tr("VTK Version"), QString(vtkVersion::GetVTKVersionFull()));
  ::addItem(tree, tr("Qt Version"), QT_VERSION_STR);

  ::addItem(tree, tr("vtkIdType size"), tr("%1bits").arg(8 * sizeof(vtkIdType)));

  vtkNew<vtkPVPythonInformation> pythonInfo;
  pythonInfo->CopyFromObject(nullptr);

  ::addItem(tree, tr("Embedded Python"), pythonInfo->GetPythonSupport() ? tr("On") : tr("Off"));
  if (pythonInfo->GetPythonSupport())
  {
    ::addItem(tree, tr("Python Library Path"), QString::fromStdString(pythonInfo->GetPythonPath()));
    ::addItem(
      tree, tr("Python Library Version"), QString::fromStdString(pythonInfo->GetPythonVersion()));

    ::addItem(
      tree, tr("Python Numpy Support"), pythonInfo->GetNumpySupport() ? tr("On") : tr("Off"));
    if (pythonInfo->GetNumpySupport())
    {
      ::addItem(tree, tr("Python Numpy Path"), QString::fromStdString(pythonInfo->GetNumpyPath()));
      ::addItem(
        tree, tr("Python Numpy Version"), QString::fromStdString(pythonInfo->GetNumpyVersion()));
    }

    ::addItem(tree, tr("Python Matplotlib Support"),
      pythonInfo->GetMatplotlibSupport() ? tr("On") : tr("Off"));
    if (pythonInfo->GetMatplotlibSupport())
    {
      ::addItem(tree, tr("Python Matplotlib Path"),
        QString::fromStdString(pythonInfo->GetMatplotlibPath()));
      ::addItem(tree, tr("Python Matplotlib Version"),
        QString::fromStdString(pythonInfo->GetMatplotlibVersion()));
    }
  }

#if defined(QT_TESTING_WITH_PYTHON)
  ::addItem(tree, tr("Python Testing"), tr("On"));
#else
  ::addItem(tree, tr("Python Testing"), tr("Off"));
#endif

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  ::addItem(tree, tr("MPI Enabled"), tr("On"));
#else
  ::addItem(tree, tr("MPI Enabled"), tr("Off"));
#endif

#ifdef PARAVIEW_BUILD_ID
  ::addItem(tree, tr("ParaView Build ID"), PARAVIEW_BUILD_ID);
#endif

  auto coreConfig = vtkRemotingCoreConfiguration::GetInstance();
  auto pqConfig = pqCoreConfiguration::instance();
  ::addItem(tree, tr("Disable Registry"), coreConfig->GetDisableRegistry() ? tr("On") : tr("Off"));
  ::addItem(tree, tr("Test Directory"), QString::fromStdString(pqConfig->testDirectory()));
  ::addItem(tree, tr("Data Directory"), QString::fromStdString(pqConfig->dataDirectory()));

  ::addItem(tree, tr("SMP Backend"), vtkSMPTools::GetBackend());
  ::addItem(tree, tr("SMP Max Number of Threads"), vtkSMPTools::GetEstimatedNumberOfThreads());

  // For local OpenGL info, we ask Qt, as that's more truthful anyways.
  QOpenGLContext* ctx = QOpenGLContext::currentContext();
  if (QOpenGLFunctions* f = ctx ? ctx->functions() : nullptr)
  {
    const char* glVendor = reinterpret_cast<const char*>(f->glGetString(GL_VENDOR));
    const char* glRenderer = reinterpret_cast<const char*>(f->glGetString(GL_RENDERER));
    const char* glVersion = reinterpret_cast<const char*>(f->glGetString(GL_VERSION));
    ::addItem(tree, tr("OpenGL Vendor"), glVendor);
    ::addItem(tree, tr("OpenGL Version"), glVersion);
    ::addItem(tree, tr("OpenGL Renderer"), glRenderer);
  }

#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmFilters && VTK_ENABLE_VISKORES_OVERRIDES
  ::addItem(tree, tr("Accelerated filters overrides available"), tr("Yes"));
#else
  ::addItem(tree, tr("Accelerated filters overrides available"), tr("No"));
#endif

  tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

//-----------------------------------------------------------------------------
void pqAboutDialog::AddServerInformation()
{
  QTreeWidget* tree = this->Ui->ServerInformation;
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqServer*> servers = smmodel->findItems<pqServer*>();
  if (!servers.empty())
  {
    this->AddServerInformation(servers[0], tree);
    tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  }
}

//-----------------------------------------------------------------------------
void pqAboutDialog::AddServerInformation(pqServer* server, QTreeWidget* tree)
{
  vtkPVServerInformation* serverInfo = server->getServerInformation();
  if (!server->isRemote())
  {
    ::addItem(tree, tr("Remote Connection"), tr("No"));
    return;
  }

  const pqServerResource& resource = server->getResource();
  QString scheme = resource.scheme();
  bool separate_render_server = (scheme == "cdsrs" || scheme == "cdsrsrc");
  bool reverse_connection = (scheme == "csrc" || scheme == "cdsrsrc");
  ::addItem(tree, tr("Remote Connection"), tr("Yes"));
  ::addItem(tree, tr("Separate Render Server"), separate_render_server ? tr("Yes") : tr("No"));
  ::addItem(tree, tr("Reverse Connection"), reverse_connection ? tr("Yes") : tr("No"));

  // TODO: handle separate render server partitions.
  ::addItem(tree, tr("Number of Processes"), server->getNumberOfPartitions());

  ::addItem(
    tree, tr("Disable Remote Rendering"), serverInfo->GetRemoteRendering() ? tr("Off") : tr("On"));
  ::addItem(tree, tr("IceT"), qPrintable(serverInfo->GetUseIceT() ? tr("On") : tr("Off")));

  if (serverInfo->GetIsInTileDisplay())
  {
    ::addItem(tree, tr("Tile Display"), tr("On"));
  }
  else
  {
    ::addItem(tree, tr("Tile Display"), tr("Off"));
  }

  ::addItem(tree, tr("vtkIdType size"), tr("%1bits").arg(serverInfo->GetIdTypeSize()));

  ::addItem(tree, tr("SMP Backend"), serverInfo->GetSMPBackendName().c_str());
  ::addItem(tree, tr("SMP Max Number of Threads"), serverInfo->GetSMPMaxNumberOfThreads());

  vtkSMSession* session = server->session();
  vtkNew<vtkPVPythonInformation> pythonInfo;
  session->GatherInformation(vtkPVSession::SERVERS, pythonInfo.GetPointer(), 0);
  ::addItem(tree, tr("Embedded Python"), pythonInfo->GetPythonSupport() ? tr("On") : tr("Off"));
  if (pythonInfo->GetPythonSupport())
  {
    ::addItem(tree, tr("Python Library Path"), QString::fromStdString(pythonInfo->GetPythonPath()));
    ::addItem(
      tree, tr("Python Library Version"), QString::fromStdString(pythonInfo->GetPythonVersion()));

    ::addItem(
      tree, tr("Python Numpy Support"), pythonInfo->GetNumpySupport() ? tr("On") : tr("Off"));
    if (pythonInfo->GetNumpySupport())
    {
      ::addItem(tree, tr("Python Numpy Path"), QString::fromStdString(pythonInfo->GetNumpyPath()));
      ::addItem(
        tree, tr("Python Numpy Version"), QString::fromStdString(pythonInfo->GetNumpyVersion()));
    }

    ::addItem(tree, tr("Python Matplotlib Support"),
      pythonInfo->GetMatplotlibSupport() ? tr("On") : tr("Off"));
    if (pythonInfo->GetMatplotlibSupport())
    {
      ::addItem(tree, tr("Python Matplotlib Path"),
        QString::fromStdString(pythonInfo->GetMatplotlibPath()));
      ::addItem(tree, tr("Python Matplotlib Version"),
        QString::fromStdString(pythonInfo->GetMatplotlibVersion()));
    }
  }

  vtkNew<vtkPVRenderingCapabilitiesInformation> renInfo;
  session->GatherInformation(vtkPVSession::RENDER_SERVER, renInfo.GetPointer(), 0);
  if (renInfo->Supports(vtkPVRenderingCapabilitiesInformation::RENDERING))
  {
    vtkNew<vtkPVOpenGLInformation> OpenGLInfo;
    session->GatherInformation(vtkPVSession::RENDER_SERVER, OpenGLInfo.GetPointer(), 0);
    ::addItem(tree, tr("OpenGL Vendor"), QString::fromStdString(OpenGLInfo->GetVendor()));
    ::addItem(tree, tr("OpenGL Version"), QString::fromStdString(OpenGLInfo->GetVersion()));
    ::addItem(tree, tr("OpenGL Renderer"), QString::fromStdString(OpenGLInfo->GetRenderer()));
    ::addItem(tree, tr("Window Backend"), QString::fromStdString(OpenGLInfo->GetWindowBackend()));

    if (renInfo->Supports(vtkPVRenderingCapabilitiesInformation::HEADLESS_RENDERING))
    {
      std::ostringstream headlessModes;
      if (renInfo->Supports(vtkPVRenderingCapabilitiesInformation::HEADLESS_RENDERING_USES_EGL))
      {
        headlessModes << "EGL ";
      }
      if (renInfo->Supports(vtkPVRenderingCapabilitiesInformation::HEADLESS_RENDERING_USES_OSMESA))
      {
        headlessModes << "OSMesa";
      }
      ::addItem(
        tree, tr("Supported Headless Backends"), QString::fromStdString(headlessModes.str()));
    }
    else
    {
      ::addItem(tree, tr("Supported Headless Backends"), tr("None"));
    }
  }
  else
  {
    ::addItem(tree, "OpenGL", tr("Not supported"));
  }

  ::addItem(tree, tr("Accelerated Filters Overrides Available"),
    serverInfo->GetAcceleratedFiltersOverrideAvailable() ? tr("Yes") : tr("No"));
}

//-----------------------------------------------------------------------------
QString pqAboutDialog::formatToText(QTreeWidget* tree)
{
  QString text;
  QTreeWidgetItemIterator it(tree);
  while (*it)
  {
    text += (*it)->text(0) + ": " + (*it)->text(1) + "\n";
    ++it;
  }
  return text;
}

//-----------------------------------------------------------------------------
QString pqAboutDialog::formatToText()
{
  QString text = tr("Client Information:\n");
  QTreeWidget* tree = this->Ui->ClientInformation;
  text += this->formatToText(tree);
  tree = this->Ui->ServerInformation;
  text += tr("\nConnection Information:\n");
  text += this->formatToText(tree);
  return text;
}

//-----------------------------------------------------------------------------
void pqAboutDialog::saveToFile()
{
  pqFileDialog fileDialog(nullptr, pqCoreUtilities::mainWidget(), tr("Save to File"), QString(),
    tr("Text Files") + " (*.txt);;" + tr("All Files") + " (*)", false);
  fileDialog.setFileMode(pqFileDialog::AnyFile);
  if (fileDialog.exec() != pqFileDialog::Accepted)
  {
    // Canceled
    return;
  }

  QString filename = fileDialog.getSelectedFiles().first();
  QByteArray filename_ba = filename.toUtf8();
  std::ofstream fileStream;
  fileStream.open(filename_ba.data());
  if (fileStream.is_open())
  {
    fileStream << this->formatToText().toStdString();
    fileStream.close();
  }
}

//-----------------------------------------------------------------------------
void pqAboutDialog::copyToClipboard()
{
  QClipboard* clipboard = QGuiApplication::clipboard();
  clipboard->setText(this->formatToText());
}
