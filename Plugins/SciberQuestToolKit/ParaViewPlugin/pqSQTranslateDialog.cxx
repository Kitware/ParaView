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
#include "pqSQTranslateDialog.h"

#include "ui_pqSQTranslateDialogForm.h"

#include <QDoubleValidator>
#include <QLineEdit>
#include <QRadioButton>
#include <QString>

// User interface
//=============================================================================
class pqSQTranslateDialogUI : public Ui::pqSQTranslateDialogForm
{
};

//------------------------------------------------------------------------------
pqSQTranslateDialog::pqSQTranslateDialog(QWidget* Parent, Qt::WindowFlags flags)
  : QDialog(Parent, flags)
  , Ui(0)
{
  this->Ui = new pqSQTranslateDialogUI;
  this->Ui->setupUi(this);

  this->Ui->tx->setValidator(new QDoubleValidator(this->Ui->tx));
  this->Ui->ty->setValidator(new QDoubleValidator(this->Ui->ty));
  this->Ui->tz->setValidator(new QDoubleValidator(this->Ui->tz));
}

//------------------------------------------------------------------------------
pqSQTranslateDialog::~pqSQTranslateDialog()
{
  delete this->Ui;
}

//------------------------------------------------------------------------------
void pqSQTranslateDialog::GetTranslation(double* t)
{
  t[0] = this->GetTranslateX();
  t[1] = this->GetTranslateY();
  t[2] = this->GetTranslateZ();
}

//------------------------------------------------------------------------------
double pqSQTranslateDialog::GetTranslateX()
{
  return this->Ui->tx->text().toDouble();
}

//------------------------------------------------------------------------------
double pqSQTranslateDialog::GetTranslateY()
{
  return this->Ui->ty->text().toDouble();
}

//------------------------------------------------------------------------------
double pqSQTranslateDialog::GetTranslateZ()
{
  return this->Ui->tz->text().toDouble();
}

//------------------------------------------------------------------------------
bool pqSQTranslateDialog::GetTypeIsNewOrigin()
{
  return this->Ui->typeNewOrigin->isChecked();
}

//------------------------------------------------------------------------------
bool pqSQTranslateDialog::GetTypeIsOffset()
{
  return this->Ui->typeOffset->isChecked();
}
