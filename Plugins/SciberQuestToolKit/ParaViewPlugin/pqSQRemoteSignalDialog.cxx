/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "pqSQRemoteSignalDialog.h"

#include "ui_pqSQRemoteSignalDialogForm.h"

#include <QDebug>
#include <QDebug>
#include <QFont>
#include <QMessageBox>
#include <QPalette>
#include <QPlastiqueStyle>
#include <QProgressBar>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>

#include "FsUtils.h"
#include "PrintUtils.h"

#include <iostream>

#include "pqFileDialog.h"

#define pqErrorMacro(estr)                                                                         \
  qDebug() << "Error in:" << std::endl                                                             \
           << __FILE__ << ", line " << __LINE__ << std::endl                                       \
           << "" estr << std::endl;

// User interface
//=============================================================================
class pqSQRemoteSignalDialogUI : public Ui::pqSQRemoteSignalDialogForm
{
};

//------------------------------------------------------------------------------
pqSQRemoteSignalDialog::pqSQRemoteSignalDialog(QWidget* Parent, Qt::WindowFlags flags)
  : QDialog(Parent, flags)
  , Modified(0)
  , Ui(0)
{
  this->Ui = new pqSQRemoteSignalDialogUI;
  this->Ui->setupUi(this);

  // plumbing to increment mtime as state changes
  QObject::connect(
    this->Ui->fpeTrapUnderflow, SIGNAL(stateChanged(int)), this, SLOT(SetModified()));

  QObject::connect(this->Ui->fpeTrapOverflow, SIGNAL(stateChanged(int)), this, SLOT(SetModified()));

  QObject::connect(
    this->Ui->fpeTrapDivByZero, SIGNAL(stateChanged(int)), this, SLOT(SetModified()));

  QObject::connect(this->Ui->fpeTrapInvalid, SIGNAL(stateChanged(int)), this, SLOT(SetModified()));

  QObject::connect(this->Ui->fpeTrapInexact, SIGNAL(stateChanged(int)), this, SLOT(SetModified()));
}

//------------------------------------------------------------------------------
pqSQRemoteSignalDialog::~pqSQRemoteSignalDialog()
{
  delete this->Ui;
}

//------------------------------------------------------------------------------
void pqSQRemoteSignalDialog::SetTrapFPEDivByZero(int enable)
{
  this->Ui->fpeTrapDivByZero->setChecked(enable);
}

//------------------------------------------------------------------------------
int pqSQRemoteSignalDialog::GetTrapFPEDivByZero()
{
  return this->Ui->fpeTrapDivByZero->isChecked();
}

//------------------------------------------------------------------------------
void pqSQRemoteSignalDialog::SetTrapFPEInexact(int enable)
{
  this->Ui->fpeTrapInexact->setChecked(enable);
}

//------------------------------------------------------------------------------
int pqSQRemoteSignalDialog::GetTrapFPEInexact()
{
  return this->Ui->fpeTrapInexact->isChecked();
}

//------------------------------------------------------------------------------
void pqSQRemoteSignalDialog::SetTrapFPEInvalid(int enable)
{
  this->Ui->fpeTrapInvalid->setChecked(enable);
}

//------------------------------------------------------------------------------
int pqSQRemoteSignalDialog::GetTrapFPEInvalid()
{
  return this->Ui->fpeTrapInvalid->isChecked();
}

//------------------------------------------------------------------------------
void pqSQRemoteSignalDialog::SetTrapFPEOverflow(int enable)
{
  this->Ui->fpeTrapOverflow->setChecked(enable);
}

//------------------------------------------------------------------------------
int pqSQRemoteSignalDialog::GetTrapFPEOverflow()
{
  return this->Ui->fpeTrapOverflow->isChecked();
}

//------------------------------------------------------------------------------
void pqSQRemoteSignalDialog::SetTrapFPEUnderflow(int enable)
{
  this->Ui->fpeTrapUnderflow->setChecked(enable);
}

//------------------------------------------------------------------------------
int pqSQRemoteSignalDialog::GetTrapFPEUnderflow()
{
  return this->Ui->fpeTrapUnderflow->isChecked();
}
