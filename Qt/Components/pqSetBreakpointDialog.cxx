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
  message.setText("Breakpoint time is invalid or "
                  "it is smaller than current time");
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
