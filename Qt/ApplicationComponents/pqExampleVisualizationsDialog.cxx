#include "pqExampleVisualizationsDialog.h"
#include "ui_pqExampleVisualizationsDialog.h"

#include "pqDebug.h"
#include "pqLoadStateReaction.h"

//-----------------------------------------------------------------------------
pqExampleVisualizationsDialog::pqExampleVisualizationsDialog(QWidget* parent)
  : Superclass(parent),
    ui(new Ui::pqExampleVisualizationsDialog)
{
  ui->setupUi(this);

  QObject::connect(this->ui->CanExampleButton, SIGNAL(clicked(bool)),
                   this, SLOT(onButtonPressed()));
  QObject::connect(this->ui->DiskOutRefExampleButton, SIGNAL(clicked(bool)),
                   this, SLOT(onButtonPressed()));
}

//-----------------------------------------------------------------------------
pqExampleVisualizationsDialog::~pqExampleVisualizationsDialog()
{
  delete ui;
}

//-----------------------------------------------------------------------------
void pqExampleVisualizationsDialog::onButtonPressed()
{

#if defined (_WIN32)
  QString stateFileRoot = QCoreApplication::applicationDirPath() + "/../doc/";
#elif defined(__APPLE__)
  QString stateFileRoot = QCoreApplication::applicationDirPath() + "/../../../doc/";
#else
  QString stateFileRoot = QCoreApplication::applicationDirPath() + "/../../doc/";
#endif

  QPushButton* button = qobject_cast<QPushButton*>(sender());
  if (button)
    {
    if (button == this->ui->CanExampleButton)
      {
      QString stateFile = stateFileRoot + "CanExample.pvsm";
      pqLoadStateReaction::loadState(stateFile);
      this->hide();
      }
    else if (button == this->ui->DiskOutRefExampleButton)
      {
      QString stateFile = stateFileRoot + "DiskOutRefExample.pvsm";
      pqLoadStateReaction::loadState(stateFile);
      this->hide();
      }
    else
      {
      qCritical("No example file for button");
      }
    } 
}
