#include "pqRemoteCommandTemplateDialog.h"

#include "ui_pqRemoteCommandTemplateDialogForm.h"

#include <QDebug>
#include <QString>

#include <iostream>
using std::cerr;
using std::endl;

#include <sstream>
#include <string>

#define pqErrorMacro(estr)                                                                         \
  qDebug() << "Error in:" << endl << __FILE__ << ", line " << __LINE__ << endl << "" estr << endl;

// User interface
//=============================================================================
class pqRemoteCommandTemplateDialogUI : public Ui::pqRemoteCommandTemplateDialogForm
{
};

//------------------------------------------------------------------------------
pqRemoteCommandTemplateDialog::pqRemoteCommandTemplateDialog(QWidget* Parent, Qt::WindowFlags flags)
  : QDialog(Parent, flags)
  , Modified(0)
  , Ui(nullptr)
{
  this->Ui = new pqRemoteCommandTemplateDialogUI;
  this->Ui->setupUi(this);

  QObject::connect(this->Ui->commandName, SIGNAL(textChanged(QString)), this, SLOT(SetModified()));

  QObject::connect(this->Ui->commandTemplate, SIGNAL(textChanged()), this, SLOT(SetModified()));
}

//------------------------------------------------------------------------------
pqRemoteCommandTemplateDialog::~pqRemoteCommandTemplateDialog()
{
  delete this->Ui;
}

//------------------------------------------------------------------------------
void pqRemoteCommandTemplateDialog::SetCommandName(QString name)
{
  this->Ui->commandName->setText(name);
}

//------------------------------------------------------------------------------
QString pqRemoteCommandTemplateDialog::GetCommandName()
{
  return this->Ui->commandName->text();
}

//------------------------------------------------------------------------------
void pqRemoteCommandTemplateDialog::SetCommandTemplate(QString templ)
{
  this->Ui->commandTemplate->setPlainText(templ);
}

//------------------------------------------------------------------------------
QString pqRemoteCommandTemplateDialog::GetCommandTemplate()
{
  return this->Ui->commandTemplate->toPlainText();
}
