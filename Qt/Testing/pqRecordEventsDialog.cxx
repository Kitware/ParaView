/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqEventObserverXML.h"
#include "pqEventTranslator.h"
#include "pqRecordEventsDialog.h"
#include "ui_pqRecordEventsDialog.h"

#include <QPushButton>
#include <QTimer>

#include <vtkIOStream.h>

//////////////////////////////////////////////////////////////////////////////////
// pqImplementation

struct pqRecordEventsDialog::pqImplementation
{
public:
  pqImplementation(const QString& Path) :
    File(Path.toAscii().data()),
    Observer(File)
  {
  }

  Ui::pqRecordEventsDialog Ui;

  ofstream File;
  pqEventTranslator Translator;
  pqEventObserverXML Observer;
};

///////////////////////////////////////////////////////////////////////////////////
// pqRecordEventsDialog

pqRecordEventsDialog::pqRecordEventsDialog(const QString& Path, QWidget* Parent) :
  QDialog(Parent),
  Implementation(new pqImplementation(Path))
{
  this->Implementation->Ui.setupUi(this);
  this->Implementation->Ui.label->setText(QString(tr("Recording User Input to %1")).arg(Path));

  this->Implementation->Translator.ignoreObject(this->Implementation->Ui.stopButton);
  this->Implementation->Translator.addDefaultWidgetEventTranslators();
  
  QObject::connect(
    &this->Implementation->Translator,
    SIGNAL(recordEvent(const QString&, const QString&, const QString&)),
    &this->Implementation->Observer,
    SLOT(onRecordEvent(const QString&, const QString&, const QString&)));
  
  this->setWindowTitle(tr("Recording User Input"));
  this->setObjectName("");
}

pqRecordEventsDialog::~pqRecordEventsDialog()
{
  delete Implementation;
}

void pqRecordEventsDialog::accept()
{
  QDialog::accept();
  QTimer::singleShot(0, this, SLOT(onAutoDelete()));
}

void pqRecordEventsDialog::reject()
{
  QDialog::reject();
  QTimer::singleShot(0, this, SLOT(onAutoDelete()));
}

void pqRecordEventsDialog::onAutoDelete()
{
  delete this;
}
