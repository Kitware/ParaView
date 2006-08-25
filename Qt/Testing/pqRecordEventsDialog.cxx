/*=========================================================================

   Program: ParaView
   Module:    pqRecordEventsDialog.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

=========================================================================*/

#include "pqEventTranslator.h"
#include "pqRecordEventsDialog.h"
#include "pqXMLEventObserver.h"

#include "ui_pqRecordEventsDialog.h"

#include <QPushButton>
#include <QTimer>

#include <vtkIOStream.h>

//////////////////////////////////////////////////////////////////////////////////
// pqImplementation

struct pqRecordEventsDialog::pqImplementation
{
public:
  pqImplementation(pqEventTranslator* translator, const QString& path) :
    Translator(translator),
    File(path.toAscii().data()),
    Observer(File)
  {
  }
  
  ~pqImplementation()
  {
    delete this->Translator;
  }

  Ui::pqRecordEventsDialog Ui;

  pqEventTranslator* const Translator;
  ofstream File;
  pqXMLEventObserver Observer;
};

///////////////////////////////////////////////////////////////////////////////////
// pqRecordEventsDialog

pqRecordEventsDialog::pqRecordEventsDialog(pqEventTranslator* Translator, const QString& Path, QWidget* Parent) :
  QDialog(Parent),
  Implementation(new pqImplementation(Translator, Path))
{
  this->Implementation->Ui.setupUi(this);
  this->Implementation->Ui.label->setText(QString(tr("Recording User Input to %1")).arg(Path));

  this->Implementation->Translator->ignoreObject(this->Implementation->Ui.stopButton);
  
  QObject::connect(
    this->Implementation->Translator,
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
