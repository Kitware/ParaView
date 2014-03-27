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
#include "vtkSmartPointer.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

#include <QMenu>
#include <QPointer>

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
  pqInternals(pqFindDataSelectionDisplayFrame* self):
    CellLabelsMenu(self),
    PointLabelsMenu(self)
    {
    this->CellLabelsMenu.setObjectName("CellLabelsMenu");
    this->PointLabelsMenu.setObjectName("PointLabelsMenu");

    this->Ui.setupUi(self);
    this->Ui.horizontalLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->Ui.horizontalLayout->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());

    this->Ui.cellLabelsButton->setMenu(&this->CellLabelsMenu);
    this->Ui.pointLabelsButton->setMenu(&this->PointLabelsMenu);

    self->connect(&this->CellLabelsMenu, SIGNAL(aboutToShow()), SLOT(fillCellLabels()));
    self->connect(&this->CellLabelsMenu, SIGNAL(triggered(QAction*)),
      SLOT(cellLabelSelected(QAction*)));

    self->connect(&this->PointLabelsMenu, SIGNAL(aboutToShow()), SLOT(fillPointLabels()));
    self->connect(&this->PointLabelsMenu, SIGNAL(triggered(QAction*)),
      SLOT(pointLabelSelected(QAction*)));
    self->connect(this->Ui.showLabelPropertiesButton, SIGNAL(clicked()),
      SLOT(editLabelProperties()));
    self->connect(this->Ui.showFrustumButton, SIGNAL(clicked(bool)),
      SLOT(showFrustum(bool)));
    }

  //---------------------------------------------------------------------------
  void updatePanel(pqFindDataSelectionDisplayFrame* self)
    {
    //this->Ui.labels->setPort(this->Port);
    try
      {
      this->Links.clear();

      if (!this->View || !this->Port ||
        !qobject_cast<pqRenderViewBase*>(this->View))
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
      this->Port->getServer()->proxyManager()->GetProxy(
        "global_properties", "ColorPalette");
    if (colorPalette)
      {
      this->Links.addPropertyLink(
        this->Ui.selectionColor, "chosenColorRgbF",
        SIGNAL(chosenColorChanged(const QColor&)),
        colorPalette, colorPalette->GetProperty("SelectionColor"));
      }
    this->showFrustum(this->Ui.showFrustumButton->isChecked());
    }

  //---------------------------------------------------------------------------
  vtkPVDataSetAttributesInformation* attributeInformation(int fieldAssociation)
    {
    return this->Port?
      this->Port->getDataInformation()->GetAttributeInformation(fieldAssociation)
      : NULL;
    }
  
  //---------------------------------------------------------------------------
  // fill the menu with available arrays. Ensure that the menu shows the
  // currently selected label field correctly.
  void fillLabels(int fieldAssociation)
    {
    QMenu& menu = fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS?
      this->CellLabelsMenu : this->PointLabelsMenu;
    menu.clear();

    vtkPVDataSetAttributesInformation* attrInfo=
      this->attributeInformation(fieldAssociation);
    if (!attrInfo)
      {
      menu.addAction("(not available)");
      return;
      }
      
    pqDataRepresentation* repr = this->Port->getRepresentation(this->View);
    Q_ASSERT(repr != NULL);

    menu.addAction("ID")->setData(
      fieldAssociation==vtkDataObject::FIELD_ASSOCIATION_CELLS?
      "vtkOriginalCellIds" : "vtkOriginalPointIds");

    for (int cc=0; cc < attrInfo->GetNumberOfArrays(); cc++)
      {
      vtkPVArrayInformation* arrayInfo = attrInfo->GetArrayInformation(cc);
      // important to mark partial array since that array may be totally missing
      // from the selection ;).
      if (arrayInfo->GetIsPartial())
        {
        QAction* action = menu.addAction(
          QString("%1 (partial)").arg(arrayInfo->GetName()));
        action->setData(arrayInfo->GetName());
        }
      else
        {
        QAction* action = menu.addAction(arrayInfo->GetName());
        action->setData(arrayInfo->GetName());
        }
      }

    QString pname = (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)?
      "SelectionCellFieldDataArrayName" : "SelectionPointFieldDataArrayName";
    QString vname = (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)?
      "SelectionCellLabelVisibility" : "SelectionPointLabelVisibility";

    QString cur_array;
    if (vtkSMPropertyHelper(repr->getProxy(), vname.toLatin1().data(), true).GetAsInt() != 0)
      {
      cur_array= vtkSMPropertyHelper(repr->getProxy(),
        pname.toLatin1().data(), /*quiet=*/true).GetAsString();
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
    Q_ASSERT(this->Port && this->View &&
      this->Port->getRepresentation(this->View));
    pqDataRepresentation* repr = this->Port->getRepresentation(this->View);
    vtkSMProxy* proxy = repr->getProxy();

    const char* pname = (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)?
      "SelectionCellFieldDataArrayName" : "SelectionPointFieldDataArrayName";
    const char* vname = (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)?
      "SelectionCellLabelVisibility" : "SelectionPointLabelVisibility";

    BEGIN_UNDO_SET("Change labels");
    if (action->isChecked())
      {
      vtkSMPropertyHelper(proxy, vname, true).Set(1);
      vtkSMPropertyHelper(proxy, pname, true).Set(action->data().toString().toLatin1().data());
      }
    else
      {
      vtkSMPropertyHelper(proxy, vname, true).Set(0);
      vtkSMPropertyHelper(proxy, pname, true).Set("");
      }
    proxy->UpdateVTKObjects();
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
        vtkSMPropertyHelper(this->FrustumView->getProxy(), "HiddenProps", true).Remove(
          this->FrustumWidget);
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
    Q_ASSERT(this->View != NULL && this->Port != NULL);

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
      this->FrustumWidget.TakeReference(
        pxm->NewProxy("representations", "FrustumWidget"));
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

    pqView* targetView = val? this->View : NULL;
    if (targetView != this->FrustumView)
      {
      if (this->FrustumView)
        {
        vtkSMPropertyHelper(this->FrustumView->getProxy(), "HiddenProps", true).Remove(
          this->FrustumWidget);
        this->FrustumView->getProxy()->UpdateVTKObjects();
        this->FrustumView->render();
        this->FrustumView = NULL;
        }

      if (targetView)
        {
        this->FrustumView = targetView;
        vtkSMPropertyHelper(
          this->FrustumView->getProxy(), "HiddenProps", true).Add(this->FrustumWidget);
        this->FrustumView->getProxy()->UpdateVTKObjects();
        this->FrustumView->render();
        }
      }

    if (targetView)
      {
      // update rendered frustum.
      vtkSMPropertyHelper frustum(selSource, "Frustum");
      double values[24];
      int index =0;
      for (int cc=0; cc < 8 ; cc++)
        {
        for (int kk=0; kk < 3; kk++)
          {
          values[index] = frustum.GetAsDouble(4*cc + kk);
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
  : Superclass(parentObject, wflags),
  Internals(new pqInternals(this))
{
  this->connect(&pqActiveObjects::instance(),
    SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->setView(pqActiveObjects::instance().activeView());

  pqSelectionManager* smgr = qobject_cast<pqSelectionManager*>(
    pqApplicationCore::instance()->manager("SELECTION_MANAGER"));
  if (smgr)
    {
    this->connect(smgr, SIGNAL(selectionChanged(pqOutputPort*)),
      SLOT(setSelectedPort(pqOutputPort*)));
    this->setSelectedPort(smgr->getSelectedPort());
    }
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
  this->Internals->View = view;
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
    this->connect(port,
      SIGNAL(visibilityChanged(pqOutputPort*, pqDataRepresentation*)),
      SLOT(updatePanel()));
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
  this->Internals->labelBy(vtkDataObject::FIELD_ASSOCIATION_CELLS, act);
}

//-----------------------------------------------------------------------------
void pqFindDataSelectionDisplayFrame::pointLabelSelected(QAction* act)
{
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

  Ui::FindDataSelectionDisplayFrame &ui = this->Internals->Ui;
  delete this->layout();
  ui.horizontalLayout = NULL;

  if (vertical)
    {
    QVBoxLayout* vbox = new QVBoxLayout(this);
    vbox->setMargin(pqPropertiesPanel::suggestedMargin());
    vbox->setSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
    vbox->addWidget(ui.cellLabelsButton);
    vbox->addWidget(ui.pointLabelsButton);

    QHBoxLayout* hbox = new QHBoxLayout();
    hbox->addWidget(ui.selectionColor);
    hbox->addStretch();
    hbox->addWidget(ui.showFrustumButton);
    hbox->addWidget(ui.showLabelPropertiesButton);

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
    hbox->addWidget(ui.showLabelPropertiesButton);
    }
}

//-----------------------------------------------------------------------------
bool pqFindDataSelectionDisplayFrame::useVerticalLayout() const
{
  return qobject_cast<QVBoxLayout*>(this->layout()) != NULL;
}

//-----------------------------------------------------------------------------
void pqFindDataSelectionDisplayFrame::editLabelProperties()
{
  pqDataRepresentation* repr = this->Internals->Port->getRepresentation(
    this->Internals->View);

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
  dialog.setWindowTitle("Advanced Selection Display Properties");
  this->Internals->View->connect(&dialog, SIGNAL(accepted()), SLOT(render()));
  dialog.exec();
  END_UNDO_SET();
}
