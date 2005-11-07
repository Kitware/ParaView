/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqRecordEventsDialog.h"

#include "pqEventObserverXML.h"
#include "pqEventTranslator.h"

#include <QPushButton>
#include <QTimer>

#include <vtkIOStream.h>

class pqRecordEventsDialog::pqImplementation
{
public:
  pqImplementation(const QString& Path) :
    file(Path.toAscii().data()),
    observer(file)
  {
    translator.addDefaultWidgetEventTranslators();
    
    QObject::connect(
      &translator,
      SIGNAL(recordEvent(const QString&, const QString&, const QString&)),
      &observer,
      SLOT(onRecordEvent(const QString&, const QString&, const QString&)));
  }

private:
  ofstream file;
  pqEventTranslator translator;
  pqEventObserverXML observer;
};

pqRecordEventsDialog::pqRecordEventsDialog(const QString& Path, QWidget* Parent) :
  QDialog(Parent),
  Implementation(new pqImplementation(Path))
{
  this->Ui.setupUi(this);
  this->Ui.label->setText(QString(tr("Recording User Input to %1")).arg(Path));
  
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
