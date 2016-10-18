/*=========================================================================

   Program: ParaView
   Module:  pqSplinePropertyWidget.cxx

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
#include "pqSplinePropertyWidget.h"
#include "ui_pqSplinePropertyWidget.h"

#include "pqPointPickingHelper.h"
#include "pqSignalAdaptorTreeWidget.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"

#include <QPointer>

//-----------------------------------------------------------------------------

class pqSplinePropertyWidget::pqInternals
{
public:
  Ui::SplinePropertyWidget Ui;
  QPointer<pqSignalAdaptorTreeWidget> PointsAdaptor;
};

//-----------------------------------------------------------------------------
pqSplinePropertyWidget::pqSplinePropertyWidget(vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup,
  pqSplinePropertyWidget::ModeTypes mode, QWidget* parentObject)
  : Superclass("representations",
      (mode == pqSplinePropertyWidget::POLYLINE ? "PolyLineWidgetRepresentation"
                                                : "SplineWidgetRepresentation"),
      smproxy, smgroup, parentObject)
  , Internals(new pqSplinePropertyWidget::pqInternals())
{
  pqInternals& internals = (*this->Internals);
  Ui::SplinePropertyWidget& ui = internals.Ui;
  ui.setupUi(this);

  if (vtkSMProperty* handlePositions = smgroup->GetProperty("HandlePositions"))
  {
    internals.PointsAdaptor = new pqSignalAdaptorTreeWidget(ui.handlePositions, true);
    this->addPropertyLink(
      internals.PointsAdaptor, "values", SIGNAL(valuesChanged()), handlePositions);
  }
  else
  {
    qCritical("Missing required property for function 'HandlePositions'.");
  }

  if (vtkSMProperty* closed = smgroup->GetProperty("Closed"))
  {
    this->addPropertyLink(ui.closed, "checked", SIGNAL(toggled(bool)), closed);
    ui.closed->setText(closed->GetXMLLabel());
  }
  else
  {
    ui.closed->hide();
  }

  // link show3DWidget checkbox
  this->connect(ui.show3DWidget, SIGNAL(toggled(bool)), SLOT(setWidgetVisible(bool)));
  ui.show3DWidget->connect(this, SIGNAL(widgetVisibilityToggled(bool)), SLOT(setChecked(bool)));
  this->setWidgetVisible(ui.show3DWidget->isChecked());

  this->connect(ui.addPoint, SIGNAL(clicked()), SLOT(addPoint()));
  this->connect(ui.removePoints, SIGNAL(clicked()), SLOT(removePoints()));

  // Setup picking handlers
  pqPointPickingHelper* pickHelper = new pqPointPickingHelper(QKeySequence(tr("P")), false, this);
  pickHelper->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  pickHelper->connect(this, SIGNAL(widgetVisibilityUpdated(bool)), SLOT(setShortcutEnabled(bool)));
  this->connect(
    pickHelper, SIGNAL(pick(double, double, double)), SLOT(pick(double, double, double)));

  pqPointPickingHelper* pickHelper2 =
    new pqPointPickingHelper(QKeySequence(tr("Ctrl+P")), true, this);
  pickHelper2->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  pickHelper2->connect(this, SIGNAL(widgetVisibilityUpdated(bool)), SLOT(setShortcutEnabled(bool)));
  this->connect(
    pickHelper2, SIGNAL(pick(double, double, double)), SLOT(pick(double, double, double)));
}

//-----------------------------------------------------------------------------
pqSplinePropertyWidget::~pqSplinePropertyWidget()
{
}

//-----------------------------------------------------------------------------
void pqSplinePropertyWidget::setLineColor(const QColor& color)
{
  double dcolor[3];
  dcolor[0] = color.redF();
  dcolor[1] = color.greenF();
  dcolor[2] = color.blueF();

  vtkSMProxy* wdgProxy = this->widgetProxy();
  vtkSMPropertyHelper(wdgProxy, "LineColor").Set(dcolor, 3);
  wdgProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqSplinePropertyWidget::placeWidget()
{
  // nothing to do.
}

//-----------------------------------------------------------------------------
void pqSplinePropertyWidget::addPoint()
{
  pqInternals& internals = (*this->Internals);
  Ui::SplinePropertyWidget& ui = internals.Ui;

  QTreeWidgetItem* newItem = internals.PointsAdaptor->growTable();
  QTreeWidget* tree = ui.handlePositions;
  tree->setCurrentItem(newItem);
  // edit the first column.
  tree->editItem(newItem, 0);

  emit this->changeAvailable();
  this->render();
}

//-----------------------------------------------------------------------------
void pqSplinePropertyWidget::removePoints()
{
  pqInternals& internals = (*this->Internals);
  Ui::SplinePropertyWidget& ui = internals.Ui;

  QList<QTreeWidgetItem*> items = ui.handlePositions->selectedItems();
  foreach (QTreeWidgetItem* item, items)
  {
    if (ui.handlePositions->topLevelItemCount() <= 2)
    {
      qWarning("At least two point locations are required. Deletion request ignored.");
      // don't allow deletion of the last two points.
      return;
    }
    delete item;
  }

  emit this->changeAvailable();
  this->render();
}

//-----------------------------------------------------------------------------
void pqSplinePropertyWidget::pick(double argx, double argy, double argz)
{
  vtkSMProxy* widget = this->widgetProxy();
  pqInternals& internals = (*this->Internals);
  Ui::SplinePropertyWidget& ui = internals.Ui;

  QList<QTreeWidgetItem*> items = ui.handlePositions->selectedItems();
  if (items.size() > 0)
  {
    QTreeWidgetItem* item = items.front();
    item->setText(0, QString("%1").arg(argx));
    item->setText(1, QString("%1").arg(argy));
    item->setText(2, QString("%1").arg(argz));
  }
  widget->UpdateVTKObjects();
  emit this->changeAvailable();
  this->render();
}
