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
#include "vtkSMSourceProxy.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkSMStringVectorProperty.h"

// ParaView includes
#include "pqPipelineFilter.h"
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

  QObject::connect(this->Internal->UI.AttributeMode,
                   SIGNAL(currentIndexChanged(const QString&)),
                   this,
                   SLOT(updateVariables(const QString&)));
  
  QObject::connect(this->Internal->UI.AttributeMode,
                   SIGNAL(currentIndexChanged(const QString&)),
                   this->Internal->UI.Function,
                   SLOT(clear()));
  
  QObject::connect(this->Internal->UI.Vectors,
                   SIGNAL(currentIndexChanged(int)),
                   this,
                   SLOT(vectorChosen(int)));
  
  QObject::connect(this->Internal->UI.Scalars,
                   SIGNAL(currentIndexChanged(int)),
                   this,
                   SLOT(scalarChosen(int)));

  // clicking on any button or any part of the panel where another button
  // doesn't take focus will cause the line edit to have focus 
  this->setFocusProxy(this->Internal->UI.Function);
  
  // connect all buttons for which the text of the button 
  // is the same as what goes into the function
  QRegExp regexp("^([ijk]Hat|n[0-9]|ln|log10|sin|cos|"
                 "tan|asin|acos|atan|sinh|cosh|tanh|"
                 "sqrt|exp|ceil|floor|abs|norm|mag|"
                 "LeftParentheses|RightParentheses|"
                 "Divide|Multiply|Minus|Plus|Dot)$");
  QList<QToolButton*> buttons;
  buttons = this->findChildren<QToolButton*>(regexp);
  foreach(QToolButton* tb, buttons)
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
  
  QToolButton* tb = this->Internal->UI.xy;
  QSignalMapper* mapper = new QSignalMapper(tb);
  QObject::connect(tb,
                   SIGNAL(pressed()),
                   mapper,
                   SLOT(map()));
  mapper->setMapping(tb, "^");
  QObject::connect(mapper,
                   SIGNAL(mapped(const QString&)),
                   this,
                   SLOT(buttonPressed(const QString&)));
  
  tb = this->Internal->UI.v1v2;
  mapper = new QSignalMapper(tb);
  QObject::connect(tb,
                   SIGNAL(pressed()),
                   mapper,
                   SLOT(map()));
  mapper->setMapping(tb, ".");
  QObject::connect(mapper,
                   SIGNAL(mapped(const QString&)),
                   this,
                   SLOT(buttonPressed(const QString&)));

  
  QObject::connect(this->Internal->UI.Clear,
                   SIGNAL(pressed()),
                   this->Internal->UI.Function,
                   SLOT(clear()));
 


  // mark panel modified if the following are changed 
  QObject::connect(this->Internal->UI.Function,
                   SIGNAL(textEdited(const QString&)),
                   this,
                   SLOT(modified()));
  QObject::connect(this->Internal->UI.ResultArrayName,
                   SIGNAL(textEdited(const QString&)),
                   this,
                   SLOT(modified()));
  QObject::connect(this->Internal->UI.AttributeMode,
                   SIGNAL(currentIndexChanged(const QString&)),
                   this,
                   SLOT(modified()));
  QObject::connect(this->Internal->UI.ReplaceInvalidResult,
                   SIGNAL(stateChanged(int)),
                   this,
                   SLOT(modified()));
  QObject::connect(this->Internal->UI.ReplacementValue,
                   SIGNAL(textChanged(const QString&)),
                   this,
                   SLOT(modified()));


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
  this->updateVariables(this->Internal->UI.AttributeMode->currentText());
  this->reset();
}

//-----------------------------------------------------------------------------
/// accept the changes made to the properties
/// changes will be propogated down to the server manager
void pqCalculatorPanel::accept()
{
  this->pqObjectPanel::accept();

  if(!this->proxy() || !this->proxy()->getProxy())
    {
    return;
    }

  vtkSMProxy* CalcProxy = this->proxy()->getProxy();

  // put in new variables
  QComboBox* scalarCombo = this->Internal->UI.Scalars;
  vtkSMStringVectorProperty* ScalarProperty;
  ScalarProperty = vtkSMStringVectorProperty::SafeDownCast(
       CalcProxy->GetProperty("AddScalarVariable"));
  if(ScalarProperty)
    {
    int numVariables = scalarCombo->count()-1;
    for(int i=0; i<numVariables; i++)
      {
      QString VarName = scalarCombo->itemText(i+1);
      QString ArrayName = VarName;
      QString Component = QString("%1").arg(0);
      QVariant d = scalarCombo->itemData(i+1, Qt::UserRole);
      if(d.isValid())
        {
        QStringList v = d.toStringList();
        if(v.size() == 2)
          {
          ArrayName = v[0];
          Component = v[1];
          }
        }
      pqSMAdaptor::setMultipleElementProperty(ScalarProperty, 3*i, VarName);
      pqSMAdaptor::setMultipleElementProperty(ScalarProperty, 3*i+1, ArrayName);
      pqSMAdaptor::setMultipleElementProperty(ScalarProperty, 3*i+2, Component);
      }
    ScalarProperty->SetNumberOfElements(numVariables*3);
    }

  QComboBox* vectorCombo = this->Internal->UI.Vectors;
  vtkSMStringVectorProperty* VectorProperty;
  VectorProperty = vtkSMStringVectorProperty::SafeDownCast(
    CalcProxy->GetProperty("AddVectorVariable"));
  if(VectorProperty)
    {
    int numVariables = vectorCombo->count()-1;
    for(int i=0; i<numVariables; i++)
      {
      QString VarName = vectorCombo->itemText(i+1);
      pqSMAdaptor::setMultipleElementProperty(VectorProperty, 5*i, VarName);
      pqSMAdaptor::setMultipleElementProperty(VectorProperty, 5*i+1, VarName);
      pqSMAdaptor::setMultipleElementProperty(VectorProperty, 5*i+2, "0");
      pqSMAdaptor::setMultipleElementProperty(VectorProperty, 5*i+3, "1");
      pqSMAdaptor::setMultipleElementProperty(VectorProperty, 5*i+4, "2");
      }
    VectorProperty->SetNumberOfElements(numVariables*5);
    }
  
  pqSMAdaptor::setElementProperty(
                 CalcProxy->GetProperty("AttributeMode"),
                 this->Internal->UI.AttributeMode->currentText() == 
                 "Point Data" ? 1 : 2);

  pqSMAdaptor::setElementProperty(
                 CalcProxy->GetProperty("ResultArrayName"),
                 this->Internal->UI.ResultArrayName->text());
  
  pqSMAdaptor::setEnumerationProperty(
          CalcProxy->GetProperty("ReplaceInvalidValues"),
          this->Internal->UI.ReplaceInvalidResult->isChecked());
  
  pqSMAdaptor::setElementProperty(
          CalcProxy->GetProperty("ReplacementValue"),
          this->Internal->UI.ResultArrayName->text());

  pqSMAdaptor::setElementProperty(
                 CalcProxy->GetProperty("Function"),
                 this->Internal->UI.Function->text());

  CalcProxy->UpdateVTKObjects();

}

//-----------------------------------------------------------------------------
/// reset the changes made
/// editor will query properties from the server manager
void pqCalculatorPanel::reset()
{
  this->pqObjectPanel::reset();
  
  vtkSMProxy* CalcProxy = this->proxy()->getProxy();


  // restore the function
  QVariant v = pqSMAdaptor::getElementProperty(
                 CalcProxy->GetProperty("Function"));
 
  this->Internal->UI.Function->setText(v.toString());

  // restore the attribute mode
  v = pqSMAdaptor::getElementProperty(CalcProxy->GetProperty("AttributeMode"));
  this->Internal->UI.AttributeMode->setCurrentIndex(v.toInt() == 2 ? 1 : 0);
  
  // restore the results array name
  v = pqSMAdaptor::getElementProperty(
          CalcProxy->GetProperty("ResultArrayName"));
  this->Internal->UI.ResultArrayName->setText(v.toString());
  
  // restore the replace invalid results
  v = pqSMAdaptor::getEnumerationProperty(
          CalcProxy->GetProperty("ReplaceInvalidValues"));
  this->Internal->UI.ReplaceInvalidResult->setChecked(v.toBool());
  
  // restore the replacement value
  v = pqSMAdaptor::getElementProperty(
          CalcProxy->GetProperty("ReplacementValue"));
  this->Internal->UI.ReplacementValue->setText(v.toString());

}

void pqCalculatorPanel::buttonPressed(const QString& buttonText)
{
  this->Internal->UI.Function->insert(buttonText);
}

void pqCalculatorPanel::updateVariables(const QString& mode)
{
  this->Internal->UI.Vectors->clear();
  this->Internal->UI.Scalars->clear();

  this->Internal->UI.Vectors->addItem("Insert Vector...");
  this->Internal->UI.Scalars->addItem("Insert Scalar...");

  vtkPVDataSetAttributesInformation* fdi = NULL;
  pqPipelineFilter* f = qobject_cast<pqPipelineFilter*>(this->proxy());
  if(!f)
    {
    return;
    }

  if(mode == "Point Data")
    {
    fdi = vtkSMSourceProxy::SafeDownCast(f->getInput(0)->getProxy())->
      GetDataInformation()->GetPointDataInformation();
    }
  else if(mode == "Cell Data")
    {
    fdi = vtkSMSourceProxy::SafeDownCast(f->getInput(0)->getProxy())->
      GetDataInformation()->GetCellDataInformation();
    }
  
  if(!fdi)
    {
    return;
    }

  for(int i=0; i<fdi->GetNumberOfArrays(); i++)
    {
    vtkPVArrayInformation* arrayInfo = fdi->GetArrayInformation(i);
    if(arrayInfo->GetIsPartial())
      {
      continue;
      }

    int numComponents = arrayInfo->GetNumberOfComponents();
    QString name = arrayInfo->GetName();

    for(int j=0; j<numComponents; j++)
      {
      if(numComponents == 1)
        {
        this->Internal->UI.Scalars->addItem(name);
        }
      else
        {
        QString n = name + QString("_%1").arg(j);
        QStringList d;
        d.append(name);
        d.append(QString("%1").arg(j));
        int where = this->Internal->UI.Scalars->count();
        this->Internal->UI.Scalars->insertItem(where, n, d);
        }
      }

    if(numComponents == 3)
      {
      this->Internal->UI.Vectors->addItem(name);
      }
    }
}

void pqCalculatorPanel::scalarChosen(int i)
{
  if(i > 0)
    {
    QString text = this->Internal->UI.Scalars->itemText(i);
    this->Internal->UI.Function->insert(text);

    this->Internal->UI.Scalars->setCurrentIndex(0);
    }
}

void pqCalculatorPanel::vectorChosen(int i)
{
  if(i > 0)
    {
    QString text = this->Internal->UI.Vectors->itemText(i);
    this->Internal->UI.Function->insert(text);
    this->Internal->UI.Vectors->setCurrentIndex(0);
    }
}

void pqCalculatorPanel::modified()
{
  emit this->canAcceptOrReject(true);
}

