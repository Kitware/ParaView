// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqSetBreakpointDialog.h"
#include "ui_pqSetBreakpointDialog.h"

#include <QMessageBox>

#include "pqApplicationCore.h"
#include "pqLiveInsituManager.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"

#include "vtkSMLiveInsituLinkProxy.h"

//-----------------------------------------------------------------------------
pqSetBreakpointDialog::pqSetBreakpointDialog(QWidget* Parent)
  : QDialog(Parent)
  , Ui(new Ui::pqSetBreakpointDialog())
{
  this->Ui->setupUi(this);
  this->setObjectName("pqSetBreakpointDialog");
  QObject::connect(this->Ui->ButtonBox, SIGNAL(accepted()), this, SLOT(onAccepted()));
  QObject::connect(
    pqLiveInsituManager::instance(), SIGNAL(timeUpdated()), this, SLOT(onTimeUpdated()));
}

//-----------------------------------------------------------------------------
pqSetBreakpointDialog::~pqSetBreakpointDialog()
{
  delete this->Ui;
}

//-----------------------------------------------------------------------------
void pqSetBreakpointDialog::onAccepted()
{
  pqLiveInsituManager* server = pqLiveInsituManager::instance();
  QString timeString = this->Ui->BreakpointTime->text();
  bool ok = false;
  if (this->Ui->buttonGroup->checkedButton() == this->Ui->radioButtonTime)
  {
    double time = timeString.toDouble(&ok);
    if (ok && time > server->time())
    {
      server->setBreakpoint(time);
      this->accept();
      return;
    }
  }
  else
  {
    // maybe vtkIdType is smaller
    vtkIdType timeStep = static_cast<vtkIdType>(timeString.toLongLong(&ok));
    if (ok && timeStep > server->timeStep())
    {
      server->setBreakpoint(timeStep);
      this->accept();
      return;
    }
  }
  QMessageBox message(this);
  message.setText(tr("Breakpoint time is invalid or "
                     "it is smaller than current time"));
  message.exec();
}

//-----------------------------------------------------------------------------
void pqSetBreakpointDialog::onTimeUpdated()
{
  QString timeString = QString("%1").arg(pqLiveInsituManager::instance()->time());
  this->Ui->Time->setText(timeString);

  QString timeStepString = QString("%1").arg(pqLiveInsituManager::instance()->timeStep());
  this->Ui->TimeStep->setText(timeStepString);
}
