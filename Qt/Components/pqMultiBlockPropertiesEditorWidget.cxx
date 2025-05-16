// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqMultiBlockPropertiesEditorWidget.h"

#include "pqCoreUtilities.h"
#include "pqMultiBlockPropertiesStateWidget.h"
#include "pqPropertiesPanel.h"
#include "pqPropertyWidget.h"
#include "pqProxyWidget.h"
#include "pqUndoStack.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <QAction>
#include <QGridLayout>
#include <QLabel>
#include <QObject>
#include <QPointer>
#include <QSizePolicy>
#include <QStyle>
#include <QVBoxLayout>

#include <QCoreApplication>
#include <algorithm>

//-----------------------------------------------------------------------------
class pqMultiBlockPropertiesEditorWidget::pqInternals : public QObject
{
public:
  pqInternals() { this->ColorMapEditorHelper->SetSelectedPropertiesTypeToBlocks(); }

  QPointer<QVBoxLayout> VerticalLayout;

  QPointer<pqProxyWidget> HelperProxyWidget;
  QPointer<QGridLayout> HelperLayout;

  QPointer<pqPropertyWidget> MapScalarsWidget;
  QPointer<pqMultiBlockPropertiesStateWidget> MapScalarsStateWidget;

  QPointer<pqPropertyWidget> InterpolateScalarsBeforeMappingWidget;
  QPointer<pqMultiBlockPropertiesStateWidget> InterpolateScalarsBeforeMappingStateWidget;

  QPointer<QLabel> OpacityLabel;
  QPointer<pqPropertyWidget> OpacityWidget;
  QPointer<pqMultiBlockPropertiesStateWidget> OpacityStateWidget;

  vtkNew<vtkSMColorMapEditorHelper> ColorMapEditorHelper;
};

//-----------------------------------------------------------------------------
pqMultiBlockPropertiesEditorWidget::pqMultiBlockPropertiesEditorWidget(
  vtkSMProxy* proxy, vtkSMPropertyGroup* smGroup, QWidget* parent)
  : Superclass(proxy, smGroup, parent)
  , Internals(new pqMultiBlockPropertiesEditorWidget::pqInternals())
{
  auto& internals = *this->Internals;

  // create the layout
  QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
  sizePolicy.setHorizontalStretch(0);
  sizePolicy.setVerticalStretch(0);
  sizePolicy.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
  this->setSizePolicy(sizePolicy);
  internals.VerticalLayout = new QVBoxLayout(this);
  internals.VerticalLayout->setSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
  internals.VerticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
  internals.VerticalLayout->setContentsMargins(pqPropertiesPanel::suggestedMargins());
  internals.VerticalLayout->setStretch(0, 1);

  // create a proxy widget with the listed properties
  internals.HelperProxyWidget = new pqProxyWidget(proxy,
    { "BlockMapScalarsGUI", "BlockInterpolateScalarsBeforeMappingsGUI", "BlockOpacitiesGUI" },
    { "multiblock_inspector" }, {}, true, this);
  internals.HelperProxyWidget->setApplyChangesImmediately(false);
  internals.HelperProxyWidget->updatePanel();

  // add the proxy widget to the layout
  // internals.Ui.verticalLayout->addWidget(internals.HelperProxyWidget);
  internals.VerticalLayout->addWidget(internals.HelperProxyWidget);

  // extract the widgets from the proxy widget
  internals.MapScalarsWidget =
    internals.HelperProxyWidget->findChild<pqPropertyWidget*>("BlockMapScalarsGUI");
  internals.InterpolateScalarsBeforeMappingWidget =
    internals.HelperProxyWidget->findChild<pqPropertyWidget*>(
      "BlockInterpolateScalarsBeforeMappingsGUI");
  internals.OpacityWidget =
    internals.HelperProxyWidget->findChild<pqPropertyWidget*>("BlockOpacitiesGUI");
  if (internals.OpacityWidget)
  {
    internals.OpacityLabel = internals.HelperProxyWidget->findChild<QLabel*>(
      QString("%1Label").arg(internals.OpacityWidget->objectName()));
  }
  internals.HelperLayout = internals.HelperProxyWidget->findChild<QGridLayout*>();

  // connect the widgets to the actual properties and create state widgets which you listen from
  const int iconSize = std::max(this->style()->pixelMetric(QStyle::PM_SmallIconSize), 20);
  const int numColumns = internals.HelperLayout->columnCount();
  if (internals.MapScalarsWidget)
  {
    internals.MapScalarsWidget->setEnabled(false);
    QObject::connect(internals.MapScalarsWidget, &pqPropertyWidget::changeFinished, this,
      [&]()
      {
        const QString undoText = tr("Change ") +
          QCoreApplication::translate(
            "ServerManagerXML", this->proxy()->GetProperty("BlockMapScalars")->GetXMLLabel());
        BEGIN_UNDO_SET(undoText);
        const vtkSMUncheckedPropertyHelper mapScalarsHelper(internals.MapScalarsWidget->property());
        internals.ColorMapEditorHelper->SetSelectedMapScalars(
          this->proxy(), mapScalarsHelper.GetAsString());
        Q_EMIT this->changeFinished();
        END_UNDO_SET();
      });

    internals.MapScalarsStateWidget = new pqMultiBlockPropertiesStateWidget(
      proxy, { "BlockMapScalars" }, iconSize, QString(), this);
    internals.HelperLayout->addWidget(
      internals.MapScalarsStateWidget, 1, numColumns, 1, 1, Qt::AlignVCenter);
    QObject::connect(internals.MapScalarsStateWidget,
      &pqMultiBlockPropertiesStateWidget::stateChanged, this,
      &pqMultiBlockPropertiesEditorWidget::updateMapScalarsWidget);
    QObject::connect(internals.MapScalarsStateWidget,
      &pqMultiBlockPropertiesStateWidget::selectedBlockSelectorsChanged, this,
      &pqMultiBlockPropertiesEditorWidget::updateMapScalarsWidget);
    QObject::connect(internals.MapScalarsStateWidget,
      &pqMultiBlockPropertiesStateWidget::endStateReset, this,
      [&]() { Q_EMIT this->changeFinished(); });
    pqCoreUtilities::connect(this->proxy()->GetProperty("MapScalars"), vtkCommand::ModifiedEvent,
      this, SLOT(updateMapScalarsWidget()));
  }
  if (internals.InterpolateScalarsBeforeMappingWidget)
  {
    internals.InterpolateScalarsBeforeMappingWidget->setEnabled(false);
    QObject::connect(internals.InterpolateScalarsBeforeMappingWidget,
      &pqPropertyWidget::changeFinished, this,
      [&]()
      {
        const QString undoText = tr("Change ") +
          QCoreApplication::translate("ServerManagerXML",
            this->proxy()->GetProperty("BlockInterpolateScalarsBeforeMappings")->GetXMLLabel());
        BEGIN_UNDO_SET(undoText);
        const vtkSMUncheckedPropertyHelper interpolateScalarsHelper(
          internals.InterpolateScalarsBeforeMappingWidget->property());
        internals.ColorMapEditorHelper->SetSelectedInterpolateScalarsBeforeMapping(
          this->proxy(), interpolateScalarsHelper.GetAsInt());
        Q_EMIT this->changeFinished();
        END_UNDO_SET();
      });

    internals.InterpolateScalarsBeforeMappingStateWidget = new pqMultiBlockPropertiesStateWidget(
      proxy, { "BlockInterpolateScalarsBeforeMappings" }, iconSize, QString(), this);
    internals.HelperLayout->addWidget(
      internals.InterpolateScalarsBeforeMappingStateWidget, 2, numColumns, 1, 1, Qt::AlignVCenter);
    QObject::connect(internals.InterpolateScalarsBeforeMappingStateWidget,
      &pqMultiBlockPropertiesStateWidget::stateChanged, this,
      &pqMultiBlockPropertiesEditorWidget::updateInterpolateScalarsBeforeMappingWidget);
    QObject::connect(internals.InterpolateScalarsBeforeMappingStateWidget,
      &pqMultiBlockPropertiesStateWidget::selectedBlockSelectorsChanged, this,
      &pqMultiBlockPropertiesEditorWidget::updateInterpolateScalarsBeforeMappingWidget);
    QObject::connect(internals.InterpolateScalarsBeforeMappingStateWidget,
      &pqMultiBlockPropertiesStateWidget::endStateReset, this,
      [&]() { Q_EMIT this->changeFinished(); });
    pqCoreUtilities::connect(this->proxy()->GetProperty("InterpolateScalarsBeforeMapping"),
      vtkCommand::ModifiedEvent, this, SLOT(updateInterpolateScalarsBeforeMappingWidget()));
  }
  if (internals.OpacityWidget)
  {
    internals.OpacityLabel->setEnabled(false);
    internals.OpacityWidget->setEnabled(false);
    QObject::connect(internals.OpacityWidget, &pqPropertyWidget::changeFinished, this,
      [&]()
      {
        const QString undoText = tr("Change ") +
          QCoreApplication::translate(
            "ServerManagerXML", this->proxy()->GetProperty("BlockOpacities")->GetXMLLabel());
        BEGIN_UNDO_SET(undoText);
        const vtkSMUncheckedPropertyHelper opacityHelper(internals.OpacityWidget->property());
        internals.ColorMapEditorHelper->SetSelectedOpacity(
          this->proxy(), opacityHelper.GetAsDouble());
        Q_EMIT this->changeFinished();
        END_UNDO_SET();
      });

    internals.OpacityStateWidget =
      new pqMultiBlockPropertiesStateWidget(proxy, { "BlockOpacities" }, iconSize, QString(), this);
    internals.HelperLayout->addWidget(
      internals.OpacityStateWidget, 3, numColumns, 1, 1, Qt::AlignVCenter);
    QObject::connect(internals.OpacityStateWidget, &pqMultiBlockPropertiesStateWidget::stateChanged,
      this, &pqMultiBlockPropertiesEditorWidget::updateOpacityWidget);
    QObject::connect(internals.OpacityStateWidget,
      &pqMultiBlockPropertiesStateWidget::selectedBlockSelectorsChanged, this,
      &pqMultiBlockPropertiesEditorWidget::updateOpacityWidget);
    QObject::connect(internals.OpacityStateWidget,
      &pqMultiBlockPropertiesStateWidget::endStateReset, this,
      [&]() { Q_EMIT this->changeFinished(); });
    pqCoreUtilities::connect(this->proxy()->GetProperty("Opacity"), vtkCommand::ModifiedEvent, this,
      SLOT(updateOpacityWidget()));
  }
}

//-----------------------------------------------------------------------------
pqMultiBlockPropertiesEditorWidget::~pqMultiBlockPropertiesEditorWidget() = default;

//-----------------------------------------------------------------------------
void pqMultiBlockPropertiesEditorWidget::updateMapScalarsWidget()
{
  auto& internals = *this->Internals;
  const bool prev = this->blockSignals(true);
  auto state = internals.MapScalarsStateWidget->getState();
  // the state widget has already updated itself
  internals.MapScalarsWidget->setEnabled(state != 0 /*Disabled*/);

  vtkSMUncheckedPropertyHelper mapScalarsHelper(internals.MapScalarsWidget->property());
  if (state <= pqMultiBlockPropertiesStateWidget::BlockPropertyState::RepresentationInherited)
  {
    mapScalarsHelper.Set(internals.ColorMapEditorHelper->GetMapScalars(this->proxy()));
  }
  else
  {
    auto selectedBlockSelectors =
      internals.ColorMapEditorHelper->GetSelectedBlockSelectors(this->proxy());
    auto selectorAndState = internals.ColorMapEditorHelper->HasBlockProperty(
      this->proxy(), selectedBlockSelectors, "BlockMapScalars");
    mapScalarsHelper.Set(
      internals.ColorMapEditorHelper->GetBlockMapScalars(this->proxy(), selectorAndState.first));
  }
  this->blockSignals(prev);
}

//-----------------------------------------------------------------------------
void pqMultiBlockPropertiesEditorWidget::updateInterpolateScalarsBeforeMappingWidget()
{
  auto& internals = *this->Internals;
  const bool prev = this->blockSignals(true);

  auto state = internals.InterpolateScalarsBeforeMappingStateWidget->getState();
  // the state widget has already updated itself
  internals.InterpolateScalarsBeforeMappingWidget->setEnabled(state != 0 /*Disabled*/);

  vtkSMUncheckedPropertyHelper interpolateScalarsHelper(
    internals.InterpolateScalarsBeforeMappingWidget->property());
  if (state <= pqMultiBlockPropertiesStateWidget::BlockPropertyState::RepresentationInherited)
  {
    interpolateScalarsHelper.Set(
      internals.ColorMapEditorHelper->GetInterpolateScalarsBeforeMapping(this->proxy()));
  }
  else
  {
    auto selectedBlockSelectors =
      internals.ColorMapEditorHelper->GetSelectedBlockSelectors(this->proxy());
    auto selectorAndState = internals.ColorMapEditorHelper->HasBlockProperty(
      this->proxy(), selectedBlockSelectors, "BlockInterpolateScalarsBeforeMappings");
    interpolateScalarsHelper.Set(
      internals.ColorMapEditorHelper->GetBlockInterpolateScalarsBeforeMapping(
        this->proxy(), selectorAndState.first));
  }
  this->blockSignals(prev);
}

//-----------------------------------------------------------------------------
void pqMultiBlockPropertiesEditorWidget::updateOpacityWidget()
{
  auto& internals = *this->Internals;
  const bool prev = this->blockSignals(true);

  auto state = internals.OpacityStateWidget->getState();
  // the state widget has already updated itself
  internals.OpacityLabel->setEnabled(state != 0 /*Disabled*/);
  internals.OpacityWidget->setEnabled(state != 0 /*Disabled*/);

  vtkSMUncheckedPropertyHelper opacityHelper(internals.OpacityWidget->property());
  if (state <= pqMultiBlockPropertiesStateWidget::BlockPropertyState::RepresentationInherited)
  {
    opacityHelper.Set(internals.ColorMapEditorHelper->GetOpacity(this->proxy()));
  }
  else
  {
    auto selectedBlockSelectors =
      internals.ColorMapEditorHelper->GetSelectedBlockSelectors(this->proxy());
    auto selectorAndState = internals.ColorMapEditorHelper->HasBlockProperty(
      this->proxy(), selectedBlockSelectors, "BlockOpacities");
    opacityHelper.Set(
      internals.ColorMapEditorHelper->GetBlockOpacity(this->proxy(), selectorAndState.first));
  }
  this->blockSignals(prev);
}
