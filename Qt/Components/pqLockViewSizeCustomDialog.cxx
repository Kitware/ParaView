// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
  this->setWindowFlags(this->windowFlags().setFlag(Qt::WindowContextHelpButtonHint, false));

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
