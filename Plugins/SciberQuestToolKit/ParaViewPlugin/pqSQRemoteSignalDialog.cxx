#include "pqSQRemoteSignalDialog.h"

#include "ui_pqSQRemoteSignalDialogForm.h"

#include <QDebug>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>
#include <QString>
#include <QStringList>
#include <QSettings>
#include <QMessageBox>
#include <QProgressBar>
#include <QPalette>
#include <QFont>
#include <QPlastiqueStyle>
#include <QDebug>

#include "PrintUtils.h"
#include "FsUtils.h"

#include <iostream>

#include "pqFileDialog.h"

#define pqErrorMacro(estr)\
  qDebug()\
      << "Error in:" << std::endl\
      << __FILE__ << ", line " << __LINE__ << std::endl\
      << "" estr << std::endl;


// User interface
//=============================================================================
class pqSQRemoteSignalDialogUI
    :
  public Ui::pqSQRemoteSignalDialogForm
    {};

//------------------------------------------------------------------------------
pqSQRemoteSignalDialog::pqSQRemoteSignalDialog(
    QWidget *Parent,
    Qt::WindowFlags flags)
            :
    QDialog(Parent,flags),
    Modified(0),
    Ui(0)
{
  this->Ui = new pqSQRemoteSignalDialogUI;
  this->Ui->setupUi(this);

  // plumbing to increment mtime as state changes
  QObject::connect(
    this->Ui->fpeTrapUnderflow, SIGNAL(stateChanged(int)),
    this, SLOT(SetModified()));

  QObject::connect(
    this->Ui->fpeTrapOverflow, SIGNAL(stateChanged(int)),
    this, SLOT(SetModified()));

  QObject::connect(
    this->Ui->fpeTrapDivByZero, SIGNAL(stateChanged(int)),
    this, SLOT(SetModified()));

  QObject::connect(
    this->Ui->fpeTrapInvalid, SIGNAL(stateChanged(int)),
    this, SLOT(SetModified()));

  QObject::connect(
    this->Ui->fpeTrapInexact, SIGNAL(stateChanged(int)),
    this, SLOT(SetModified()));
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
