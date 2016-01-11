#include "pqWelcomeDialog.h"
#include "ui_pqWelcomeDialog.h"

//-----------------------------------------------------------------------------
pqWelcomeDialog::pqWelcomeDialog(QWidget *parent)
  : Superclass (parent),
    ui(new Ui::pqWelcomeDialog)
{
    ui->setupUi(this);
}

//-----------------------------------------------------------------------------
pqWelcomeDialog::~pqWelcomeDialog()
{
    delete ui;
}
