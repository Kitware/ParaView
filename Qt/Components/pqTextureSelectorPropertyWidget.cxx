/*=========================================================================

   Program: ParaView
   Module: pqTextureSelectorPropertyWidget.h

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

#include "pqTextureSelectorPropertyWidget.h"

// ParaView Includes
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqServerManagerModel.h"
#include "pqTextureComboBox.h"
#include "pqUndoStack.h"
#include "pqView.h"

// Server Manager Includes
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
  this->setToolTip("Select/Load texture to apply.");

  QVBoxLayout* l = new QVBoxLayout;
  l->setMargin(0);

  // Recover domain and sanity check
  this->Domain = smProperty->FindDomain<vtkSMProxyGroupDomain>();
  if (!this->Domain || this->Domain->GetNumberOfGroups() != 1 ||
    strcmp(this->Domain->GetGroup(0), pqTextureComboBox::TEXTURES_GROUP.c_str()) != 0)
  {
    qCritical() << "pqTextureSelectorPropertyWidget can only be used with a ProxyProperty"
                   " with a ProxyGroupDomain containing only the \""
                << QString(pqTextureComboBox::TEXTURES_GROUP.c_str()) << "\" group";
  }

  // Create the combobox selector and set its value
  this->Selector = new pqTextureComboBox(this->Domain, this);
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
  vtkPVXMLElement* hints = smProperty->GetHints()
    ? smProperty->GetHints()->FindNestedElementByName("TextureSelectorWidget")
    : NULL;
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
    }
    this->checkAttributes(checkTCoords, checkTangents);
  }
}

//-----------------------------------------------------------------------------
void pqTextureSelectorPropertyWidget::onTextureChanged(vtkSMProxy* texture)
{
  SM_SCOPED_TRACE(ChooseTexture).arg(this->proxy()).arg(texture).arg(this->property());
  BEGIN_UNDO_SET("Texture Change");
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
  this->Selector->updateFromTexture(vtkSMPropertyHelper(this->property()).GetAsProxy());
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
        this->setToolTip("No tcoords present in the data. Cannot apply texture.");
      }
      else if (tangents &&
        pdInfo->GetAttributeInformation(vtkDataSetAttributes::TANGENTS) == nullptr)
      {
        enable = false;
        this->setToolTip("No tangents present in the data. Cannot apply texture.");
      }
    }
  }
  this->Selector->setEnabled(enable);
  if (enable)
  {
    this->setToolTip("");
  }
}
