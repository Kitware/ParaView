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
#include "pqFindDataSelectionDisplayFrame.h"
#include "ui_pqFindDataSelectionDisplayFrame.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqOutputPort.h"
#include "pqPropertiesPanel.h"
#include "pqPropertyLinks.h"
#include "pqProxyWidgetDialog.h"
#include "pqRenderViewBase.h"
#include "pqSelectionManager.h"
#include "pqSignalAdaptors.h"
#include "pqUndoStack.h"
#include "vtkDataObject.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMInteractiveSelectionPipeline.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"

#include <QMenu>
#include <QPointer>

#include <cassert>

class pqFindDataSelectionDisplayFrame::pqInternals
{
  vtkSmartPointer<vtkSMProxy> FrustumWidget;
  QPointer<pqView> FrustumView;

public:
  QPointer<pqView> View;
  QPointer<pqOutputPort> Port;
  Ui::FindDataSelectionDisplayFrame Ui;
  pqPropertyLinks Links;
  QMenu CellLabelsMenu;
  QMenu PointLabelsMenu;

  //---------------------------------------------------------------------------
  pqInternals(pqFindDataSelectionDisplayFrame* self)
    : CellLabelsMenu(self)
    , PointLabelsMenu(self)
  {
    this->CellLabelsMenu.setObjectName("CellLabelsMenu");
    this->PointLabelsMenu.setObjectName("PointLabelsMenu");

    this->Ui.setupUi(self);
    this->Ui.interactiveSelectionColor->setVisible(false);
    this->Ui.labelPropertiesInteractiveSelection->setVisible(false);
    this->Ui.horizontalLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->Ui.horizontalLayout->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());

    this->Ui.cellLabelsButton->setMenu(&this->CellLabelsMenu);
    this->Ui.pointLabelsButton->setMenu(&this->PointLabelsMenu);

    self->connect(&this->CellLabelsMenu, SIGNAL(aboutToShow()), SLOT(fillCellLabels()));
    self->connect(
      &this->CellLabelsMenu, SIGNAL(triggered(QAction*)), SLOT(cellLabelSelected(QAction*)));

    self->connect(&this->PointLabelsMenu, SIGNAL(aboutToShow()), SLOT(fillPointLabels()));
    self->connect(
      &this->PointLabelsMenu, SIGNAL(triggered(QAction*)), SLOT(pointLabelSelected(QAction*)));
    self->connect(
      this->Ui.labelPropertiesSelection, SIGNAL(clicked()), SLOT(editLabelPropertiesSelection()));
    self->connect(this->Ui.labelPropertiesInteractiveSelection, SIGNAL(clicked()),
      SLOT(editLabelPropertiesInteractiveSelection()));
    self->connect(this->Ui.showFrustumButton, SIGNAL(clicked(bool)), SLOT(showFrustum(bool)));
  }

  //---------------------------------------------------------------------------
  void updatePanel(pqFindDataSelectionDisplayFrame* self)
  {
    // this->Ui.labels->setPort(this->Port);
    try
    {
      this->Links.clear();

      if (!this->View || !this->Port || !qobject_cast<pqRenderViewBase*>(this->View))
      {
        throw false;
      }
      pqDataRepresentation* repr = this->Port->getRepresentation(this->View);
      if (!repr || !repr->isVisible())
      {
        throw false;
      }
    }
    catch (bool)
    {
      self->setEnabled(false);
      this->destroyFrustum();
      return;
    }

    self->setEnabled(true);

    // Link the selection color to the global selection color so that it will
    // affect all views, otherwise the user may be get confused.
    vtkSMProxy* colorPalette =
      this->Port->getServer()->proxyManager()->GetProxy("global_properties", "ColorPalette");
    if (colorPalette)
    {
      this->Links.addPropertyLink(this->Ui.selectionColor, "chosenColorRgbF",
        SIGNAL(chosenColorChanged(const QColor&)), colorPalette,
        colorPalette->GetProperty("SelectionColor"));
      this->Links.addPropertyLink(this->Ui.interactiveSelectionColor, "chosenColorRgbF",
        SIGNAL(chosenColorChanged(const QColor&)), colorPalette,
        colorPalette->GetProperty("InteractiveSelectionColor"));
    }
    this->showFrustum(this->Ui.showFrustumButton->isChecked());
  }

  //---------------------------------------------------------------------------
  vtkPVDataSetAttributesInformation* attributeInformation(int fieldAssociation)
  {
    return this->Port ? this->Port->getDataInformation()->GetAttributeInformation(fieldAssociation)
                      : NULL;
  }

  //---------------------------------------------------------------------------
  // fill the menu with available arrays. Ensure that the menu shows the
  // currently selected label field correctly.
  void fillLabels(int fieldAssociation)
  {
    QMenu& menu = fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS
      ? this->CellLabelsMenu
      : this->PointLabelsMenu;
    menu.clear();

    vtkPVDataSetAttributesInformation* attrInfo = this->attributeInformation(fieldAssociation);
    if (!attrInfo)
    {
      menu.addAction("(not available)");
      return;
    }

    pqDataRepresentation* repr = this->Port->getRepresentation(this->View);
    assert(repr != NULL);

    menu.addAction("ID")->setData(fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS
        ? "vtkOriginalCellIds"
        : "vtkOriginalPointIds");

    for (int cc = 0; cc < attrInfo->GetNumberOfArrays(); cc++)
    {
      vtkPVArrayInformation* arrayInfo = attrInfo->GetArrayInformation(cc);
      // important to mark partial array since that array may be totally missing
      // from the selection ;).
      if (arrayInfo->GetIsPartial())
      {
        QAction* action = menu.addAction(QString("%1 (partial)").arg(arrayInfo->GetName()));
        action->setData(arrayInfo->GetName());
      }
      else
      {
        QAction* action = menu.addAction(arrayInfo->GetName());
        action->setData(arrayInfo->GetName());
      }
    }

    QString pname = (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
      ? "SelectionCellFieldDataArrayName"
      : "SelectionPointFieldDataArrayName";
    QString vname = (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
      ? "SelectionCellLabelVisibility"
      : "SelectionPointLabelVisibility";

    QString cur_array;
    if (vtkSMPropertyHelper(repr->getProxy(), vname.toLocal8Bit().data(), true).GetAsInt() != 0)
    {
      cur_array = vtkSMPropertyHelper(repr->getProxy(), pname.toLocal8Bit().data(), /*quiet=*/true)
                    .GetAsString();
    }

    foreach (QAction* action, menu.actions())
    {
      action->setCheckable(true);
      if (cur_array.isEmpty() == false && action->data().toString() == cur_array)
      {
        action->setChecked(true);
      }
    }
  }

  //---------------------------------------------------------------------------
  // Set the active representation  to label using the array mentioned.
  void labelBy(int fieldAssociation, QAction* action)
  {
    assert(this->Port && this->View && this->Port->getRepresentation(this->View));
    pqDataRepresentation* repr = this->Port->getRepresentation(this->View);
    vtkSMProxy* selectionProxy = repr->getProxy();
    vtkSMRenderViewProxy* viewProxy = vtkSMRenderViewProxy::SafeDownCast(this->View->getProxy());
    if (!viewProxy)
    {
      return;
    }
    vtkSMProxy* iSelectionProxy =
      vtkSMInteractiveSelectionPipeline::GetInstance()->GetSelectionRepresentation();

    const char* selectionArrayName = (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
      ? "SelectionCellFieldDataArrayName"
      : "SelectionPointFieldDataArrayName";
    const char* selectionVisibilityName =
      (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
      ? "SelectionCellLabelVisibility"
      : "SelectionPointLabelVisibility";
    const char* iSelectionArrayName = (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
      ? "CellFieldDataArrayName"
      : "PointFieldDataArrayName";
    const char* iSelectionVisibilityName =
      (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS) ? "CellLabelVisibility"
                                                                   : "PointLabelVisibility";

    BEGIN_UNDO_SET("Change labels");
    if (action->isChecked())
    {
      // selection
      vtkSMPropertyHelper(selectionProxy, selectionVisibilityName, true).Set(1);
      vtkSMPropertyHelper(selectionProxy, selectionArrayName, true)
        .Set(action->data().toString().toLocal8Bit().data());
      // interactive selection
      if (iSelectionProxy)
      {
        vtkSMPropertyHelper(iSelectionProxy, iSelectionVisibilityName, true).Set(1);
        vtkSMPropertyHelper(iSelectionProxy, iSelectionArrayName, true)
          .Set(action->data().toString().toLocal8Bit().data());
      }
    }
    else
    {
      // selection
      vtkSMPropertyHelper(selectionProxy, selectionVisibilityName, true).Set(0);
      vtkSMPropertyHelper(selectionProxy, selectionArrayName, true).Set("");
      // interactive selection
      if (iSelectionProxy)
      {
        vtkSMPropertyHelper(iSelectionProxy, iSelectionVisibilityName, true).Set(0);
        vtkSMPropertyHelper(iSelectionProxy, iSelectionArrayName, true).Set("");
      }
    }
    selectionProxy->UpdateVTKObjects();
    if (iSelectionProxy)
    {
      iSelectionProxy->UpdateVTKObjects();
    }
    END_UNDO_SET();

    this->View->render();
  }
  //---------------------------------------------------------------------------
  void destroyFrustum()
  {
    if (this->FrustumWidget)
    {
      if (this->FrustumView)
      {
        vtkSMPropertyHelper(this->FrustumView->getProxy(), "HiddenProps", true)
          .Remove(this->FrustumWidget);
        this->FrustumView->getProxy()->UpdateVTKObjects();
        this->FrustumView->render();
      }
    }
    this->FrustumWidget = NULL;
    this->FrustumView = NULL;
  }

  //---------------------------------------------------------------------------
  // create the frustum widget if none exists. Add it to the current view  and
  // update it's visibility.
  void showFrustum(bool val)
  {
    if (!this->View || !this->Port)
    {
      return;
    }

    vtkSMSourceProxy* selSource = this->Port->getSelectionInput();
    if (!selSource || strcmp(selSource->GetXMLName(), "FrustumSelectionSource") != 0)
    {
      val = false; // cannot show frustum.
      this->Ui.showFrustumButton->setEnabled(false);
    }
    else
    {
      this->Ui.showFrustumButton->setEnabled(true);
    }

    // we are connecting showFrustum() slot to clicked(bool) signal hence, this
    // doesn't result in calling this method again.
    this->Ui.showFrustumButton->setChecked(val);

    if (this->FrustumWidget == NULL && val == true)
    {
      vtkSMSessionProxyManager* pxm = this->View->proxyManager();
      this->FrustumWidget.TakeReference(pxm->NewProxy("representations", "FrustumWidget"));
      if (this->FrustumWidget)
      {
        this->FrustumWidget->PrototypeOn();
        this->FrustumWidget->UpdateVTKObjects();
      }
    }

    if (this->FrustumWidget == NULL)
    {
      // nothing to do.
      return;
    }

    pqView* targetView = val ? this->View : NULL;
    if (targetView != this->FrustumView)
    {
      if (this->FrustumView)
      {
        vtkSMPropertyHelper(this->FrustumView->getProxy(), "HiddenProps", true)
          .Remove(this->FrustumWidget);
        this->FrustumView->getProxy()->UpdateVTKObjects();
        this->FrustumView->render();
        this->FrustumView = NULL;
      }

      if (targetView)
      {
        this->FrustumView = targetView;
        vtkSMPropertyHelper(this->FrustumView->getProxy(), "HiddenProps", true)
          .Add(this->FrustumWidget);
        this->FrustumView->getProxy()->UpdateVTKObjects();
        this->FrustumView->render();
      }
    }

    if (targetView)
    {
      // update rendered frustum.
      vtkSMPropertyHelper frustum(selSource, "Frustum");
      double values[24];
      int index = 0;
      for (int cc = 0; cc < 8; cc++)
      {
        for (int kk = 0; kk < 3; kk++)
        {
          values[index] = frustum.GetAsDouble(4 * cc + kk);
          index++;
        }
      }
      vtkSMPropertyHelper(this->FrustumWidget, "Frustum").Set(values, 24);
      this->FrustumWidget->UpdateVTKObjects();
    }
  }
};

//-----------------------------------------------------------------------------
pqFindDataSelectionDisplayFrame::pqFindDataSelectionDisplayFrame(
  QWidget* parentObject, Qt::WindowFlags wflags)
  : Superclass(parentObject, wflags)
  , Internals(new pqInternals(this))
{
  this->connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->setView(pqActiveObjects::instance().activeView());

  pqActiveObjects& activeObjects = pqActiveObjects::instance();
  this->connect(
    &activeObjects, SIGNAL(portChanged(pqOutputPort*)), SLOT(setSelectedPort(pqOutputPort*)));
  this->connect(&activeObjects, SIGNAL(dataUpdated()), SLOT(onDataUpdated()));
  this->setSelectedPort(activeObjects.activePort());
  // if no pqSelectionManager, then one must use public API to set the active
  // selection manually.
}

//-----------------------------------------------------------------------------
pqFindDataSelectionDisplayFrame::~pqFindDataSelectionDisplayFrame()
{
  delete this->Internals;
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
void pqFindDataSelectionDisplayFrame::setView(pqView* view)
{
  if (this->Internals->View)
  {
    this->disconnect(this->Internals->View, SIGNAL(selectionModeChanged(bool)), this,
      SLOT(onSelectionModeChanged(bool)));
  }

  this->Internals->View = view;
  if (this->Internals->View)
  {
    this->connect(this->Internals->View, SIGNAL(selectionModeChanged(bool)), this,
      SLOT(onSelectionModeChanged(bool)));
  }

  this->Internals->updatePanel(this);
}

//-----------------------------------------------------------------------------
void pqFindDataSelectionDisplayFrame::onSelectionModeChanged(bool frustum)
{
  // Only change visibility of the frustum if we should turn it off.
  // We don't want to turn it on automatically.
  if (!frustum)
  {
    this->showFrustum(frustum);
  }

  this->Internals->updatePanel(this);
}

//-----------------------------------------------------------------------------
void pqFindDataSelectionDisplayFrame::updatePanel()
{
  this->Internals->updatePanel(this);
}

//-----------------------------------------------------------------------------
void pqFindDataSelectionDisplayFrame::setSelectedPort(pqOutputPort* port)
{
  if (this->Internals->Port)
  {
    this->disconnect(this->Internals->Port);
  }
  this->Internals->Port = port;
  this->Internals->updatePanel(this);
  if (port)
  {
    // this is needed to ensure that the frame is enabled/disabled when the
    // visibility of the port in the active view changes.
    this->connect(
      port, SIGNAL(visibilityChanged(pqOutputPort*, pqDataRepresentation*)), SLOT(updatePanel()));
  }
}

//-----------------------------------------------------------------------------
void pqFindDataSelectionDisplayFrame::fillCellLabels()
{
  this->Internals->fillLabels(vtkDataObject::FIELD_ASSOCIATION_CELLS);
}

//-----------------------------------------------------------------------------
void pqFindDataSelectionDisplayFrame::fillPointLabels()
{
  this->Internals->fillLabels(vtkDataObject::FIELD_ASSOCIATION_POINTS);
}

//-----------------------------------------------------------------------------
void pqFindDataSelectionDisplayFrame::cellLabelSelected(QAction* act)
{
  vtkSMInteractiveSelectionPipeline::GetInstance()->GetOrCreateSelectionRepresentation();
  this->Internals->labelBy(vtkDataObject::FIELD_ASSOCIATION_CELLS, act);
}

//-----------------------------------------------------------------------------
void pqFindDataSelectionDisplayFrame::pointLabelSelected(QAction* act)
{
  vtkSMInteractiveSelectionPipeline::GetInstance()->GetOrCreateSelectionRepresentation();
  this->Internals->labelBy(vtkDataObject::FIELD_ASSOCIATION_POINTS, act);
}

//-----------------------------------------------------------------------------
void pqFindDataSelectionDisplayFrame::showFrustum(bool val)
{
  this->Internals->showFrustum(val);
}

//-----------------------------------------------------------------------------
void pqFindDataSelectionDisplayFrame::setUseVerticalLayout(bool vertical)
{
  // we support this so that we can add this widget in a dock panel to allow
  // each access to these properties.
  if (this->useVerticalLayout() == vertical)
  {
    return;
  }

  Ui::FindDataSelectionDisplayFrame& ui = this->Internals->Ui;
  delete this->layout();
  ui.horizontalLayout = NULL;

  if (vertical)
  {
    this->Internals->Ui.interactiveSelectionColor->setVisible(true);
    this->Internals->Ui.labelPropertiesInteractiveSelection->setVisible(true);

    QVBoxLayout* vbox = new QVBoxLayout(this);
    vbox->setMargin(pqPropertiesPanel::suggestedMargin());
    vbox->setSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
    vbox->addWidget(ui.cellLabelsButton);
    vbox->addWidget(ui.pointLabelsButton);

    QHBoxLayout* hbox = new QHBoxLayout();
    hbox->addWidget(ui.selectionColor);
    hbox->addStretch();
    hbox->addWidget(ui.showFrustumButton);
    hbox->addWidget(ui.labelPropertiesSelection);
    vbox->addLayout(hbox);

    hbox = new QHBoxLayout();
    hbox->addWidget(ui.interactiveSelectionColor);
    hbox->addStretch();
    hbox->addWidget(ui.labelPropertiesInteractiveSelection);
    vbox->addLayout(hbox);
    vbox->addStretch();
  }
  else
  {
    QHBoxLayout* hbox = new QHBoxLayout(this);
    hbox->setMargin(pqPropertiesPanel::suggestedMargin());
    hbox->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
    hbox->addWidget(ui.selectionColor);
    hbox->addWidget(ui.cellLabelsButton);
    hbox->addWidget(ui.pointLabelsButton);
    hbox->addWidget(ui.showFrustumButton);
    hbox->addWidget(ui.labelPropertiesSelection);
  }
}

//-----------------------------------------------------------------------------
bool pqFindDataSelectionDisplayFrame::useVerticalLayout() const
{
  return qobject_cast<QVBoxLayout*>(this->layout()) != NULL;
}

void pqFindDataSelectionDisplayFrame::updateInteractiveSelectionLabelProperties()
{
  vtkSMProxy* selectionProxy =
    this->Internals->Port->getRepresentation(this->Internals->View)->getProxy();
  vtkSMProxy* iSelectionProxy =
    vtkSMInteractiveSelectionPipeline::GetInstance()->GetSelectionRepresentation();
  if (!selectionProxy || !iSelectionProxy)
  {
    return;
  }

  QList<QPair<QString, QString> > prop;
  prop << QPair<QString, QString>("SelectionOpacity", "Opacity")
       << QPair<QString, QString>("SelectionPointSize", "PointSize")
       << QPair<QString, QString>("SelectionLineWidth", "LineWidth")
       << QPair<QString, QString>("SelectionCellLabelBold", "CellLabelBold")
       << QPair<QString, QString>("SelectionCellLabelColor", "CellLabelColor")
       << QPair<QString, QString>("SelectionCellLabelFontFamily", "CellLabelFontFamily")
       << QPair<QString, QString>("SelectionCellLabelFontSize", "CellLabelFontSize")
       << QPair<QString, QString>("SelectionCellLabelFormat", "CellLabelFormat")
       << QPair<QString, QString>("SelectionCellLabelItalic", "CellLabelItalic")
       << QPair<QString, QString>("SelectionCellLabelJustification", "CellLabelJustification")
       << QPair<QString, QString>("SelectionCellLabelOpacity", "CellLabelOpacity")
       << QPair<QString, QString>("SelectionCellLabelShadow", "CellLabelShadow")
       << QPair<QString, QString>("SelectionPointLabelBold", "PointLabelBold")
       << QPair<QString, QString>("SelectionPointLabelColor", "PointLabelColor")
       << QPair<QString, QString>("SelectionPointLabelFontFamily", "PointLabelFontFamily")
       << QPair<QString, QString>("SelectionPointLabelFontSize", "PointLabelFontSize")
       << QPair<QString, QString>("SelectionPointLabelFormat", "PointLabelFormat")
       << QPair<QString, QString>("SelectionPointLabelItalic", "PointLabelItalic")
       << QPair<QString, QString>("SelectionPointLabelJustification", "PointLabelJustification")
       << QPair<QString, QString>("SelectionPointLabelOpacity", "PointLabelOpacity")
       << QPair<QString, QString>("SelectionPointLabelShadow", "PointLabelShadow");
  for (int i = 0; i < prop.size(); ++i)
  {
    vtkSMProperty* selectionProperty =
      selectionProxy->GetProperty(prop[i].first.toStdString().c_str());
    vtkSMProperty* iSelectionProperty =
      iSelectionProxy->GetProperty(prop[i].second.toStdString().c_str());
    iSelectionProperty->Copy(selectionProperty);
  }
}

//-----------------------------------------------------------------------------
void pqFindDataSelectionDisplayFrame::editLabelPropertiesInteractiveSelection()
{
  vtkSMProxy* proxyISelectionRepresentation =
    vtkSMInteractiveSelectionPipeline::GetInstance()->GetOrCreateSelectionRepresentation();

  QStringList properties;
  properties << "Opacity"
             << "PointSize"
             << "LineWidth"
             << "Cell Label Font"
             << "CellLabelBold"
             << "CellLabelColor"
             << "CellLabelFontFamily"
             << "CellLabelFontSize"
             << "CellLabelFormat"
             << "CellLabelItalic"
             << "CellLabelJustification"
             << "CellLabelOpacity"
             << "CellLabelShadow"
             << "Point Label Font"
             << "PointLabelBold"
             << "PointLabelColor"
             << "PointLabelFontFamily"
             << "PointLabelFontSize"
             << "PointLabelFormat"
             << "PointLabelItalic"
             << "PointLabelJustification"
             << "PointLabelOpacity"
             << "PointLabelShadow";

  BEGIN_UNDO_SET("Interactive selection label properties");
  pqProxyWidgetDialog dialog(proxyISelectionRepresentation, properties, this);
  dialog.setWindowTitle("Interactive Selection Label Properties");
  this->Internals->View->connect(&dialog, SIGNAL(accepted()), SLOT(render()));
  dialog.exec();
  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqFindDataSelectionDisplayFrame::editLabelPropertiesSelection()
{
  pqDataRepresentation* repr = this->Internals->Port->getRepresentation(this->Internals->View);

  QStringList properties;
  properties << "SelectionOpacity"
             << "SelectionPointSize"
             << "SelectionLineWidth"
             << "Cell Label Font"
             << "SelectionCellLabelBold"
             << "SelectionCellLabelColor"
             << "SelectionCellLabelFontFamily"
             << "SelectionCellLabelFontSize"
             << "SelectionCellLabelFormat"
             << "SelectionCellLabelItalic"
             << "SelectionCellLabelJustification"
             << "SelectionCellLabelOpacity"
             << "SelectionCellLabelShadow"
             << "Point Label Font"
             << "SelectionPointLabelBold"
             << "SelectionPointLabelColor"
             << "SelectionPointLabelFontFamily"
             << "SelectionPointLabelFontSize"
             << "SelectionPointLabelFormat"
             << "SelectionPointLabelItalic"
             << "SelectionPointLabelJustification"
             << "SelectionPointLabelOpacity"
             << "SelectionPointLabelShadow";

  BEGIN_UNDO_SET("Change selection display properties");
  pqProxyWidgetDialog dialog(repr->getProxy(), properties, this);
  dialog.setWindowTitle("Selection Label Properties");
  this->Internals->View->connect(&dialog, SIGNAL(accepted()), SLOT(render()));
  dialog.exec();
  this->updateInteractiveSelectionLabelProperties();
  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqFindDataSelectionDisplayFrame::onDataUpdated()
{
  // remove a label array name that does not exist anymore
  int fieldAssociations[] = { vtkDataObject::FIELD_ASSOCIATION_CELLS,
    vtkDataObject::FIELD_ASSOCIATION_POINTS };
  const char* selectionArrayNames[] = { "SelectionCellFieldDataArrayName",
    "SelectionPointFieldDataArrayName" };
  const char* selectionVisibilityNames[] = { "SelectionCellLabelVisibility",
    "SelectionPointLabelVisibility" };
  const char* iSelectionArrayNames[] = { "CellFieldDataArrayName", "PointFieldDataArrayName" };
  const char* iSelectionVisibilityNames[] = { "CellLabelVisibility", "PointLabelVisibility" };

  pqDataRepresentation* pqrepresentation =
    this->Internals->Port->getRepresentation(this->Internals->View);
  if (!pqrepresentation)
  {
    return;
  }
  vtkSMProxy* selectionRepresentation = pqrepresentation->getProxy();
  vtkSMProxy* iSelectionRepresentation =
    vtkSMInteractiveSelectionPipeline::GetInstance()->GetOrCreateSelectionRepresentation();
  for (int i = 0; i < 2; ++i)
  {
    int fieldAssociation = fieldAssociations[i];
    const char* iSelectionVisibilityName = iSelectionVisibilityNames[i];
    const char* iSelectionArrayName = iSelectionArrayNames[i];
    const char* selectionVisibilityName = selectionVisibilityNames[i];
    const char* selectionArrayName = selectionArrayNames[i];

    QString arrayName;
    if (vtkSMPropertyHelper(selectionRepresentation, selectionVisibilityName, true).GetAsInt() != 0)
    {
      arrayName = vtkSMPropertyHelper(selectionRepresentation, selectionArrayName,
                    /*quiet=*/true)
                    .GetAsString();
    }
    if (!arrayName.isEmpty())
    {
      vtkPVDataSetAttributesInformation* attrInfo =
        this->Internals->attributeInformation(fieldAssociation);
      if (!attrInfo)
      {
        return;
      }

      bool found = false;
      for (int cc = 0; cc < attrInfo->GetNumberOfArrays(); cc++)
      {
        vtkPVArrayInformation* arrayInfo = attrInfo->GetArrayInformation(cc);
        if (arrayName == arrayInfo->GetName())
        {
          found = true;
          break;
        }
      }
      if (!found)
      {
        // update selection representation
        vtkSMPropertyHelper(selectionRepresentation, selectionVisibilityName, true).Set(0);
        vtkSMPropertyHelper(selectionRepresentation, selectionArrayName, /*quiet=*/true).Set("");
        selectionRepresentation->UpdateVTKObjects();
        // update interactive selection representation
        vtkSMPropertyHelper(iSelectionRepresentation, iSelectionVisibilityName, true).Set(0);
        vtkSMPropertyHelper(iSelectionRepresentation, iSelectionArrayName, /*quiet=*/true).Set("");
        iSelectionRepresentation->UpdateVTKObjects();
      }
    }
  }
}
