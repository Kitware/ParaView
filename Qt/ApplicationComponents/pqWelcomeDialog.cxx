#include "pqWelcomeDialog.h"
#include "ui_pqWelcomeDialog.h"

#include "pqApplicationCore.h"
#include "pqDesktopServicesReaction.h"
#include "pqExampleVisualizationsDialog.h"
#include "pqServer.h"
#include "pqSettings.h"
#include "vtkPVConfig.h" // for PARAVIEW_VERSION
#include "vtkPVFileInformation.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QString>
#include <QUrl>

//-----------------------------------------------------------------------------
pqWelcomeDialog::pqWelcomeDialog(QWidget* parentObject)
  : Superclass(parentObject)
  , ui(new Ui::pqWelcomeDialog)
{
  ui->setupUi(this);

  QObject::connect(this->ui->DoNotShowAgainButton, SIGNAL(stateChanged(int)), this,
    SLOT(onDoNotShowAgainStateChanged(int)));
  QObject::connect(this->ui->GettingStartedGuideButton, SIGNAL(clicked(bool)), this,
    SLOT(onGettingStartedGuideClicked()));
  QObject::connect(this->ui->ExampleVisualizationsButton, SIGNAL(clicked(bool)), this,
    SLOT(onExampleVisualizationsClicked()));
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
void pqWelcomeDialog::onExampleVisualizationsClicked()
{
  pqExampleVisualizationsDialog exampleDialog(this);
  exampleDialog.setModal(true);
  this->hide();
  exampleDialog.exec();
}

//-----------------------------------------------------------------------------
void pqWelcomeDialog::onDoNotShowAgainStateChanged(int state)
{
  bool showDialog = (state != Qt::Checked);

  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->setValue("GeneralSettings.ShowWelcomeDialog", showDialog ? 1 : 0);

  pqServer* server = pqApplicationCore::instance()->getActiveServer();
  if (!server)
  {
    qCritical("No active server available!");
    return;
  }

  vtkSMSessionProxyManager* pxm = server->proxyManager();
  if (!pxm)
  {
    qCritical("No proxy manager!");
    return;
  }

  vtkSMProxy* proxy = pxm->GetProxy("settings", "GeneralSettings");
  if (proxy)
  {
    vtkSMPropertyHelper(proxy, "ShowWelcomeDialog").Set(showDialog ? 1 : 0);
  }
}
