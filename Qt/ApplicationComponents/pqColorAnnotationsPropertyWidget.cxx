// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqColorAnnotationsPropertyWidget.h"

#include "pqActiveObjects.h"
#include "pqAnnotationsModel.h"
#include "pqColorAnnotationsWidget.h"
#include "pqCoreUtilities.h"
#include "pqDataRepresentation.h"
#include "pqPropertyWidgetDecorator.h"

#include "vtkAbstractArray.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMVectorProperty.h"

#include <QMessageBox>
#include <QPointer>
#include <QString>
#include <QTextStream>
#include <QVBoxLayout>

namespace
{

//-----------------------------------------------------------------------------
// Decorator used to hide the widget when using IndexedLookup.
class pqColorAnnotationsPropertyWidgetDecorator : public pqPropertyWidgetDecorator
{
  typedef pqPropertyWidgetDecorator Superclass;
  bool IsAdvanced;

public:
  pqColorAnnotationsPropertyWidgetDecorator(vtkPVXMLElement* xmlArg, pqPropertyWidget* parentArg)
    : Superclass(xmlArg, parentArg)
    , IsAdvanced(false)
  {
  }
  ~pqColorAnnotationsPropertyWidgetDecorator() override = default;

  void setIsAdvanced(bool val)
  {
    if (val != this->IsAdvanced)
    {
      this->IsAdvanced = val;
      Q_EMIT this->visibilityChanged();
    }
  }
  bool canShowWidget(bool show_advanced) const override
  {
    return this->IsAdvanced ? show_advanced : true;
  }

private:
  Q_DISABLE_COPY(pqColorAnnotationsPropertyWidgetDecorator)
};
}

//=============================================================================
class pqColorAnnotationsPropertyWidget::pqInternals
{
public:
  QPointer<pqColorAnnotationsPropertyWidgetDecorator> Decorator;
  pqColorAnnotationsWidget* AnnotationsWidget;
  vtkNew<vtkEventQtSlotConnect> VTKConnector;

  pqInternals(pqColorAnnotationsPropertyWidget* self)
  {
    this->AnnotationsWidget = new pqColorAnnotationsWidget(self);
    this->AnnotationsWidget->setLookupTableProxy(self->proxy());
    self->setLayout(new QVBoxLayout{});
    self->layout()->addWidget(this->AnnotationsWidget);

    this->Decorator = new pqColorAnnotationsPropertyWidgetDecorator(nullptr, self);
  }
};

//=============================================================================
pqColorAnnotationsPropertyWidget::pqColorAnnotationsPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqInternals(this))
{
  this->addPropertyLink(
    this, "annotations", SIGNAL(annotationsChanged()), smproxy->GetProperty("Annotations"));

  auto presetProp = this->proxy()->GetProperty("LastPresetName");
  const char* name = "KAAMS";
  if (presetProp)
  {
    name = vtkSMPropertyHelper(presetProp).GetAsString();
  }
  this->Internals->AnnotationsWidget->applyPreset(name);

  this->addPropertyLink(
    this, "indexedColors", SIGNAL(indexedColorsChanged()), smproxy->GetProperty("IndexedColors"));

  this->addPropertyLink(this, "indexedOpacities", SIGNAL(indexedOpacitiesChanged()),
    smproxy->GetProperty("IndexedOpacities"));

  // if proxy has a property named IndexedLookup, "Color" can be controlled only
  // when IndexedLookup is ON.
  if (smproxy->GetProperty("IndexedLookup"))
  {
    // we are not controlling the IndexedLookup property, we are merely
    // observing it to ensure the UI is updated correctly. Hence we don't fire
    // any signal to update the smproperty.
    this->Internals->VTKConnector->Connect(smproxy->GetProperty("IndexedLookup"),
      vtkCommand::ModifiedEvent, this, SLOT(updateIndexedLookupState()));

    this->updateIndexedLookupState();

    // Add decorator so the widget can be marked as advanced when IndexedLookup
    // is OFF.
    this->addDecorator(this->Internals->Decorator);
  }

  if (smgroup->GetProperty("EnableOpacityMapping"))
  {
    this->addPropertyLink(this, "opacityMapping", SIGNAL(opacityMappingChanged()),
      smgroup->GetProperty("EnableOpacityMapping"));
  }
  else
  {
    this->Internals->AnnotationsWidget->supportsOpacityMapping(false);
  }

  this->connect(this->Internals->AnnotationsWidget, &pqColorAnnotationsWidget::presetChanged, this,
    &pqColorAnnotationsPropertyWidget::changeFinished);
  this->connect(this->Internals->AnnotationsWidget, &pqColorAnnotationsWidget::indexedColorsChanged,
    this, &pqColorAnnotationsPropertyWidget::indexedColorsChanged);
  this->connect(this->Internals->AnnotationsWidget,
    &pqColorAnnotationsWidget::indexedOpacitiesChanged, this,
    &pqColorAnnotationsPropertyWidget::indexedOpacitiesChanged);
  this->connect(this->Internals->AnnotationsWidget, &pqColorAnnotationsWidget::annotationsChanged,
    this, &pqColorAnnotationsPropertyWidget::annotationsChanged);
  this->connect(this->Internals->AnnotationsWidget,
    &pqColorAnnotationsWidget::opacityMappingChanged, this,
    &pqColorAnnotationsPropertyWidget::opacityMappingChanged);

  this->Internals->AnnotationsWidget->setColumnVisibility(pqAnnotationsModel::VISIBILITY, false);
}

//-----------------------------------------------------------------------------
pqColorAnnotationsPropertyWidget::~pqColorAnnotationsPropertyWidget()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::updateIndexedLookupState()
{
  if (this->proxy()->GetProperty("IndexedLookup"))
  {
    bool val = vtkSMPropertyHelper(this->proxy(), "IndexedLookup").GetAsInt() != 0;

    this->Internals->AnnotationsWidget->indexedLookupStateUpdated(val);
    this->Internals->Decorator->setIsAdvanced(!val);

    pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
    if (val && repr && repr->isVisible())
    {
      this->Internals->AnnotationsWidget->supportsOpacityMapping(true);
      vtkSMPropertyHelper annotationsInitialized(this->proxy(), "AnnotationsInitialized");
      if (!annotationsInitialized.GetAsInt())
      {
        // Attempt to add active annotations.
        bool success = this->Internals->AnnotationsWidget->addActiveAnnotations(
          false /* do not force generation */);
        if (!success)
        {
          QString promptMessage;
          QTextStream qs(&promptMessage);
          qs << tr("Could not initialize annotations for categorical coloring. There may be too "
                   "many discrete values in your data, (more than %1) or you may be coloring by a "
                   "floating point data array. Please add annotations manually.")
                  .arg(vtkAbstractArray::MAX_DISCRETE_VALUES);
          pqCoreUtilities::promptUser("pqColorAnnotationsPropertyWidget::updatedIndexedLookupState",
            QMessageBox::Information,
            tr("Could not determine discrete values to use for annotations"), promptMessage,
            QMessageBox::Ok | QMessageBox::Save);
        }
        else
        {
          annotationsInitialized.Set(1);
        }
      }
    }
    else
    {
      this->Internals->AnnotationsWidget->supportsOpacityMapping(false);
    }
  }
}

//-----------------------------------------------------------------------------
QList<QVariant> pqColorAnnotationsPropertyWidget::annotations() const
{
  return this->Internals->AnnotationsWidget->annotations();
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::setAnnotations(const QList<QVariant>& value)
{
  this->Internals->AnnotationsWidget->setAnnotations(value);
}

//-----------------------------------------------------------------------------
QList<QVariant> pqColorAnnotationsPropertyWidget::indexedColors() const
{
  return this->Internals->AnnotationsWidget->indexedColors();
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::setIndexedColors(const QList<QVariant>& value)
{
  this->Internals->AnnotationsWidget->setIndexedColors(value);
}

//-----------------------------------------------------------------------------
QList<QVariant> pqColorAnnotationsPropertyWidget::indexedOpacities() const
{
  return this->Internals->AnnotationsWidget->indexedOpacities();
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::setIndexedOpacities(const QList<QVariant>& value)
{
  this->Internals->AnnotationsWidget->setIndexedOpacities(value);
}

//-----------------------------------------------------------------------------
QVariant pqColorAnnotationsPropertyWidget::opacityMapping() const
{
  return this->Internals->AnnotationsWidget->opacityMapping();
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::setOpacityMapping(const QVariant& value)
{
  this->Internals->AnnotationsWidget->setOpacityMapping(value);
}
