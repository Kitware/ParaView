/*=========================================================================

   Program: ParaView
   Module:    pqCalculatorPanel.cxx

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

// this include
#include "pqCalculatorPanel.h"

// Qt includes
#include <QSignalMapper>
#include <QMenu>

// VTK includes

// ParaView Server Manager includes
#include "vtkSMProperty.h"

#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkSMStringVectorProperty.h"

// ParaView includes
#include "pqDataRepresentation.h"
#include "pqPropertyLinks.h"
#include "pqSignalAdaptors.h"
#include "pqPropertyManager.h"
#include "pqPipelineFilter.h"
#include "pqOutputPort.h"
#include "pqSMAdaptor.h"
#include "ui_pqCalculatorPanel.h"

class pqCalculatorPanel::pqInternal 
 : public QObject, public Ui::CalculatorPanel
{
public:
  pqInternal( QObject * p ) : QObject( p ) 
    { 
      this->AttributeModeAdaptor = NULL;
      this->AttributeModeLink.removeAllPropertyLinks();
    }
    
  ~pqInternal() 
    { 
      this->AttributeModeLink.removeAllPropertyLinks();
      if ( this->AttributeModeAdaptor )
        {
        delete this->AttributeModeAdaptor;
        this->AttributeModeAdaptor = NULL;
        }
    }
    
  QMenu                     ScalarsMenu;
  QMenu                     VectorsMenu;
  pqPropertyLinks           AttributeModeLink;
  pqSignalAdaptorComboBox * AttributeModeAdaptor;
};

//-----------------------------------------------------------------------------
/// constructor
pqCalculatorPanel::pqCalculatorPanel(pqProxy* pxy, QWidget* p) :
  pqObjectPanel(pxy, p)
{
  this->Internal = new pqInternal(this);
  this->Internal->setupUi(this);

  QObject::connect(this->Internal->AttributeMode,
                   SIGNAL(currentIndexChanged(const QString&)),
                   this,
                   SLOT(updateVariables(const QString&)));
  
  QObject::connect(this->Internal->AttributeMode,
                   SIGNAL(currentIndexChanged(const QString&)),
                   this->Internal->Function,
                   SLOT(clear()));
  
  this->Internal->Vectors->setMenu(&this->Internal->VectorsMenu);
  QObject::connect(&this->Internal->VectorsMenu,
                   SIGNAL(triggered(QAction*)),
                   this,
                   SLOT(variableChosen(QAction*)));
  
  this->Internal->Scalars->setMenu(&this->Internal->ScalarsMenu);
  QObject::connect(&this->Internal->ScalarsMenu,
                   SIGNAL(triggered(QAction*)),
                   this,
                   SLOT(variableChosen(QAction*)));
  
  
  // the following three connections make sure the arrays and associated
  // variables are timely updated 
  QObject::connect(  &this->Internal->ScalarsMenu,
                     SIGNAL( aboutToShow() ),
                     this,
                     SLOT( updateVariableNames() )  );
                   
  QObject::connect(  &this->Internal->VectorsMenu,
                     SIGNAL( aboutToShow() ),
                     this,
                     SLOT( updateVariableNames() )  );
                     
  QObject::connect(  this->Internal->Function,
                     SIGNAL( editingFinished() ),
                     this,
                     SLOT( updateVariableNames() )  );
  
  // the connection between the Qt widget and the ParaView proxy 
  //                   
  // As an example, signal editingFinished() released by QLineEdit invokes 
  // vtkPVArrayCalculator::SetFunction() to update the class IVAR while 
  // vtkPVArrayCalculator::SetFunction() invokes QLineEdit::text() to update 
  // the panel.
  this->propertyManager()->registerLink
        (   this->Internal->Function, "text", 
            SIGNAL( editingFinished() ),
            this->proxy(), 
            this->proxy()->GetProperty( "Function" )   );
           
  this->propertyManager()->registerLink
        (   this->Internal->ResultArrayName, "text", 
            SIGNAL( editingFinished() ),
            this->proxy(), 
            this->proxy()->GetProperty( "ResultArrayName" )   );
           
  this->propertyManager()->registerLink
        (   this->Internal->ReplacementValue, "text", 
            SIGNAL( editingFinished() ),
            this->proxy(), 
            this->proxy()->GetProperty( "ReplacementValue" )   );
           
  this->propertyManager()->registerLink
        (   this->Internal->ReplaceInvalidResult, "checked",
            SIGNAL(  stateChanged( int )  ),
            this->proxy(), 
            this->proxy()->GetProperty( "ReplaceInvalidValues" )   );
            
  this->propertyManager()->registerLink
        (   this->Internal->CoordinateResults, "checked",
            SIGNAL(  stateChanged( int )  ),
            this->proxy(), 
            this->proxy()->GetProperty( "CoordinateResults" )   );
            
  // a special case: pqSignalAdaptorComboBox is needed to connect the 
  // AttributeMode (a QComboBox widge) and the class IVAR
  this->Internal->AttributeModeAdaptor = new
                  pqSignalAdaptorComboBox( this->Internal->AttributeMode );
  this->Internal->AttributeModeAdaptor->setObjectName( "AttributeModeAdaptor" );
  this->Internal->AttributeModeLink.addPropertyLink
                  (   this->Internal->AttributeModeAdaptor, 
                      "currentText",
                      SIGNAL(  currentTextChanged( const QString & )  ), 
                      this->proxy(),
                      this->proxy()->GetProperty( "AttributeMode" )   );
  QObject::connect(  &this->Internal->AttributeModeLink,  
                      SIGNAL( smPropertyChanged() ),
                      this, 
                      SLOT( reset() )  
                  );


  // clicking on any button or any part of the panel where another button
  // doesn't take focus will cause the line edit to have focus 
  this->setFocusProxy(this->Internal->Function);
  
  // connect all buttons for which the text of the button 
  // is the same as what goes into the function 
  QRegExp regexp("^([ijk]Hat|ln|log10|sin|cos|"
                 "tan|asin|acos|atan|sinh|cosh|tanh|"
                 "sqrt|exp|ceil|floor|abs|norm|mag|"
                 "LeftParentheses|RightParentheses|"
                 "Divide|Multiply|Minus|Plus)$");
                 
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
  
  QToolButton* tb = this->Internal->xy;
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
  
  tb = this->Internal->v1v2;
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

  
  QObject::connect(this->Internal->Clear,
                   SIGNAL(pressed()),
                   this->Internal->Function,
                   SLOT(clear()));
 


  // mark panel modified if the following are changed 
  QObject::connect(this->Internal->Function,
                   SIGNAL(editingFinished()),
                   this,
                   SLOT(setModified()));
  QObject::connect(this->Internal->ResultArrayName,
                   SIGNAL(textEdited(const QString&)),
                   this,
                   SLOT(setModified()));
  QObject::connect(this->Internal->AttributeMode,
                   SIGNAL(currentIndexChanged(const QString&)),
                   this,
                   SLOT(setModified()));
  QObject::connect(this->Internal->ReplaceInvalidResult,
                   SIGNAL(stateChanged(int)),
                   this,
                   SLOT(setModified()));
  QObject::connect(this->Internal->ReplacementValue,
                   SIGNAL(textChanged(const QString&)),
                   this,
                   SLOT(setModified()));
  QObject::connect(this->Internal->CoordinateResults,
                   SIGNAL(toggled(bool)),
                   this,
                   SLOT(setModified()));
  QObject::connect(this->Internal->CoordinateResults,
                   SIGNAL(toggled(bool)),
                   this,
                   SLOT(disableResults(bool)));

  this->updateVariables(this->Internal->AttributeMode->currentText());
  this->reset();
}

//-----------------------------------------------------------------------------
/// destructor
pqCalculatorPanel::~pqCalculatorPanel()
{
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

  vtkSMProxy* CalcProxy = this->proxy();

  int mode = this->Internal->AttributeMode->currentText() == 
             "Point Data" ? 1 : 2;

  // put in new variables
  QList<QAction*> scalarActions = this->Internal->ScalarsMenu.actions();
  vtkSMStringVectorProperty* ScalarProperty;
  ScalarProperty = vtkSMStringVectorProperty::SafeDownCast(
       CalcProxy->GetProperty("AddScalarVariable"));
  if(ScalarProperty)
    {
    int offset = 0;
    if(mode == 1)
      {
      offset = 3;
      }
    int numVariables = scalarActions.count()-offset;

    for(int i=0; i<numVariables; i++)
      {
      QAction* a = scalarActions[i+offset];
      QString VarName = a->text();
      QString ArrayName = VarName;
      QString Component = QString("%1").arg(0);
      QVariant d = a->data();
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

  QList<QAction*> vectorActions = this->Internal->VectorsMenu.actions();
  vtkSMStringVectorProperty* VectorProperty;
  VectorProperty = vtkSMStringVectorProperty::SafeDownCast(
    CalcProxy->GetProperty("AddVectorVariable"));
  if(VectorProperty)
    {
    int offset = 0;
    if(mode == 1)
      {
      offset = 1;
      }
    int numVariables = vectorActions.count()-offset;
    for(int i=0; i<numVariables; i++)
      {
      QAction* a = vectorActions[i+offset];
      QString VarName = a->text();
      pqSMAdaptor::setMultipleElementProperty(VectorProperty, 5*i, VarName);
      pqSMAdaptor::setMultipleElementProperty(VectorProperty, 5*i+1, VarName);
      pqSMAdaptor::setMultipleElementProperty(VectorProperty, 5*i+2, "0");
      pqSMAdaptor::setMultipleElementProperty(VectorProperty, 5*i+3, "1");
      pqSMAdaptor::setMultipleElementProperty(VectorProperty, 5*i+4, "2");
      }
    VectorProperty->SetNumberOfElements(numVariables*5);
    }
  
  if(mode == 1)
    {
    vtkSMStringVectorProperty* CoordinateVectorProperty;
    CoordinateVectorProperty = vtkSMStringVectorProperty::SafeDownCast(
      CalcProxy->GetProperty("AddCoordinateVectorVariable"));
    if(CoordinateVectorProperty)
      {
      pqSMAdaptor::setMultipleElementProperty(CoordinateVectorProperty, 0, "coords");
      pqSMAdaptor::setMultipleElementProperty(CoordinateVectorProperty, 1, "0");
      pqSMAdaptor::setMultipleElementProperty(CoordinateVectorProperty, 2, "1");
      pqSMAdaptor::setMultipleElementProperty(CoordinateVectorProperty, 3, "2");
      CoordinateVectorProperty->SetNumberOfElements(4);
      }
      
    vtkSMStringVectorProperty* CoordinateScalarProperty;
    CoordinateScalarProperty = vtkSMStringVectorProperty::SafeDownCast(
      CalcProxy->GetProperty("AddCoordinateScalarVariable"));
    if(CoordinateScalarProperty)
      {
      pqSMAdaptor::setMultipleElementProperty(CoordinateScalarProperty, 0, "coordsX");
      pqSMAdaptor::setMultipleElementProperty(CoordinateScalarProperty, 1, "0");
      pqSMAdaptor::setMultipleElementProperty(CoordinateScalarProperty, 2, "coordsY");
      pqSMAdaptor::setMultipleElementProperty(CoordinateScalarProperty, 3, "1");
      pqSMAdaptor::setMultipleElementProperty(CoordinateScalarProperty, 4, "coordsZ");
      pqSMAdaptor::setMultipleElementProperty(CoordinateScalarProperty, 5, "2");
      CoordinateScalarProperty->SetNumberOfElements(6);
      }
    }
  
  pqSMAdaptor::setElementProperty(
                 CalcProxy->GetProperty("AttributeMode"),
                 mode);

  if(!this->Internal->ResultArrayName->testAttribute(Qt::WA_ForceDisabled))
    {
    pqSMAdaptor::setElementProperty(
                   CalcProxy->GetProperty("ResultArrayName"),
                   this->Internal->ResultArrayName->text());
    }
  
  pqSMAdaptor::setEnumerationProperty(
                 CalcProxy->GetProperty("CoordinateResults"), 
                 this->Internal->CoordinateResults->isChecked());
  
  pqSMAdaptor::setEnumerationProperty(
          CalcProxy->GetProperty("ReplaceInvalidValues"),
          this->Internal->ReplaceInvalidResult->isChecked());
  
  pqSMAdaptor::setElementProperty(
          CalcProxy->GetProperty("ReplacementValue"),
          this->Internal->ReplacementValue->text());

  pqSMAdaptor::setElementProperty(
                 CalcProxy->GetProperty("Function"),
                 this->Internal->Function->text());

  CalcProxy->UpdateVTKObjects();

}

//-----------------------------------------------------------------------------
/// reset the changes made
/// editor will query properties from the server manager
void pqCalculatorPanel::reset()
{
  this->pqObjectPanel::reset();
  
  vtkSMProxy* CalcProxy = this->proxy();


  // restore the attribute mode
  QVariant v = pqSMAdaptor::getElementProperty(CalcProxy->GetProperty("AttributeMode"));
  this->Internal->AttributeMode->setCurrentIndex(v.toInt() == 2 ? 1 : 0);
  
  // restore the function (after attribute or function will be cleared)
  v = pqSMAdaptor::getElementProperty(CalcProxy->GetProperty("Function"));
  this->Internal->Function->setText(v.toString());
  
  // restore the results array name
  v = pqSMAdaptor::getElementProperty(
          CalcProxy->GetProperty("ResultArrayName"));
  this->Internal->ResultArrayName->setText(v.toString());
  
  // restore the replace invalid results
  v = pqSMAdaptor::getEnumerationProperty(
         CalcProxy->GetProperty("CoordinateResults"));
  this->Internal->CoordinateResults->setChecked(v.toBool());

  // restore the replace invalid results
  v = pqSMAdaptor::getEnumerationProperty(
          CalcProxy->GetProperty("ReplaceInvalidValues"));
  this->Internal->ReplaceInvalidResult->setChecked(v.toBool());
  
  // restore the replacement value
  v = pqSMAdaptor::getElementProperty(
          CalcProxy->GetProperty("ReplacementValue"));
  this->Internal->ReplacementValue->setText(v.toString());

}

void pqCalculatorPanel::buttonPressed(const QString& buttonText)
{
  this->Internal->Function->insert(buttonText);
}

void pqCalculatorPanel::updateVariableNames()
{
  this->updateVariables( this->Internal->AttributeMode->currentText() );
}

void pqCalculatorPanel::updateVariables(const QString& mode)
{
  this->Internal->VectorsMenu.clear();
  this->Internal->ScalarsMenu.clear();

  if(mode == "Point Data")
    {
    this->Internal->VectorsMenu.addAction("coords");
    this->Internal->ScalarsMenu.addAction("coordsX");
    this->Internal->ScalarsMenu.addAction("coordsY");
    this->Internal->ScalarsMenu.addAction("coordsZ");
    }

  vtkPVDataSetAttributesInformation* fdi = NULL;
  pqPipelineFilter* f = qobject_cast<pqPipelineFilter*>(this->referenceProxy());
  if(!f)
    {
    return;
    }

  if(mode == "Point Data")
    {
    fdi = f->getInput(f->getInputPortName(0), 0)->getDataInformation()
      ->GetPointDataInformation();
    }
  else if(mode == "Cell Data")
    {
    fdi = f->getInput(f->getInputPortName(0), 0)->getDataInformation()
      ->GetCellDataInformation();
    }
  
  if(!fdi)
    {
    return;
    }

  for(int i=0; i<fdi->GetNumberOfArrays(); i++)
    {
    vtkPVArrayInformation* arrayInfo = fdi->GetArrayInformation(i);
    if (arrayInfo->GetDataType() == VTK_STRING
        || arrayInfo->GetDataType() == VTK_VARIANT )
      {
      continue;
      }

    int numComponents = arrayInfo->GetNumberOfComponents();
    QString name = arrayInfo->GetName();

    for(int j=0; j<numComponents; j++)
      {
      if(numComponents == 1)
        {
        this->Internal->ScalarsMenu.addAction(name);
        }
      else
        {        
        QString compName(arrayInfo->GetComponentName( j ));        
        QString n = name + QString( "_%1").arg( compName );
        QStringList d;
        d.append(name);
        d.append(QString("%1").arg(j));
        QAction* a = new QAction(n, &this->Internal->ScalarsMenu);
        a->setData(d);
        this->Internal->ScalarsMenu.addAction(a);
        }
      }

    if(numComponents == 3)
      {
      this->Internal->VectorsMenu.addAction(name);
      }
    }
}

void pqCalculatorPanel::variableChosen(QAction* a)
{
  if(a)
    {
    QString text = a->text();
    this->Internal->Function->insert(text);
    }
}

void pqCalculatorPanel::disableResults(bool e)
{
  this->Internal->ResultArrayName->setEnabled(!e);
}

