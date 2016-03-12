#include "pqWelcomeDialog.h"
#include "ui_pqWelcomeDialog.h"

#include "pqApplicationCore.h"
#include "pqServer.h"
#include "pqSettings.h"

#include <QCoreApplication>
#include <QString>

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
}

//-----------------------------------------------------------------------------
pqWelcomeDialog::~pqWelcomeDialog()
{
    delete ui;
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
