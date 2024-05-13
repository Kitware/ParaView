// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqUseSeparateColorMapReaction.h"

#include "pqActiveObjects.h"
#include "pqDataRepresentation.h"
#include "pqDisplayColorWidget.h"
#include "pqPropertyLinksConnection.h"
#include "pqUndoStack.h"
#include "pqView.h"

#include "vtkNew.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMScalarBarWidgetRepresentationProxy.h"
#include "vtkSMTransferFunctionProxy.h"

#include <QCoreApplication>
#include <QString>

//=============================================================================
/// This class makes it possible to add custom logic when updating the
/// "ColorArrayName" property instead of directly setting the SMProperty. The
/// custom logic, in this case, ensures that the LUT is setup and initialized
/// (all done by vtkSMColorMapEditorHelper::SetSelectedUseSeparateColorMap()).
class pqUseSeparateColorMapReaction::PropertyLinksConnection : public pqPropertyLinksConnection
{
private:
  using Superclass = pqPropertyLinksConnection;
  using ValueType = pqDisplayColorWidget::ValueType;

  vtkNew<vtkSMColorMapEditorHelper> ColorMapEditorHelper;

public:
  PropertyLinksConnection(QObject* qobject, const char* qproperty, const char* qsignal,
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex, bool use_unchecked_modified_event,
    QObject* parentObject = nullptr)
    : Superclass(qobject, qproperty, qsignal, smproxy, smproperty, smindex,
        use_unchecked_modified_event, parentObject)
  {
    this->ColorMapEditorHelper->SetSelectedPropertiesType(
      vtkSMColorMapEditorHelper::GetPropertyType(smproperty));
  }
  ~PropertyLinksConnection() override = default;

protected:
  /// Called to update the ServerManager Property due to UI change.
  void setServerManagerValue(bool use_unchecked, const QVariant& value) override
  {
    assert(use_unchecked == false);
    Q_UNUSED(use_unchecked);

    const int useSep = value.toBool() ? 1 : 0;

    const QString undoText = QCoreApplication::translate("PropertyLinksConnection", "Change ") +
      QCoreApplication::translate("ServerManagerXML", this->propertySM()->GetXMLLabel());
    BEGIN_UNDO_SET(undoText);
    this->ColorMapEditorHelper->SetSelectedUseSeparateColorMap(this->proxySM(), useSep);
    END_UNDO_SET();
  }

  /// called to get the current value for the ServerManager Property.
  QVariant currentServerManagerValue(bool use_unchecked) const override
  {
    assert(use_unchecked == false);
    Q_UNUSED(use_unchecked);

    vtkSMProxy* reprProxy = this->proxySM();

    vtkSMProperty* colorProp =
      this->ColorMapEditorHelper->GetSelectedUseSeparateColorMapProperty(reprProxy);
    if (!colorProp)
    {
      return 0;
    }
    auto useSeps = this->ColorMapEditorHelper->GetSelectedUseSeparateColorMaps(reprProxy);
    return !useSeps.empty() && useSeps[0] == 1 ? 1 : 0;
  }

private:
  Q_DISABLE_COPY(PropertyLinksConnection)
};

//-----------------------------------------------------------------------------
pqUseSeparateColorMapReaction::pqUseSeparateColorMapReaction(
  QAction* parentObject, pqDisplayColorWidget* colorWidget, bool track_active_objects)
  : Superclass(parentObject)
  , ColorWidget(colorWidget)
{
  parentObject->setCheckable(true);
  if (track_active_objects)
  {
    QObject::connect(&pqActiveObjects::instance(),
      QOverload<pqDataRepresentation*>::of(&pqActiveObjects::representationChanged), this,
      &pqUseSeparateColorMapReaction::setActiveRepresentation, Qt::QueuedConnection);
    this->setActiveRepresentation();
  }
  else
  {
    this->updateEnableState();
  }
}

//-----------------------------------------------------------------------------
pqUseSeparateColorMapReaction::~pqUseSeparateColorMapReaction() = default;

//-----------------------------------------------------------------------------
pqDataRepresentation* pqUseSeparateColorMapReaction::representation() const
{
  return this->Representation;
}

//-----------------------------------------------------------------------------
void pqUseSeparateColorMapReaction::setActiveRepresentation()
{
  this->setRepresentation(pqActiveObjects::instance().activeRepresentation());
}

//-----------------------------------------------------------------------------
void pqUseSeparateColorMapReaction::setRepresentation(
  pqDataRepresentation* repr, int selectedPropertiesType)
{
  if (this->Representation != nullptr && this->Representation == repr &&
    this->ColorMapEditorHelper->GetSelectedPropertiesType() == selectedPropertiesType)
  {
    return;
  }
  this->Links.clear();
  if (this->Representation)
  {
    this->disconnect(this->Representation);
    this->Representation = nullptr;
  }
  this->ColorMapEditorHelper->SetSelectedPropertiesType(selectedPropertiesType);
  if (repr)
  {
    this->Representation = repr;
    if (this->ColorMapEditorHelper->GetSelectedPropertiesType() ==
      vtkSMColorMapEditorHelper::SelectedPropertiesTypes::Blocks)
    {
      QObject::connect(this->Representation, &pqDataRepresentation::blockColorArrayNameModified,
        this, &pqUseSeparateColorMapReaction::updateEnableState, Qt::QueuedConnection);
    }
    else
    {
      QObject::connect(this->Representation, &pqDataRepresentation::colorArrayNameModified, this,
        &pqUseSeparateColorMapReaction::updateEnableState, Qt::QueuedConnection);
    }
  }
  vtkSMProxy* reprProxy = this->Representation ? this->Representation->getProxy() : nullptr;
  vtkSMProperty* useSeparateProp = reprProxy
    ? this->ColorMapEditorHelper->GetSelectedUseSeparateColorMapProperty(reprProxy)
    : nullptr;
  this->updateEnableState();
  // Create the link
  if (reprProxy && useSeparateProp)
  {
    this->Links.addPropertyLink<pqUseSeparateColorMapReaction::PropertyLinksConnection>(
      this->parentAction(), "checked", SIGNAL(toggled(bool)), reprProxy, useSeparateProp);
  }
}

//-----------------------------------------------------------------------------
void pqUseSeparateColorMapReaction::updateEnableState()
{
  // Recover proxy and action
  vtkSMProxy* reprProxy = this->Representation ? this->Representation->getProxy() : nullptr;
  vtkSMProperty* useSeparateProp = reprProxy
    ? this->ColorMapEditorHelper->GetSelectedUseSeparateColorMapProperty(reprProxy)
    : nullptr;
  const bool canSep = reprProxy && useSeparateProp &&
    this->ColorMapEditorHelper->GetAnySelectedUsingScalarColoring(reprProxy);
  bool isSep = false;
  if (canSep)
  {
    isSep = this->ColorMapEditorHelper->GetAnySelectedUseSeparateColorMap(reprProxy);
  }
  // Set action state
  this->parentAction()->setEnabled(canSep);
  this->parentAction()->setChecked(isSep);
}

//-----------------------------------------------------------------------------
void pqUseSeparateColorMapReaction::querySelectedUseSeparateColorMap()
{
  if (this->Representation && this->Links.getPropertyLink(0))
  {
    // force the current selected use separate color map to be queried.
    Q_EMIT this->Links.getPropertyLink(0)->smpropertyModified();
  }
}

//-----------------------------------------------------------------------------
void pqUseSeparateColorMapReaction::onTriggered()
{
  // Disable Multi Components Mapping
  pqDataRepresentation* repr = this->Representation.data();
  vtkSMRepresentationProxy* proxy = vtkSMRepresentationProxy::SafeDownCast(repr->getProxy());
  vtkSMProperty* mcmProperty = proxy->GetProperty("MultiComponentsMapping");
  if (vtkSMPropertyHelper(mcmProperty).GetAsInt() == 1)
  {
    const auto useSep = this->ColorMapEditorHelper->GetAnySelectedUseSeparateColorMap(proxy);
    if (!useSep)
    {
      vtkSMPropertyHelper(mcmProperty).Set(0);
    }
  }

  pqView* view = pqActiveObjects::instance().activeView();
  vtkSMProxy* viewProxy = view ? view->getProxy() : nullptr;

  if (this->ColorMapEditorHelper->GetAnySelectedUsingScalarColoring(proxy))
  {
    const vtkSMProperty* lutProp = this->ColorMapEditorHelper->GetSelectedColorArrayProperty(proxy);
    if (!lutProp)
    {
      auto luts = this->ColorMapEditorHelper->GetSelectedLookupTables(proxy);
      auto selectors = this->ColorMapEditorHelper->GetSelectedBlockSelectors(proxy);
      for (size_t i = 0; i < luts.size(); ++i)
      {
        if (vtkSMProxy* lutProxy = luts[i])
        {
          auto sbProxy = vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
            vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lutProxy, viewProxy));
          if (this->ColorMapEditorHelper->GetSelectedPropertiesType() ==
            vtkSMColorMapEditorHelper::SelectedPropertiesTypes::Representation)
          {
            sbProxy->RemoveRange(proxy);
          }
          else
          {
            sbProxy->RemoveBlockRange(proxy, selectors[i]);
          }
          sbProxy->UpdateVTKObjects();
        }
        else
        {
          qWarning("Failed to determine the LookupTable being used.");
        }
      }
    }
    else
    {
      qWarning("Missing 'LookupTable' property");
    }
  }

  // Force color widget to update representation and color map
  Q_EMIT this->ColorWidget->arraySelectionChanged();
}
