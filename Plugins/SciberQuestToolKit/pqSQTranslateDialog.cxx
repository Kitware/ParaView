#include "pqSQTranslateDialog.h"

#include "ui_pqSQTranslateDialogForm.h"

#include <QLineEdit>
#include <QString>
#include <QDoubleValidator>
#include <QRadioButton>

// User interface
//=============================================================================
class pqSQTranslateDialogUI
    :
  public Ui::pqSQTranslateDialogForm
    {};

//------------------------------------------------------------------------------
pqSQTranslateDialog::pqSQTranslateDialog(
    QWidget *Parent,
    Qt::WindowFlags flags)
            :
    QDialog(Parent,flags),
    Ui(0)
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
void pqSQTranslateDialog::GetTranslation(double *t)
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
