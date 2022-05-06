/*=========================================================================

   Program: ParaView
   Module:  pqFindDataSelectionDisplayFrame.cxx

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
#include "pqDataRepresentation.h"
#include "pqOutputPort.h"
#include "pqPropertiesPanel.h"
#include "pqPropertyLinks.h"
#include "pqProxyWidgetDialog.h"
#include "pqRenderViewBase.h"
#include "pqSignalAdaptors.h"
#include "pqUndoStack.h"

#include "vtkDataObject.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMInteractiveSelectionPipeline.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTrace.h"
#include "vtkSmartPointer.h"

#include <QMenu>
#include <QPointer>

#include <cassert>

class pqFindDataSelectionDisplayFrame::pqInternals
{
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
    this->Ui.mainLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->Ui.mainLayout->setSpacing(pqPropertiesPanel::suggestedVerticalSpacing());

    this->Ui.cellLabelsButton->setMenu(&this->CellLabelsMenu);
    this->Ui.pointLabelsButton->setMenu(&this->PointLabelsMenu);

    QObject::connect(&this->CellLabelsMenu, &QMenu::aboutToShow,
      [this]() { this->fillLabels(vtkDataObject::FIELD_ASSOCIATION_CELLS); });
    QObject::connect(&this->CellLabelsMenu, &QMenu::triggered, [this](QAction* actn) {
      vtkSMInteractiveSelectionPipeline::GetInstance()->GetOrCreateSelectionRepresentation();
      this->labelBy(vtkDataObject::FIELD_ASSOCIATION_CELLS, actn);
    });

    QObject::connect(&this->PointLabelsMenu, &QMenu::aboutToShow,
      [this]() { this->fillLabels(vtkDataObject::FIELD_ASSOCIATION_POINTS); });
    QObject::connect(&this->PointLabelsMenu, &QMenu::triggered, [this](QAction* actn) {
      vtkSMInteractiveSelectionPipeline::GetInstance()->GetOrCreateSelectionRepresentation();
      this->labelBy(vtkDataObject::FIELD_ASSOCIATION_POINTS, actn);
    });

    self->connect(
      this->Ui.labelPropertiesSelection, SIGNAL(clicked()), SLOT(editLabelPropertiesSelection()));
    self->connect(this->Ui.labelPropertiesInteractiveSelection, SIGNAL(clicked()),
      SLOT(editLabelPropertiesInteractiveSelection()));
  }

  //---------------------------------------------------------------------------
  void updatePanel(pqFindDataSelectionDisplayFrame* self)
  {
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
      return;
    }

    self->setEnabled(true);

    // Link the selection color to the global selection color so that it will
    // affect all views, otherwise the user may be get confused.
    vtkSMProxy* colorPalette =
      this->Port->getServer()->proxyManager()->GetProxy("settings", "ColorPalette");
    if (colorPalette)
    {
      this->Links.addPropertyLink(this->Ui.selectionColor, "chosenColorRgbF",
        SIGNAL(chosenColorChanged(const QColor&)), colorPalette,
        colorPalette->GetProperty("SelectionColor"));
      this->Links.addPropertyLink(this->Ui.interactiveSelectionColor, "chosenColorRgbF",
        SIGNAL(chosenColorChanged(const QColor&)), colorPalette,
        colorPalette->GetProperty("InteractiveSelectionColor"));
    }
  }

  //---------------------------------------------------------------------------
  vtkPVDataSetAttributesInformation* attributeInformation(int fieldAssociation)
  {
    return this->Port ? this->Port->getDataInformation()->GetAttributeInformation(fieldAssociation)
                      : nullptr;
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
    assert(repr != nullptr);

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

    QString arrayName;
    if (vtkSMPropertyHelper(repr->getProxy(), vname.toUtf8().data(), true).GetAsInt() != 0)
    {
      arrayName =
        vtkSMPropertyHelper(repr->getProxy(), pname.toUtf8().data(), /*quiet=*/true).GetAsString();
    }

    for (QAction* action : menu.actions())
    {
      action->setCheckable(true);
      if (arrayName.isEmpty() == false && action->data().toString() == arrayName)
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
    {
      SM_SCOPED_TRACE(PropertiesModified).arg("proxy", selectionProxy);
      if (action->isChecked())
      {
        // selection
        vtkSMPropertyHelper(selectionProxy, selectionVisibilityName, true).Set(1);
        vtkSMPropertyHelper(selectionProxy, selectionArrayName, true)
          .Set(action->data().toString().toUtf8().data());
        // interactive selection
        if (iSelectionProxy)
        {
          vtkSMPropertyHelper(iSelectionProxy, iSelectionVisibilityName, true).Set(1);
          vtkSMPropertyHelper(iSelectionProxy, iSelectionArrayName, true)
            .Set(action->data().toString().toUtf8().data());
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
    }
    selectionProxy->UpdateVTKObjects();
    if (iSelectionProxy)
    {
      iSelectionProxy->UpdateVTKObjects();
    }
    END_UNDO_SET();

    this->View->render();
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
  this->Internals = nullptr;
}

//-----------------------------------------------------------------------------
void pqFindDataSelectionDisplayFrame::setView(pqView* view)
{
  if (view != this->Internals->View)
  {
    this->Internals->View = view;
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

  QList<QPair<QString, QString>> prop;
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
  auto& internals = (*this->Internals);
  auto updateLabels = [&internals](
                        vtkSMProxy* repr, int fieldAssociation, const std::string& prefix) {
    const auto arrayNameProperty = prefix +
      (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS ? "Cell" : "Point") +
      "FieldDataArrayName";
    const auto visibilityProperty = prefix +
      (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS ? "Cell" : "Point") +
      "LabelVisibility";

    vtkSMPropertyHelper visibilityHelper(repr, visibilityProperty.c_str(), /*quiet=*/true);
    if (visibilityHelper.GetAsInt() == 0)
    {
      return;
    }

    vtkSMPropertyHelper arrayNameHelper(repr, arrayNameProperty.c_str(), /*quiet=*/true);
    const std::string arrayName =
      arrayNameHelper.GetAsString() ? arrayNameHelper.GetAsString() : "";
    if (!arrayName.empty())
    {
      // note, these are not part of the input data and hence will never be
      // present in the data information, but are always available after
      // "extract selection".
      if ((fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS &&
            arrayName == "vtkOriginalCellIds") ||
        (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS &&
          arrayName == "vtkOriginalPointIds"))
      {
        return;
      }

      auto dsaInfo = internals.attributeInformation(fieldAssociation);
      if (dsaInfo && dsaInfo->GetArrayInformation(arrayName.c_str()) != nullptr)
      {
        // all's well. no need to stop showing these labels.
        return;
      }
    }

    // hide the labels. they don't exist anymore.
    visibilityHelper.Set(0);
    arrayNameHelper.Set("");
    repr->UpdateVTKObjects();
  };

  if (auto selectionRepresentation = internals.Port->getRepresentation(internals.View))
  {
    updateLabels(
      selectionRepresentation->getProxy(), vtkDataObject::FIELD_ASSOCIATION_CELLS, "Selection");
    updateLabels(
      selectionRepresentation->getProxy(), vtkDataObject::FIELD_ASSOCIATION_POINTS, "Selection");
  }

  if (auto iSelectionRepresentation =
        vtkSMInteractiveSelectionPipeline::GetInstance()->GetSelectionRepresentation())
  {
    updateLabels(iSelectionRepresentation, vtkDataObject::FIELD_ASSOCIATION_CELLS, {});
    updateLabels(iSelectionRepresentation, vtkDataObject::FIELD_ASSOCIATION_POINTS, {});
  }
}
