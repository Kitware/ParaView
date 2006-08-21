/*=========================================================================

   Program: ParaView
   Module:    pqCalculatorPanel.cxx

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

// this include
#include "pqCalculatorPanel.h"

// Qt includes
#include <QSignalMapper>

// VTK includes

// ParaView Server Manager includes
#include "vtkSMProperty.h"

// ParaView includes
#include "pqProxy.h"
#include "pqSMAdaptor.h"
#include "ui_pqCalculatorPanel.h"


// we include this for static plugins
#define QT_STATICPLUGIN
#include <QtPlugin>

QString pqCalculatorPanelInterface::name() const
{
  return "Calculator";
}

pqObjectPanel* pqCalculatorPanelInterface::createPanel(QWidget* p)
{
  return new pqCalculatorPanel(p);
}

Q_EXPORT_PLUGIN(pqCalculatorPanelInterface)
Q_IMPORT_PLUGIN(pqCalculatorPanelInterface)

class pqCalculatorPanel::pqInternal
{
public:
  Ui::CalculatorPanel UI;
};

//-----------------------------------------------------------------------------
/// constructor
pqCalculatorPanel::pqCalculatorPanel(QWidget* p)
  : pqObjectPanel(p)
{
  this->Internal = new pqInternal();
  this->Internal->UI.setupUi(this);

  // connect all the number buttons
  foreach(QToolButton* tb, this->findChildren<QToolButton*>(QRegExp("^n[0-9]$")))
    {
    QSignalMapper* mapper = new QSignalMapper(tb);
    QObject::connect(tb,
                     SIGNAL(pressed()),
                     mapper,
                     SLOT(map()));
    mapper->setMapping(tb, tb->text());
    QObject::connect(mapper,
                     SIGNAL(mapped(const QString&)),
                     this,
                     SLOT(buttonPressed(const QString&)));
    }
}

//-----------------------------------------------------------------------------
/// destructor
pqCalculatorPanel::~pqCalculatorPanel()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqCalculatorPanel::setProxyInternal(pqProxy* p)
{
  this->pqObjectPanel::setProxyInternal(p);
}

//-----------------------------------------------------------------------------
/// accept the changes made to the properties
/// changes will be propogated down to the server manager
void pqCalculatorPanel::accept()
{
  this->pqObjectPanel::accept();

  if(!this->proxy())
    {
    return;
    }

  vtkSMProperty* removeAllVariables =
    this->proxy()->getProxy()->GetProperty("RemoveAllVariables");
  if(removeAllVariables)
    {
    removeAllVariables->Modified();
    }

  pqSMAdaptor::setElementProperty(
                 this->proxy()->getProxy()->GetProperty("AttributeMode"),
                 this->Internal->UI.AttributeMode->currentText() == "Point Data"
                 ? 1 : 2);

  pqSMAdaptor::setElementProperty(
                 this->proxy()->getProxy()->GetProperty("ResultArrayName"),
                 this->Internal->UI.ResultArrayName->text());

  pqSMAdaptor::setElementProperty(
                 this->proxy()->getProxy()->GetProperty("Function"),
                 this->Internal->UI.Function->text());

  this->proxy()->getProxy()->UpdateVTKObjects();

}

//-----------------------------------------------------------------------------
/// reset the changes made
/// editor will query properties from the server manager
void pqCalculatorPanel::reset()
{
  this->pqObjectPanel::reset();
}

void pqCalculatorPanel::buttonPressed(const QString& buttonText)
{
  this->Internal->UI.Function->insert(buttonText);
}

