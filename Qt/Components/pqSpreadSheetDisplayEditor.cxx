/*=========================================================================

   Program: ParaView
   Module:    pqSpreadSheetDisplayEditor.cxx

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

========================================================================*/
#include "pqSpreadSheetDisplayEditor.h"
#include "ui_pqSpreadSheetDisplayEditor.h"

// Server Manager Includes.
#include "vtkSMProxy.h"
#include "vtkSMIntVectorProperty.h"

// Qt Includes.

// ParaView Includes.
#include "pqComboBoxDomain.h"
#include "pqPropertyLinks.h"
#include "pqRepresentation.h"
#include "pqServer.h"
#include "pqSignalAdaptorCompositeTreeWidget.h"
#include "pqSignalAdaptors.h"

//-----------------------------------------------------------------------------
class pqSpreadSheetDisplayEditor::pqInternal : public Ui::SpreadSheetDisplayEditor
{
public:
  pqPropertyLinks Links;
  pqSignalAdaptorComboBox* AttributeModeAdaptor;
  pqComboBoxDomain* AttributeModeDomain;
  pqSignalAdaptorSpinBox* ProcessIDAdaptor;
  pqSignalAdaptorCompositeTreeWidget* CompositeTreeAdaptor;
};

//-----------------------------------------------------------------------------
pqSpreadSheetDisplayEditor::pqSpreadSheetDisplayEditor(
  pqRepresentation* repr, QWidget* _parent) :Superclass(repr, _parent)
{
  this->Internal = new pqInternal();
  this->Internal->setupUi(this);
  
  this->Internal->AttributeModeAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->AttributeMode);
  this->Internal->ProcessIDAdaptor = new pqSignalAdaptorSpinBox(
    this->Internal->ProcessID);
  this->Internal->CompositeTreeAdaptor = new pqSignalAdaptorCompositeTreeWidget(
    this->Internal->CompositeTree, 
    vtkSMIntVectorProperty::SafeDownCast(
      repr->getProxy()->GetProperty("CompositeDataSetIndex")), 
    /*autoUpdateVisibility=*/true,
    /*showSelectedElementCounts=*/true);
  this->Internal->AttributeModeDomain = 0;

  this->setRepresentationInternal(repr);

  QObject::connect(this->Internal->AttributeMode,
                   SIGNAL(currentIndexChanged(const QString&)), 
                   this, 
                   SLOT(onAttributeModeChanged(const QString&)));

  this->onAttributeModeChanged(this->Internal->AttributeMode->currentText());
}

//-----------------------------------------------------------------------------
pqSpreadSheetDisplayEditor::~pqSpreadSheetDisplayEditor()
{
  delete this->Internal;
}


//-----------------------------------------------------------------------------
void pqSpreadSheetDisplayEditor::setRepresentationInternal(pqRepresentation* repr)
{
  vtkSMProxy* reprProxy = repr->getProxy();

  this->Internal->AttributeModeDomain = new pqComboBoxDomain(
    this->Internal->AttributeMode,
    reprProxy->GetProperty("FieldAssociation"), "enum");
  this->Internal->Links.addPropertyLink(this->Internal->ViewData,
    "checked", SIGNAL(stateChanged(int)),
    reprProxy, reprProxy->GetProperty("Visibility"));
  this->Internal->Links.addPropertyLink(this->Internal->AttributeModeAdaptor,
    "currentText", SIGNAL(currentTextChanged(const QString&)),
    reprProxy, reprProxy->GetProperty("FieldAssociation"));
  //this->Internal->Links.addPropertyLink(this->Internal->ProcessIDAdaptor,
  //  "value", SIGNAL(valueChanged(int)),
  //  reprProxy, reprProxy->GetProperty("ProcessID"));
  this->Internal->Links.addPropertyLink(this->Internal->CompositeTreeAdaptor,
    "values", SIGNAL(valuesChanged()),
    reprProxy, reprProxy->GetProperty("CompositeDataSetIndex"));

  QObject::connect(&this->Internal->Links, SIGNAL(qtWidgetChanged()),
    this, SLOT(updateAllViews()));

  // Update the label displaying the number of processes
  int numPartitions = repr->getServer()->getNumberOfPartitions();
  this->Internal->ProcessIDLabel->setText(
    QString("Process ID: (Range 0 - %1)").arg(numPartitions-1));

  // Update the upper bounds for the spin box
  this->Internal->ProcessID->setMaximum(numPartitions-1);
}


//-----------------------------------------------------------------------------
void pqSpreadSheetDisplayEditor::onAttributeModeChanged(const QString &mode)
{
  if(mode == "Field Data")
    {
    this->Internal->ProcessIDLabel->show();
    this->Internal->ProcessID->show();
    }
  else
    {
    this->Internal->ProcessIDLabel->hide();
    this->Internal->ProcessID->hide();
    }
}
