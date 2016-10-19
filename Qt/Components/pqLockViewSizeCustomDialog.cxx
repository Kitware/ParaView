/*=========================================================================

   Program: ParaView
   Module:    pqLockViewSizeCustomDialog.cxx

   Copyright (c) 2005-2010 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqLockViewSizeCustomDialog.h"
#include "ui_pqLockViewSizeCustomDialog.h"

#include <QIntValidator>
#include <QPushButton>

#include "pqApplicationCore.h"
#include "pqSettings.h"
#include "pqTabbedMultiViewWidget.h"

//=============================================================================
class pqLockViewSizeCustomDialog::pqUI : public Ui::pqLockViewSizeCustomDialog
{
public:
  typedef Ui::pqLockViewSizeCustomDialog Superclass;
  QPushButton* Unlock;

  void setupUi(QDialog* parent)
  {
    this->Superclass::setupUi(parent);

    this->Unlock = new QPushButton(tr("Unlock"), parent);
    this->Unlock->setObjectName("Unlock");
    this->ButtonBox->addButton(this->Unlock, QDialogButtonBox::DestructiveRole);
  }
};

//-----------------------------------------------------------------------------
pqLockViewSizeCustomDialog::pqLockViewSizeCustomDialog(QWidget* _parent, Qt::WindowFlags f)
  : Superclass(_parent, f)
{
  this->ui = new pqUI();
  this->ui->setupUi(this);

  QIntValidator* validator = new QIntValidator(this);
  validator->setBottom(50);
  this->ui->Width->setValidator(validator);

  validator = new QIntValidator(this);
  validator->setBottom(50);
  this->ui->Height->setValidator(validator);

  QObject::connect(this->ui->ButtonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked(bool)),
    this, SLOT(apply()));
  QObject::connect(this->ui->Unlock, SIGNAL(clicked(bool)), this, SLOT(unlock()));

  pqSettings* settings = pqApplicationCore::instance()->settings();
  QSize resolution = settings->value("LockViewSize/CustomResolution", QSize(300, 300)).toSize();
  this->ui->Width->setText(QString::number(resolution.width()));
  this->ui->Height->setText(QString::number(resolution.height()));
}

pqLockViewSizeCustomDialog::~pqLockViewSizeCustomDialog()
{
  delete this->ui;
}

//-----------------------------------------------------------------------------
inline QSize pqLockViewSizeCustomDialog::customResolution() const
{
  return QSize(this->ui->Width->text().toInt(), this->ui->Height->text().toInt());
}

//-----------------------------------------------------------------------------
void pqLockViewSizeCustomDialog::apply()
{
  pqTabbedMultiViewWidget* viewManager = qobject_cast<pqTabbedMultiViewWidget*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_WIDGET"));
  if (viewManager)
  {
    viewManager->lockViewSize(this->customResolution());
  }
  else
  {
    qCritical("pqLockViewSizeCustomDialog requires pqTabbedMultiViewWidget.");
  }
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->setValue("LockViewSize/CustomResolution", this->customResolution());
}

//-----------------------------------------------------------------------------
void pqLockViewSizeCustomDialog::accept()
{
  this->apply();
  this->Superclass::accept();
}

//-----------------------------------------------------------------------------
void pqLockViewSizeCustomDialog::unlock()
{
  pqTabbedMultiViewWidget* viewManager = qobject_cast<pqTabbedMultiViewWidget*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_WIDGET"));
  if (viewManager)
  {
    viewManager->lockViewSize(QSize(-1, -1));
  }
  else
  {
    qCritical("pqLockViewSizeCustomDialog requires pqTabbedMultiViewWidget.");
  }
  this->reject();
}
