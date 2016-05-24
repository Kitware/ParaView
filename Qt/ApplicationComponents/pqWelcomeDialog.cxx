#include "pqWelcomeDialog.h"
#include "ui_pqWelcomeDialog.h"

#include "pqApplicationCore.h"
#include "pqDesktopServicesReaction.h"
#include "pqExampleVisualizationsDialog.h"
#include "pqServer.h"
#include "pqSettings.h"

#include <QCoreApplication>
#include <QString>
#include <QUrl>

#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"

//-----------------------------------------------------------------------------
pqWelcomeDialog::pqWelcomeDialog(QWidget *parent)
  : Superclass (parent),
    ui(new Ui::pqWelcomeDialog)
{
    ui->setupUi(this);

    QObject::connect(this->ui->DoNotShowAgainButton, SIGNAL(stateChanged(int)),
                     this, SLOT(onDoNotShowAgainStateChanged(int)));
    QObject::connect(this->ui->GettingStartedGuideButton, SIGNAL(clicked(bool)),
                     this, SLOT(onGettingStartedGuideClicked()));
    QObject::connect(this->ui->ExampleVisualizationsButton, SIGNAL(clicked(bool)),
                     this, SLOT(onExampleVisualizationsClicked()));
}

//-----------------------------------------------------------------------------
pqWelcomeDialog::~pqWelcomeDialog()
{
    delete ui;
}

//-----------------------------------------------------------------------------
void pqWelcomeDialog::onGettingStartedGuideClicked()
{
#if defined(_WIN32) || defined(__APPLE__)
  QString paraViewGettingStartedFile = QCoreApplication::applicationDirPath() + "/../doc/GettingStarted.pdf";
#else
  QString paraViewGettingStartedFile = QCoreApplication::applicationDirPath() + "/../../doc/GettingStarted.pdf";
#endif
  QUrl gettingStartedURL = QUrl::fromLocalFile(paraViewGettingStartedFile);
  pqDesktopServicesReaction::openUrl(gettingStartedURL);
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
  bool show = (state != Qt::Checked);

  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->setValue("GeneralSettings.ShowWelcomeDialog", show ? 1 : 0);

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
    vtkSMPropertyHelper(proxy, "ShowWelcomeDialog").Set(show ? 1 : 0);
    }
}
