/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqPythonDialog.h"
#include "ui_pqPython.h"

//////////////////////////////////////////////////////////////////////
// pqPythonDialog::pqImplementation

struct pqPythonDialog::pqImplementation
{
  Ui::pqPythonDialog Ui;
};

pqPythonDialog::pqPythonDialog(QWidget* Parent) :
  QDialog(Parent),
  Implementation(new pqImplementation())
{
  this->Implementation->Ui.setupUi(this);
  this->setObjectName("pythonDialog");
  this->setWindowTitle(tr("Python Shell"));
}

pqPythonDialog::~pqPythonDialog()
{
  delete Implementation;
}

void pqPythonDialog::accept()
{
  QDialog::accept();
  delete this;
}

void pqPythonDialog::reject()
{
  QDialog::reject();
  delete this;
}

