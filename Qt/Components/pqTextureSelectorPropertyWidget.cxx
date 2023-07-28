// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqTextureSelectorPropertyWidget.h"

// ParaView Includes
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqServerManagerModel.h"
#include "pqTextureComboBox.h"
#include "pqUndoStack.h"
#include "pqView.h"

// Server Manager Includes
#include "vtkDataSetAttributes.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyGroupDomain.h"
#include "vtkSMTrace.h"

// Qt Includes
#include <QVBoxLayout>

pqTextureSelectorPropertyWidget::pqTextureSelectorPropertyWidget(
  vtkSMProxy* smProxy, vtkSMProperty* smProperty, QWidget* pWidget)
  : pqPropertyWidget(smProxy, pWidget)
{
  this->setProperty(smProperty);
  this->setToolTip(tr("Select/Load texture to apply."));

  QVBoxLayout* l = new QVBoxLayout;
  l->setContentsMargins(0, 0, 0, 0);

  // Create the combobox selector and set its value
  auto* domain = smProperty->FindDomain<vtkSMProxyGroupDomain>();
  bool canLoadNew = true;
  vtkPVXMLElement* hints = smProperty->GetHints()
    ? smProperty->GetHints()->FindNestedElementByName("TextureSelectorWidget")
    : nullptr;
  if (hints)
  {
    QString attr = hints->GetAttributeOrDefault("can_load_new", "1");
    canLoadNew = static_cast<bool>(attr.toInt());
  }

  this->Selector = new pqTextureComboBox(domain, canLoadNew, this);
  this->onPropertyChanged();
  l->addWidget(this->Selector);
  this->setLayout(l);

  // Connect the combo box to the property
  QObject::connect(
    this->Selector, SIGNAL(textureChanged(vtkSMProxy*)), this, SLOT(onTextureChanged(vtkSMProxy*)));
  this->VTKConnector->Connect(
    smProperty, vtkCommand::ModifiedEvent, this, SLOT(onPropertyChanged()));

  pqServerManagerModel* smm = pqApplicationCore::instance()->getServerManagerModel();

  // If check_tcoords="1" is specified, we enabled the widget only if tcoords are available
  // Valid only for a RepresentationProxy
  if (hints)
  {
    bool checkTCoords = strcmp(hints->GetAttributeOrDefault("check_tcoords", ""), "1") == 0;
    bool checkTangents = strcmp(hints->GetAttributeOrDefault("check_tangents", ""), "1") == 0;

    this->Representation = smm->findItem<pqDataRepresentation*>(smProxy);
    if (this->Representation)
    {
      QObject::connect(this->Representation, &pqDataRepresentation::dataUpdated, this,
        [=] { this->checkAttributes(checkTCoords, checkTangents); });

      QObject::connect(this->Representation, &pqDataRepresentation::attrArrayNameModified, this,
        [=] { this->checkAttributes(checkTCoords, checkTangents); });
      this->checkAttributes(checkTCoords, checkTangents);
    }
  }
}

//-----------------------------------------------------------------------------
void pqTextureSelectorPropertyWidget::onTextureChanged(vtkSMProxy* texture)
{
  SM_SCOPED_TRACE(ChooseTexture).arg(this->proxy()).arg(texture).arg(this->property());
  BEGIN_UNDO_SET(tr("Texture Change"));
  vtkSMPropertyHelper(this->property()).Set(texture);
  this->proxy()->UpdateVTKObjects();
  END_UNDO_SET();
  Q_EMIT this->changeAvailable();
  Q_EMIT this->changeFinished();
}

//-----------------------------------------------------------------------------
void pqTextureSelectorPropertyWidget::onPropertyChanged()
{
  bool block = this->blockSignals(true);
  vtkSMProxy* proxy = vtkSMPropertyHelper(this->property()).GetAsProxy();
  this->Selector->updateFromTexture(proxy);
  this->blockSignals(block);
}

//-----------------------------------------------------------------------------
void pqTextureSelectorPropertyWidget::checkAttributes(bool tcoords, bool tangents)
{
  bool enable = true;
  // Enable only if we have point texture coordinates.
  vtkPVDataInformation* dataInfo = this->Representation->getRepresentedDataInformation();
  if (dataInfo)
  {
    vtkPVDataSetAttributesInformation* pdInfo = dataInfo->GetPointDataInformation();
    if (pdInfo)
    {
      if (tcoords && pdInfo->GetAttributeInformation(vtkDataSetAttributes::TCOORDS) == nullptr)
      {
        enable = false;
        this->setToolTip(tr("No tcoords present in the data. Cannot apply texture."));
      }
      else if (tangents &&
        pdInfo->GetAttributeInformation(vtkDataSetAttributes::TANGENTS) == nullptr)
      {
        enable = false;
        this->setToolTip(tr("No tangents present in the data. Cannot apply texture."));
      }
    }
  }
  this->Selector->setEnabled(enable);
  if (enable)
  {
    this->setToolTip("");
  }
}
