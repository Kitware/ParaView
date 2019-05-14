/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#include "pqIntegrationModelSeedHelperWidget.h"

#include "vtkLagrangianSeedHelper.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

#include "pqLineEdit.h"
#include "pqPropertiesPanel.h"

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>

#include <sstream>

//-----------------------------------------------------------------------------
pqIntegrationModelSeedHelperWidget::pqIntegrationModelSeedHelperWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproxy, smproperty, parentObject)
{
  // Connect to input property, so the widget is reset is the input is changed
  this->FlowInputProperty = vtkSMInputProperty::SafeDownCast(this->proxy()->GetProperty("Input"));
  this->VTKConnector->Connect(this->FlowInputProperty, vtkCommand::UncheckedPropertyModifiedEvent,
    this, SLOT(forceResetSeedWidget()));

  // Reset the widget
  this->resetSeedWidget(true);

  // Add a property link between the properry shown in the widget to the QProperty arrayToGenerate
  this->addPropertyLink(this, "arrayToGenerate", SIGNAL(arrayToGenerateChanged()), smproperty);
}

//-----------------------------------------------------------------------------
void pqIntegrationModelSeedHelperWidget::resetWidget()
{
  this->resetSeedWidget(false);
}

//-----------------------------------------------------------------------------
void pqIntegrationModelSeedHelperWidget::forceResetSeedWidget()
{
  this->resetSeedWidget(true);
}

//-----------------------------------------------------------------------------
void pqIntegrationModelSeedHelperWidget::resetSeedWidget(bool force)
{
  // Avoid resetting when it is not necessary
  if (force || this->ModelProperty->GetUncheckedProxy(0) != this->ModelPropertyValue)
  {
    // Recover the current unchecked value, ie the value selected by the user but not yet applied
    this->ModelPropertyValue = this->ModelProperty->GetUncheckedProxy(0);

    // Remove all previous layout and child widget
    qDeleteAll(this->children());

    // Recover model array names and components
    if (this->ModelPropertyValue)
    {
      this->ModelPropertyValue->UpdatePropertyInformation();
      vtkSMStringVectorProperty* namesProp = vtkSMStringVectorProperty::SafeDownCast(
        this->ModelPropertyValue->GetProperty("SeedArrayNames"));
      vtkSMIntVectorProperty* compsProp = vtkSMIntVectorProperty::SafeDownCast(
        this->ModelPropertyValue->GetProperty("SeedArrayComps"));
      vtkSMIntVectorProperty* typesProp = vtkSMIntVectorProperty::SafeDownCast(
        this->ModelPropertyValue->GetProperty("SeedArrayTypes"));

      // Check property number of elements
      unsigned int nArrays = namesProp->GetNumberOfElements();
      if (nArrays != compsProp->GetNumberOfElements())
      {
        qCritical() << "not the same number of names and components: "
                    << namesProp->GetNumberOfElements()
                    << " != " << compsProp->GetNumberOfElements()
                    << ", array generation may be invalid";
      }

      // Recover potential optional flow input
      vtkSMSourceProxy* flowProxy =
        vtkSMSourceProxy::SafeDownCast(this->FlowInputProperty->GetProxy(0));

      // Create main layout
      QGridLayout* gridLayout = new QGridLayout(this);
      gridLayout->setMargin(pqPropertiesPanel::suggestedMargin());
      gridLayout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
      gridLayout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
      gridLayout->setColumnStretch(0, 0);
      gridLayout->setColumnStretch(1, 1);

      for (unsigned int i = 0; i < nArrays; i++)
      {
        const char* arrayName = namesProp->GetElement(i);
        const char* labelName = vtkSMProperty::CreateNewPrettyLabel(arrayName);
        int type = typesProp->GetElement(i);
        int nComponents = compsProp->GetElement(i);

        // Create a group box for each array to generate
        QGroupBox* gb = new QGroupBox(labelName, this);
        gb->setCheckable(true);
        gb->setChecked(false);
        gb->setProperty("name", arrayName);
        gb->setProperty("type", type);
        QObject::connect(gb, SIGNAL(toggled(bool)), this, SIGNAL(arrayToGenerateChanged()));
        delete labelName;

        // Add a layout in each
        QGridLayout* gbLayout = new QGridLayout(gb);
        gbLayout->setMargin(pqPropertiesPanel::suggestedMargin());
        gbLayout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
        gbLayout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
        gb->setLayout(gbLayout);

        // Only with flow input
        if (flowProxy)
        {
          // Create a combo box to select generation mode
          QComboBox* cmb = new QComboBox(gb);
          cmb->setObjectName("Selector");
          cmb->addItem(tr("Constant"), vtkLagrangianSeedHelper::CONSTANT);
          cmb->addItem(tr("Flow Interpolation"), vtkLagrangianSeedHelper::FLOW);
          QObject::connect(cmb, SIGNAL(currentIndexChanged(int)), this, SLOT(updateEnabledState()));
          QObject::connect(
            cmb, SIGNAL(currentIndexChanged(int)), this, SIGNAL(arrayToGenerateChanged()));
          gbLayout->addWidget(cmb, 0, 0, 1, nComponents);
        }

        // Add a pqLineEdit for each component
        for (int j = 0; j < nComponents; j++)
        {
          pqLineEdit* line = new pqLineEdit(gb);
          QDoubleValidator* val = new QDoubleValidator(line);
          line->setValidator(val);
          QObject::connect(
            line, SIGNAL(textChangedAndEditingFinished()), this, SIGNAL(arrayToGenerateChanged()));
          gbLayout->addWidget(line, 1, j, 1, 1);
        }

        // Only with flow input
        if (flowProxy)
        {
          // Create a combo box with each array from the flow
          QComboBox* cmbArray = new QComboBox(gb);
          cmbArray->setObjectName("Arrays");
          cmbArray->setEnabled(false);

          vtkPVDataSetAttributesInformation* pointDataInfo =
            flowProxy->GetDataInformation()->GetAttributeInformation(
              vtkDataObject::FIELD_ASSOCIATION_POINTS);
          vtkPVDataSetAttributesInformation* cellDataInfo =
            flowProxy->GetDataInformation()->GetAttributeInformation(
              vtkDataObject::FIELD_ASSOCIATION_CELLS);
          QPixmap cellPixmap(":/pqWidgets/Icons/pqCellData16.png");
          QPixmap pointPixmap(":/pqWidgets/Icons/pqPointData16.png");

          for (int ipd = 0; ipd < pointDataInfo->GetNumberOfArrays(); ipd++)
          {
            vtkPVArrayInformation* arrayInfo = pointDataInfo->GetArrayInformation(ipd);
            if (arrayInfo->GetNumberOfComponents() == nComponents)
            {
              cmbArray->addItem(
                QIcon(pointPixmap), arrayInfo->GetName(), vtkDataObject::FIELD_ASSOCIATION_POINTS);
            }
          }
          for (int icd = 0; icd < cellDataInfo->GetNumberOfArrays(); icd++)
          {
            vtkPVArrayInformation* arrayInfo = cellDataInfo->GetArrayInformation(icd);
            if (arrayInfo->GetNumberOfComponents() == nComponents)
            {
              cmbArray->addItem(
                QIcon(cellPixmap), arrayInfo->GetName(), vtkDataObject::FIELD_ASSOCIATION_CELLS);
            }
          }
          gbLayout->addWidget(cmbArray, 2, 0, 1, nComponents);
          QObject::connect(
            cmbArray, SIGNAL(currentIndexChanged(int)), this, SIGNAL(arrayToGenerateChanged()));
        }
        gridLayout->addWidget(gb, i, 0);
      }
    }
    emit(this->arrayToGenerateChanged());
  }
}

//-----------------------------------------------------------------------------
void pqIntegrationModelSeedHelperWidget::updateEnabledState()
{
  QComboBox* cb = qobject_cast<QComboBox*>(QObject::sender());
  QGroupBox* gb = qobject_cast<QGroupBox*>(cb->parent());
  bool constant = cb->itemData(cb->currentIndex()).toInt() == vtkLagrangianSeedHelper::CONSTANT;
  gb->findChild<QComboBox*>("Arrays")->setEnabled(!constant);
  foreach (pqLineEdit* line, gb->findChildren<pqLineEdit*>())
  {
    line->setEnabled(constant);
  }
}

//-----------------------------------------------------------------------------
QList<QVariant> pqIntegrationModelSeedHelperWidget::arrayToGenerate() const
{
  QList<QVariant> values;
  foreach (QGroupBox* gb, this->findChildren<QGroupBox*>())
  {
    // Recover the state of each groupbox
    if (gb->isChecked())
    {
      QList<pqLineEdit*> lines = gb->findChildren<pqLineEdit*>();
      QComboBox* cb = gb->findChild<QComboBox*>("Selector");
      QComboBox* cbArray = gb->findChild<QComboBox*>("Arrays");
      int constOrFlow =
        cb ? cb->itemData(cb->currentIndex()).toInt() : vtkLagrangianSeedHelper::CONSTANT;
      unsigned int nComponents = lines.size();

      // Fill property
      values.push_back(gb->property("name"));
      values.push_back(gb->property("type"));
      values.push_back(constOrFlow);
      values.push_back(nComponents);
      std::ostringstream str;
      if (constOrFlow == vtkLagrangianSeedHelper::CONSTANT)
      {
        for (unsigned int i = 0; i < nComponents; i++)
        {
          str << lines[i]->text().toLatin1().data() << ";";
        }
        values.push_back(str.str().c_str());
      }
      else
      {
        str << cbArray->itemData(cbArray->currentIndex()).toInt() << ';'
            << cbArray->currentText().toLatin1().data();
        values.push_back(str.str().c_str());
      }
    }
  }
  return values;
}

//-----------------------------------------------------------------------------
void pqIntegrationModelSeedHelperWidget::setArrayToGenerate(const QList<QVariant>& values)
{
  QGroupBox* gb;
  QList<QGroupBox*> gbs = this->findChildren<QGroupBox*>();
  foreach (gb, gbs)
  {
    gb->setChecked(false);
  }

  for (QList<QVariant>::const_iterator i = values.begin(); i != values.end(); i += 5)
  {
    // Incremental python filling value check
    // When using python, this method is called incrementally,
    // this allows to wait for the correct call
    if ((i + 4)->toString().isEmpty())
    {
      continue;
    }

    // Recover array name
    QString arrayName = i->toString();
    int type = (i + 1)->toInt();
    gb = nullptr;
    foreach (gb, gbs)
    {
      // Identify correct combo box
      if (arrayName == gb->property("name"))
      {
        break;
      }
    }
    if (!gb)
    {
      qCritical() << "Could not find group box with name" << arrayName;
      continue;
    }
    // Check type
    if (gb->property("type") != type)
    {
      qCritical() << "Dynamic array typing is not supported, type is ignored";
    }

    // Set it checked
    gb->setChecked(true);

    // Recover selector state
    int constOrFlow = (i + 2)->toInt();
    QComboBox* cb = gb->findChild<QComboBox*>("Selector");
    if (!cb)
    {
      if (constOrFlow != vtkLagrangianSeedHelper::CONSTANT)
      {
        continue;
      }
    }
    else
    {
      for (int j = 0; j < cb->count(); j++)
      {
        if (cb->itemData(j) == constOrFlow)
        {
          cb->setCurrentIndex(j);
          break;
        }
      }
    }
    // Recover values
    int nComponents = (i + 3)->toInt();
    QStringList dataStrings = (i + 4)->toString().split(';', QString::SkipEmptyParts);
    if (constOrFlow == vtkLagrangianSeedHelper::CONSTANT)
    {
      QList<pqLineEdit*> lines = gb->findChildren<pqLineEdit*>();
      if (lines.count() != nComponents || dataStrings.size() != nComponents)
      {
        qCritical() << "Unexpected number of components or values";
        continue;
      }
      for (int j = 0; j < nComponents; j++)
      {
        lines[j]->setText(dataStrings[j]);
      }
    }
    else
    {
      if (dataStrings.size() != 2)
      {
        qCritical() << "Unexpected number of values";
        continue;
      }
      QComboBox* cbArray = gb->findChild<QComboBox*>("Arrays");
      for (int j = 0; j < cbArray->count(); j++)
      {
        if (cbArray->itemData(j) == dataStrings[0] && cbArray->itemText(j) == dataStrings[1])
        {
          cbArray->setCurrentIndex(j);
          break;
        }
      }
    }
  }
}
