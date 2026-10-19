// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqWelcomeDialog.h"
#include "ui_pqWelcomeDialog.h"

#include "pqApplicationCore.h"
#include "pqClickableLabel.h"
#include "pqCoreUtilities.h"
#include "pqDesktopServicesReaction.h"
#include "pqExampleVisualizationsDialog.h"
#include "pqInterfaceTracker.h"
#include "pqLoadDataReaction.h"
#include "pqLoadStateReaction.h"
#include "pqRecentlyUsedResourceLoaderInterface.h"
#include "pqRecentlyUsedResourcesList.h"
#include "pqServer.h"
#include "pqServerConfigurationCollection.h"
#include "pqServerConnectDialog.h"
#include "pqServerConnectReaction.h"
#include "pqServerLauncher.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "vtkPVFileInformation.h"
#include "vtkPVVersionQuick.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QKeyEvent>
#include <QString>
#include <QUrl>

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
#define pqCheckBoxSignal checkStateChanged
using pqCheckState = Qt::CheckState;
#else
#define pqCheckBoxSignal stateChanged
using pqCheckState = int;
#endif
namespace
{

constexpr vtkIdType MAX_NUMBER_OF_ROWS = 5;
const QString labelButtonStyle = R"(
  QLabel {
      background-color: transparent;
      border: 1px solid transparent;
      padding: 2px;
      margin: 0px;
  }
  
  QLabel:hover {
      background-color: rgba(255, 255, 255, 150);
      border: 1px solid rgba(30, 30, 30, 80);
      border-radius: 4px;
  }
  
  QLabel:pressed {
      background-color: rgba(255, 255, 255, 60);
  }
  )";

//-----------------------------------------------------------------------------
QString getFilepath(
  const QList<pqRecentlyUsedResourceLoaderInterface*>& ifaces, const pqServerResource& resource)
{
  for (int cc = 0; cc < ifaces.size(); ++cc)
  {
    pqRecentlyUsedResourceLoaderInterface* iface = ifaces[cc];
    if (iface && iface->canLoad(resource))
    {
      return iface->label(resource);
    }
  }
  return "";
}

//-----------------------------------------------------------------------------
bool load(const QList<pqRecentlyUsedResourceLoaderInterface*>& ifaces,
  const pqServerResource& resource, pqServer* server)
{
  for (int cc = 0, max = ifaces.size(); cc < max; ++cc)
  {
    pqRecentlyUsedResourceLoaderInterface* iface = ifaces[cc];
    if (iface && iface->canLoad(resource))
    {
      return iface->load(resource, server);
    }
  }
  return false;
}
};

//-----------------------------------------------------------------------------
pqWelcomeDialog::pqWelcomeDialog(QWidget* parentObject)
  : Superclass(parentObject)
  , ui(new Ui::pqWelcomeDialog)
{
  ui->setupUi(this);

  // Keep the window decorated and movable, but hide the context help button.
  this->setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint |
    Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);

  this->setWindowFlag(Qt::WindowContextHelpButtonHint, false);

  auto addHelpLabel = [this](const QString& text, const QString& tooltip, auto onClickAction)
  {
    auto* label = new pqClickableLabel(nullptr, text, tooltip, "", nullptr, ui->HelpWidget);
    label->setStyleSheet(::labelButtonStyle);
    ui->HelpLayout->addWidget(label);

    QObject::connect(label, &pqClickableLabel::onClicked, this, onClickAction);
    return label;
  };

  addHelpLabel(tr("Getting Started Guide"), tr("Open the PDF Getting Started Guide"),
    [this](QWidget*) { this->onGettingStartedGuideClicked(); });
  addHelpLabel(tr("Exemple Visualizations"), tr("Display some exemple visualizations in ParaView"),
    [this](QWidget*) { this->onExampleVisualizationsClicked(); });
  addHelpLabel(tr("Documentation"), tr("Open the online documentation on your browser"),
    [this](QWidget*) { this->onDocumentationClicked(); });

  QObject::connect(this->ui->closeDontShowAgainButton, &QPushButton::clicked, this,
    &pqWelcomeDialog::onDoNotShowAgainButton);

  pqInterfaceTracker* itk = pqApplicationCore::instance()->interfaceTracker();
  QList<pqRecentlyUsedResourceLoaderInterface*> ifaces =
    itk->interfaces<pqRecentlyUsedResourceLoaderInterface*>();

  // Get the set of all resources in most-recently-used order ...
  const pqRecentlyUsedResourcesList::ListT& resources =
    pqApplicationCore::instance()->recentlyUsedResources().list();

  std::size_t stateFilesCount = 0;
  std::size_t dataFilesCount = 0;

  // Add recent state files and data files
  for (const auto& item : resources)
  {
    if (stateFilesCount >= ::MAX_NUMBER_OF_ROWS && dataFilesCount >= ::MAX_NUMBER_OF_ROWS)
    {
      break;
    }

    const QString filePath = ::getFilepath(ifaces, item);
    if (filePath.isEmpty())
    {
      continue;
    }

    const bool isStateFile =
      item.hasData("PARAVIEW_STATE") && stateFilesCount < ::MAX_NUMBER_OF_ROWS;
    const bool isDataFile = item.hasData("PARAVIEW_DATA") && dataFilesCount < ::MAX_NUMBER_OF_ROWS;

    if (isStateFile)
    {
      stateFilesCount++;
    }
    else if (isDataFile)
    {
      dataFilesCount++;
    }
    else
    {
      continue;
    }

    QWidget* parentWidget = isStateFile ? ui->recentStateWidget : ui->recentDataWidget;
    QLayout* targetLayout = isStateFile ? ui->StateLayout : ui->DataLayout;

    const QString fileName = QFileInfo(filePath).fileName();
    auto* fileLabel = new pqClickableLabel(nullptr, fileName, filePath, "", nullptr, parentWidget);
    fileLabel->setStyleSheet(::labelButtonStyle);

    QObject::connect(fileLabel, &pqClickableLabel::onClicked, this,
      [this, item](QWidget*)
      {
        this->hide();
        openResource(pqServerResource(item));
      });

    targetLayout->addWidget(fileLabel);
  }
  QObject::connect(ui->loadStateButton, &QPushButton::clicked, this,
    [&]()
    {
      this->hide();
      pqLoadStateReaction::loadState();
    });
  QObject::connect(ui->loadDataButton, &QPushButton::clicked, this,
    [&]()
    {
      this->hide();
      pqLoadDataReaction::loadData();
    });

  ui->loadStateButton->setFocusPolicy(Qt::NoFocus);
  ui->loadDataButton->setFocusPolicy(Qt::NoFocus);
  ui->connectServerButton->setFocusPolicy(Qt::NoFocus);
  ui->closeDontShowAgainButton->setFocusPolicy(Qt::NoFocus);

  // Add servers
  this->Configurations = pqApplicationCore::instance()->serverConfigurations().configurations();
  for (vtkIdType i = 0; i < std::min(this->Configurations.size(), ::MAX_NUMBER_OF_ROWS); i++)
  {
    const pqServerConfiguration& config = this->Configurations[i];
    pqClickableLabel* serverLabel = new pqClickableLabel(nullptr, config.name(),
      config.resource().schemeHostsPorts().toURI(), "", nullptr, ui->recentServerWidget);
    serverLabel->setStyleSheet(::labelButtonStyle);

    ui->ServerLayout->addWidget(serverLabel);
    QObject::connect(serverLabel, &pqClickableLabel::onClicked, this,
      [this, i](QWidget*)
      {
        this->hide();
        this->connectServer(i);
      });
  }
  QObject::connect(ui->connectServerButton, &QPushButton::clicked, this,
    [&]()
    {
      this->hide();
      pqServerConnectReaction::connectToServer();
    });

  resize(800, 600);
}

//-----------------------------------------------------------------------------
pqWelcomeDialog::~pqWelcomeDialog()
{
  delete ui;
}

//-----------------------------------------------------------------------------
void pqWelcomeDialog::onGettingStartedGuideClicked()
{
  QString documentationPath(vtkPVFileInformation::GetParaViewDocDirectory().c_str());
  QString paraViewGettingStartedFile = documentationPath + "/GettingStarted.pdf";
  QUrl gettingStartedURL = QUrl::fromLocalFile(paraViewGettingStartedFile);
  if (pqDesktopServicesReaction::openUrl(gettingStartedURL))
  {
    this->hide();
  }
}

//-----------------------------------------------------------------------------
void pqWelcomeDialog::onDocumentationClicked()
{
  std::string paraviewDocLink = "https://docs.paraview.org/en/latest/";
  if (PARAVIEW_VERSION_EPOCH < 999)
  {
    paraviewDocLink = std::string("https://docs.paraview.org/en/v") + PARAVIEW_VERSION;
  }
  if (pqDesktopServicesReaction::openUrl(QUrl(paraviewDocLink.c_str())))
  {
    this->hide();
  }
}

//-----------------------------------------------------------------------------
void pqWelcomeDialog::onExampleVisualizationsClicked()
{
  pqExampleVisualizationsDialog exampleDialog(this);
  exampleDialog.setModal(true);
  this->hide();
  exampleDialog.exec();
}

//-----------------------------------------------------------------------------
void pqWelcomeDialog::onDoNotShowAgainButton()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->setValue("GeneralSettings.ShowWelcomeDialog", 0);

  pqServer* server = pqApplicationCore::instance()->getActiveServer();
  if (!server)
  {
    qCritical("No active server available!");
    this->hide();
    return;
  }

  vtkSMSessionProxyManager* pxm = server->proxyManager();
  if (!pxm)
  {
    qCritical("No proxy manager!");
    this->hide();
    return;
  }

  vtkSMProxy* proxy = pxm->GetProxy("settings", "GeneralSettings");
  if (proxy)
  {
    vtkSMPropertyHelper(proxy, "ShowWelcomeDialog").Set(0);
  }
  this->hide();
}

//-----------------------------------------------------------------------------
void pqWelcomeDialog::openResource(const pqServerResource& resource)
{
  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqServer* server;

  // If a server name is set, use it to find the server, if not, use the schemehostsports instead
  if (resource.serverName().isEmpty())
  {
    const pqServerResource serverResource = resource.schemeHostsPorts();
    server = smModel->findServer(serverResource);
  }
  else
  {
    server = smModel->findServer(resource.serverName());
  }

  if (!server)
  {
    int ret =
      QMessageBox::warning(pqCoreUtilities::mainWidget(), tr("Disconnect from current server?"),
        tr("The file you opened requires connecting to a new server.\n"
           "The current connection will be closed.\n\n"
           "Are you sure you want to continue?"),
        QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::No)
    {
      return;
    }
    pqServerConfiguration configToConnect;
    if (pqServerConnectDialog::selectServer(
          configToConnect, pqCoreUtilities::mainWidget(), resource))
    {
      QScopedPointer<pqServerLauncher> launcher(pqServerLauncher::newInstance(configToConnect));
      if (launcher->connectToServer())
      {
        server = launcher->connectedServer();
      }
    }
  }

  if (server)
  {
    this->openResource(server, resource);
  }
}

//-----------------------------------------------------------------------------
bool pqWelcomeDialog::openResource(pqServer* server, const pqServerResource& resource)
{
  pqInterfaceTracker* itk = pqApplicationCore::instance()->interfaceTracker();
  QList<pqRecentlyUsedResourceLoaderInterface*> ifaces =
    itk->interfaces<pqRecentlyUsedResourceLoaderInterface*>();
  this->hide();
  if (::load(ifaces, resource, server))
  {
    pqRecentlyUsedResourcesList& mruList = pqApplicationCore::instance()->recentlyUsedResources();
    mruList.add(resource);
    mruList.save(*pqApplicationCore::instance()->settings());
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void pqWelcomeDialog::connectServer(int serverIndex)
{
  this->hide();
  if (serverIndex < 0 || serverIndex >= this->Configurations.size())
  {
    return;
  }

  pqServerConnectReaction::connectToServerWithWarning(this->Configurations[serverIndex]);
}
