// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqXYChartViewBoundsPropertyWidget.h"
#include "ui_pqXYChartViewBoundsPropertyWidget.h"

#include "pqActiveObjects.h"
#include "pqXYChartView.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMContextViewProxy.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"

#include <array>

//-----------------------------------------------------------------------------
struct pqXYChartViewBoundsPropertyWidget::pqInternals
{
  Ui::XYChartViewBoundsPropertyWidget Ui;
  bool BoundsInitialized = false;
  QList<QVariant> Bounds{ -1, -1, -1, -1 };
  vtkNew<vtkEventQtSlotConnect> VTKConnect;
  pqXYChartView* ConnectedView = nullptr;
};

//-----------------------------------------------------------------------------
pqXYChartViewBoundsPropertyWidget::pqXYChartViewBoundsPropertyWidget(
  vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parent)
  : pqPropertyWidget(proxy, parent)
  , Internals(new pqInternals)
{
  Ui::XYChartViewBoundsPropertyWidget& ui = this->Internals->Ui;
  ui.setupUi(this);
  ui.Reset->setEnabled(false);

  // Keep track of the active view
  pqActiveObjects& activeObjects = pqActiveObjects::instance();
  QObject::connect(&activeObjects, &pqActiveObjects::viewChanged, this,
    &pqXYChartViewBoundsPropertyWidget::connectToView);
  this->connectToView(activeObjects.activeView());

  // Use a custom label
  this->setShowLabel(false);
  ui.BoundsLabel->setText(property->GetXMLLabel());

  // Connect the text widgets
  this->updateTextFromBounds();
  QObject::connect(ui.BottomAxisMin, &pqDoubleLineEdit::textChangedAndEditingFinished, this,
    &pqXYChartViewBoundsPropertyWidget::onTextChanged);
  QObject::connect(ui.BottomAxisMax, &pqDoubleLineEdit::textChangedAndEditingFinished, this,
    &pqXYChartViewBoundsPropertyWidget::onTextChanged);
  QObject::connect(ui.LeftAxisMin, &pqDoubleLineEdit::textChangedAndEditingFinished, this,
    &pqXYChartViewBoundsPropertyWidget::onTextChanged);
  QObject::connect(ui.LeftAxisMax, &pqDoubleLineEdit::textChangedAndEditingFinished, this,
    &pqXYChartViewBoundsPropertyWidget::onTextChanged);
  this->addPropertyLink(this, "bounds", SIGNAL(boundsChanged()), property);

  // Connect the reset button
  QObject::connect(
    ui.Reset, &QToolButton::clicked, this, &pqXYChartViewBoundsPropertyWidget::resetBounds);
  Q_EMIT this->boundsChanged();
}

//-----------------------------------------------------------------------------
pqXYChartViewBoundsPropertyWidget::~pqXYChartViewBoundsPropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqXYChartViewBoundsPropertyWidget::connectToView(pqView* view)
{
  pqXYChartView* xyView = qobject_cast<pqXYChartView*>(view);
  if (this->Internals->ConnectedView != xyView)
  {
    this->Internals->ConnectedView = xyView;
    bool nullView = this->Internals->ConnectedView == nullptr;

    Ui::XYChartViewBoundsPropertyWidget& ui = this->Internals->Ui;
    ui.Reset->setEnabled(!nullView);
    ui.Reset->highlight(nullView);

    if (!nullView)
    {
      vtkSMContextViewProxy* proxy = this->Internals->ConnectedView->getContextViewProxy();

      // Check the first property to make sure this is a XYChartView or a derived proxy.
      // Assume the other properties exists.
      if (proxy->GetProperty("BottomAxisRangeMinimum"))
      {
        this->Internals->VTKConnect->Disconnect();
        this->Internals->VTKConnect->Connect(proxy->GetProperty("BottomAxisRangeMinimum"),
          vtkCommand::ModifiedEvent, ui.Reset, SLOT(highlight()));
        this->Internals->VTKConnect->Connect(proxy->GetProperty("BottomAxisRangeMaximum"),
          vtkCommand::ModifiedEvent, ui.Reset, SLOT(highlight()));
        this->Internals->VTKConnect->Connect(proxy->GetProperty("LeftAxisRangeMinimum"),
          vtkCommand::ModifiedEvent, ui.Reset, SLOT(highlight()));
        this->Internals->VTKConnect->Connect(proxy->GetProperty("LeftAxisRangeMaximum"),
          vtkCommand::ModifiedEvent, ui.Reset, SLOT(highlight()));
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqXYChartViewBoundsPropertyWidget::showEvent(QShowEvent* event)
{
  if (this->Internals->ConnectedView && !this->Internals->BoundsInitialized)
  {
    this->resetBounds();
  }
  this->Superclass::showEvent(event);
}

//-----------------------------------------------------------------------------
void pqXYChartViewBoundsPropertyWidget::resetBounds()
{
  Ui::XYChartViewBoundsPropertyWidget& ui = this->Internals->Ui;
  ui.Reset->clear();

  if (this->Internals->ConnectedView)
  {
    vtkSMContextViewProxy* proxy = this->Internals->ConnectedView->getContextViewProxy();

    // Check the first property to make sure this is a XYChartView or a derived proxy.
    // Assume the other properties exists.
    if (proxy->GetProperty("BottomAxisRangeMinimum"))
    {
      QList<QVariant> bds;
      bds.append(vtkSMPropertyHelper(proxy, "BottomAxisRangeMinimum").GetAsDouble());
      bds.append(vtkSMPropertyHelper(proxy, "BottomAxisRangeMaximum").GetAsDouble());
      bds.append(vtkSMPropertyHelper(proxy, "LeftAxisRangeMinimum").GetAsDouble());
      bds.append(vtkSMPropertyHelper(proxy, "LeftAxisRangeMaximum").GetAsDouble());
      this->setBounds(bds);
    }
  }
}

//-----------------------------------------------------------------------------
QList<QVariant> pqXYChartViewBoundsPropertyWidget::bounds()
{
  return this->Internals->Bounds;
}

//-----------------------------------------------------------------------------
void pqXYChartViewBoundsPropertyWidget::setBounds(const QList<QVariant>& bds)
{
  if (bds.size() < 4)
  {
    return;
  }

  if (this->Internals->Bounds != bds)
  {
    this->Internals->Bounds = bds;
    this->Internals->BoundsInitialized = true;
    this->updateTextFromBounds();
    Q_EMIT this->boundsChanged();
  }
}

//-----------------------------------------------------------------------------
void pqXYChartViewBoundsPropertyWidget::onTextChanged()
{
  Ui::XYChartViewBoundsPropertyWidget& ui = this->Internals->Ui;
  QList<QVariant> bds;
  bds.append(ui.BottomAxisMin->text());
  bds.append(ui.BottomAxisMax->text());
  bds.append(ui.LeftAxisMin->text());
  bds.append(ui.LeftAxisMax->text());
  this->setBounds(bds);
}

//-----------------------------------------------------------------------------
void pqXYChartViewBoundsPropertyWidget::updateTextFromBounds()
{
  Ui::XYChartViewBoundsPropertyWidget& ui = this->Internals->Ui;
  ui.BottomAxisMin->setText(this->Internals->Bounds[0].toString());
  ui.BottomAxisMax->setText(this->Internals->Bounds[1].toString());
  ui.LeftAxisMin->setText(this->Internals->Bounds[2].toString());
  ui.LeftAxisMax->setText(this->Internals->Bounds[3].toString());
}
