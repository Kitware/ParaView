/*=========================================================================

   Program: ParaView
   Module:    pqRecordEventsDialog.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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
#include "pqEventObserver.h"

#include "ui_pqRecordEventsDialog.h"

#include <QPushButton>
#include <QTimer>
#include <QFile>
#include <QTextStream>

//////////////////////////////////////////////////////////////////////////////////
// pqImplementation

struct pqRecordEventsDialog::pqImplementation
{
public:
  pqImplementation(pqEventTranslator& translator,
                   pqEventObserver& observer) 
    : Translator(translator), Observer(observer)
  {
  }
  
  ~pqImplementation()
  {
  }

  Ui::pqRecordEventsDialog Ui;

  pqEventTranslator& Translator;
  pqEventObserver& Observer;
  QFile File;
  QTextStream Stream;
};

///////////////////////////////////////////////////////////////////////////////////
// pqRecordEventsDialog

pqRecordEventsDialog::pqRecordEventsDialog(pqEventTranslator& Translator, 
                                           pqEventObserver& observer,
                                           const QString& Path,
                                           QWidget* Parent) 
  : QDialog(Parent),
    Implementation(new pqImplementation(Translator, observer))
{
  this->Implementation->Ui.setupUi(this);
  this->Implementation->Ui.label->setText(QString(tr("Recording User Input to %1")).arg(Path));

  this->Implementation->Translator.ignoreObject(this->Implementation->Ui.stopButton);
  this->Implementation->Translator.ignoreObject(this);
  
  this->setWindowTitle(tr("Recording User Input"));
  this->setObjectName("");

  QObject::connect(
    &this->Implementation->Translator,
    SIGNAL(recordEvent(const QString&, const QString&, const QString&)),
    &this->Implementation->Observer,
    SLOT(onRecordEvent(const QString&, const QString&, const QString&)));

  this->Implementation->File.setFileName(Path);
  this->Implementation->File.open(QIODevice::WriteOnly);
  this->Implementation->Stream.setDevice(&this->Implementation->File);
  this->Implementation->Observer.setStream(&this->Implementation->Stream);
  
  this->Implementation->Translator.start();
}

pqRecordEventsDialog::~pqRecordEventsDialog()
{
  this->Implementation->Translator.stop();
  
  QObject::disconnect(
    &this->Implementation->Translator,
    SIGNAL(recordEvent(const QString&, const QString&, const QString&)),
    &this->Implementation->Observer,
    SLOT(onRecordEvent(const QString&, const QString&, const QString&)));
  
  this->Implementation->Observer.setStream(NULL);
  this->Implementation->Stream.flush();
  this->Implementation->File.close();

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
