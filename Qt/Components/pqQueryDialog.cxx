/*=========================================================================

   Program: ParaView
   Module:    pqQueryDialog.cxx

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
#include "pqQueryDialog.h"
#include "ui_pqQueryDialog.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqPropertyLinks.h"
#include "pqQueryClauseWidget.h"
#include "pqServer.h"
#include "pqSignalAdaptors.h"
#include "pqSpreadSheetViewModel.h"
#include "pqUndoStack.h"
#include "vtkDataObject.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSmartPointer.h"
#include "vtkSMGlobalPropertiesManager.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"

#include <QList>

class pqQueryDialog::pqInternals : public Ui::pqQueryDialog
{
public:
  QList<pqQueryClauseWidget*> Clauses;
  pqSpreadSheetViewModel* DataModel;
  pqPropertyLinks Links;
  vtkSmartPointer<vtkSMViewProxy> ViewProxy;
  vtkSmartPointer<vtkSMProxy> RepresentationProxy;

  pqPropertyLinks LabelColorLinks;
  pqSignalAdaptorColor* LabelColorAdaptor;
  pqInternals()
    {
    this->DataModel = NULL;
    this->LabelColorAdaptor = NULL;
    }
};

//-----------------------------------------------------------------------------
pqQueryDialog::pqQueryDialog(
  pqOutputPort* _producer,
  QWidget* parentObject, Qt::WindowFlags _flags)
  : Superclass(parentObject, _flags)
{
  this->ExtractSelection = false;
  this->ExtractSelectionOverTime = false;

  this->Internals = new pqInternals();
  this->Internals->setupUi(this);

  this->Producer = _producer;
  // Producer cannot be NULL.
  Q_ASSERT(_producer != NULL);

  pqPipelineSource* source = _producer->getSource();
  QString fromLabel;
  if (source->getNumberOfOutputPorts() > 1)
    {
    fromLabel = QString("from <b>%1 (%2)</b>").arg(source->getSMName()).arg(
      _producer->getPortName());
    }
  else
    {
    fromLabel = QString("from <b>%1</b>").arg(source->getSMName());
    }
  this->Internals->labelFromSource->setText(fromLabel);

  // Update the GUI
  this->populateSelectionType();

  // Ensure that there's only 1 clause
  this->resetClauses();

  QObject::connect(this->Internals->selectionType,
    SIGNAL(currentIndexChanged(int)),
    this, SLOT(resetClauses()));

  QObject::connect(this->Internals->addRow,
    SIGNAL(clicked()), this, SLOT(addClause()));

  /// Currently we don't support multiple clauses.
  this->Internals->addRow->hide();

  QObject::connect(this->Internals->runQuery,
    SIGNAL(clicked()), this, SLOT(runQuery()));

  // Setup the spreadsheet view.
  this->setupSpreadSheet();

  // Link the selection color to the global selection color so that it will
  // affect all views, otherwise user may be get confused ;).
  vtkSMGlobalPropertiesManager* globalPropertiesManager =
    pqApplicationCore::instance()->getGlobalPropertiesManager();

  pqSignalAdaptorColor* adaptor = new pqSignalAdaptorColor(
    this->Internals->selectionColor, "chosenColor",
    SIGNAL(chosenColorChanged(const QColor&)), false);
  this->Internals->Links.addPropertyLink(
    adaptor,
    "color", SIGNAL(colorChanged(const QVariant&)),
    globalPropertiesManager,
    globalPropertiesManager->GetProperty("SelectionColor"));

  this->Internals->LabelColorAdaptor = new pqSignalAdaptorColor(
    this->Internals->labelColor, 
    "chosenColor",
    SIGNAL(chosenColorChanged(const QColor&)), false);

  QObject::connect(this->Internals->labels,
    SIGNAL(currentIndexChanged(int)),
    this, SLOT(setLabel(int)));

  QObject::connect(this->Internals->extractSelection,
    SIGNAL(clicked()), this, SLOT(onExtractSelection()));
  QObject::connect(this->Internals->extractSelectionOverTime,
    SIGNAL(clicked()), this, SLOT(onExtractSelectionOverTime()));

  // Render all views when any selection property is modified.
  QObject::connect(
    &this->Internals->Links, SIGNAL(qtWidgetChanged()),
    _producer, SLOT(renderAllViews()));
  QObject::connect(
    &this->Internals->LabelColorLinks, SIGNAL(qtWidgetChanged()),
    _producer, SLOT(renderAllViews()));

  vtkPVDataInformation* dataInfo = this->producer()->getDataInformation();
  if (dataInfo->GetTimeSpan()[0] >= dataInfo->GetTimeSpan()[1])
    {
    // don't show the extract selection over time option is there's not time!
    this->Internals->extractSelectionOverTime->hide();
    }

  QApplication::instance()->installEventFilter(this);
}

//-----------------------------------------------------------------------------
pqQueryDialog::~pqQueryDialog()
{
  QApplication::instance()->removeEventFilter(this);
  delete this->Internals;
  this->Internals = 0;
}

//-----------------------------------------------------------------------------
void pqQueryDialog::setupSpreadSheet()
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkIdType cid = this->Producer->getServer()->GetConnectionID();

  vtkSMProxy* repr = pxm->NewProxy("representations", "SpreadSheetRepresentation");
  repr->SetConnectionID(cid);
  // we always want to show all the blocks in the dataset, since we don't have a
  // block chooser widget in this dialog.
  vtkSMPropertyHelper(repr, "CompositeDataSetIndex").Set(0);
  vtkSMPropertyHelper(repr, "Input").Set(
    this->Producer->getSource()->getProxy(), this->Producer->getPortNumber());
  repr->UpdateVTKObjects();

  vtkSMViewProxy* view = vtkSMViewProxy::SafeDownCast(
    pxm->NewProxy("views", "SpreadSheetView"));
  view->SetConnectionID(cid);
  vtkSMPropertyHelper(view, "SelectionOnly").Set(1);
  vtkSMPropertyHelper(view, "Representations").Set(repr);
  vtkSMPropertyHelper(view, "ViewSize").Set(0, 1);
  vtkSMPropertyHelper(view, "ViewSize").Set(1, 1);
  view->UpdateVTKObjects();
  view->StillRender();;

  this->Internals->ViewProxy.TakeReference(view);
  this->Internals->RepresentationProxy.TakeReference(repr);
  this->Internals->DataModel = new pqSpreadSheetViewModel(view, this);
  this->Internals->DataModel->setActiveRepresentationProxy(repr);
  this->Internals->spreadsheet->setModel(this->Internals->DataModel);
}

#include <QInputEvent>
#include <pqCoreUtilities.h>
#include "QVTKWidget.h"
//-----------------------------------------------------------------------------
bool pqQueryDialog::eventFilter(QObject* obj, QEvent* evt)
{
  QWidget* wdg = qobject_cast<QWidget*>(obj);
  bool is_input_event = (dynamic_cast<QInputEvent*>(evt) != NULL);
  bool is_meant_for_dialog =
    (wdg == NULL || this->isAncestorOf(wdg) || this == wdg);
  bool is_render_window = (qobject_cast<QVTKWidget*>(obj));
  bool is_magical_event =
    (is_meant_for_dialog || !pqCoreUtilities::mainWidget()->isAncestorOf(wdg));

  if (is_input_event && !is_meant_for_dialog && !is_magical_event &&
    !is_render_window)
    {
    return true;
    }

  return this->Superclass::eventFilter(obj, evt);
}

//-----------------------------------------------------------------------------
void pqQueryDialog::populateSelectionType()
{
  this->Internals->selectionType->clear();
  vtkPVDataInformation* dataInfo = this->producer()->getDataInformation();
  if (dataInfo->DataSetTypeIsA("vtkGraph"))
    {
    this->Internals->selectionType->addItem("Vertex",
      vtkDataObject::VERTEX);
    this->Internals->selectionType->addItem("Edge",
      vtkDataObject::EDGE);
    }
  else if (dataInfo->DataSetTypeIsA("vtkTable"))
    {
    this->Internals->selectionType->addItem("Row",
      vtkDataObject::ROW);
    }
  else
    {
    this->Internals->selectionType->addItem("Cell",
      vtkDataObject::CELL);
    this->Internals->selectionType->addItem("Point",
      vtkDataObject::POINT);
    }
}

//-----------------------------------------------------------------------------
void pqQueryDialog::resetClauses()
{
  foreach (pqQueryClauseWidget* clause, this->Internals->Clauses)
    {
    delete clause;
    }
  this->Internals->Clauses.clear();

  delete this->Internals->queryClauseFrame->layout();
  QVBoxLayout *vbox = new QVBoxLayout(this->Internals->queryClauseFrame);
  vbox->setMargin(0);

  this->addClause();
}

//-----------------------------------------------------------------------------
void pqQueryDialog::addClause()
{
  pqQueryClauseWidget* clause = new pqQueryClauseWidget(this);
  QObject::connect(clause, SIGNAL(removeClause()),
    this, SLOT(removeClause()));
  if (this->Internals->Clauses.size() == 0)
    {
    // don't allow removal of the first clause.
    clause->setRemovable(false);
    }

  int attr_type = this->Internals->selectionType->itemData(
    this->Internals->selectionType->currentIndex()).toInt();
  clause->setProducer(this->producer());
  clause->setAttributeType(attr_type);
  clause->initialize();

  this->Internals->Clauses.push_back(clause);

  QVBoxLayout* vbox =
    qobject_cast<QVBoxLayout*>(this->Internals->queryClauseFrame->layout());
  vbox->addWidget(clause);
}

//-----------------------------------------------------------------------------
void pqQueryDialog::removeClause()
{
  pqQueryClauseWidget* clause = qobject_cast<pqQueryClauseWidget*>(
    this->sender());
  if (clause)
    {
    this->Internals->Clauses.removeAll(clause);
    delete clause;
    }
}

//-----------------------------------------------------------------------------
void pqQueryDialog::runQuery()
{
  if (this->Internals->Clauses.size() == 0)
    {
    return; // no query to run.
    }

  vtkSMProxy* selSource = this->Internals->Clauses[0]->newSelectionSource();
  if (!selSource)
    {
    return;
    }

  selSource->UpdateVTKObjects();
  this->producer()->setSelectionInput(
    vtkSMSourceProxy::SafeDownCast(selSource), 0);
  selSource->Delete();

  this->producer()->renderAllViews();

  int attr_type = this->Internals->selectionType->itemData(
    this->Internals->selectionType->currentIndex()).toInt();

  vtkSMProxy* repr = this->Internals->RepresentationProxy;
  // Pass the chosen attribute type to the spreasheet so we show cells or
  // points etc. based on what was selected.
  vtkSMPropertyHelper(repr, "FieldAssociation").Set(attr_type);
  repr->UpdateVTKObjects();
  this->Internals->ViewProxy->StillRender();

  // Once a query has been made, we enable components of the GUI that use the
  // selection
  this->Internals->selectionColor->setEnabled(true);
  this->Internals->labels->setEnabled(true);
  this->Internals->extractSelection->setEnabled(true);
  this->Internals->extractSelectionOverTime->setEnabled(true);

  // update the list of available labels.
  this->updateLabels();
  emit this->selected(this->producer());
}

//-----------------------------------------------------------------------------
namespace
{
  void pqQueryDialogAddArrays(
    QComboBox* combobox,
    vtkPVDataSetAttributesInformation* attrInfo,
    const QIcon& icon,
    QVariant data)
    {
    for (int cc=0; cc < attrInfo->GetNumberOfArrays(); cc++)
      {
      vtkPVArrayInformation* arrayInfo = attrInfo->GetArrayInformation(cc);
      // important to mark partial array since that array may be totally missing
      // from the selection ;).
      if (arrayInfo->GetIsPartial())
        {
        combobox->addItem(icon, 
          QString ("%1 (partial)").arg(arrayInfo->GetName()), data); 
        }
      else
        {
        combobox->addItem(icon, arrayInfo->GetName(), data); 
        }
      }
    }
};

//-----------------------------------------------------------------------------
void pqQueryDialog::updateLabels()
{
  // try to preserve the current label choice.
  int cur_index = this->Internals->labels->currentIndex();
  QString cur_text;
  int cur_item_data = 0;
  if (cur_index != -1)
    {
    this->Internals->labels->currentText();
    cur_item_data = this->Internals->labels->itemData(cur_index).toInt();
    }

  this->Internals->labels->blockSignals(true);
  this->Internals->labels->clear();
  this->Internals->labels->addItem("None", -1);


  int attr_type = this->Internals->selectionType->itemData(
    this->Internals->selectionType->currentIndex()).toInt();

  QIcon cellDataIcon(":/pqWidgets/Icons/pqCellData16.png");
  QIcon pointDataIcon(":/pqWidgets/Icons/pqPointData16.png");

  vtkPVDataInformation* dataInfo = this->producer()->getDataInformation();

  // Only adding cells and points for now since our labelling code doesn't
  // support vertex or edge labels.
  this->Internals->labels->addItem(pointDataIcon, "Point ID", -2);
  pqQueryDialogAddArrays(
    this->Internals->labels,
    dataInfo->GetPointDataInformation(),
    pointDataIcon,
    vtkDataObject::POINT);
  if (attr_type == vtkDataObject::CELL)
    {
    // don't add cell arrays if selection type is "Point".
    this->Internals->labels->addItem(cellDataIcon, "Cell ID", -3);
    pqQueryDialogAddArrays(
      this->Internals->labels,
      dataInfo->GetCellDataInformation(), cellDataIcon,
      vtkDataObject::CELL);
    }
  this->Internals->labels->blockSignals(false);

  if (cur_index != -1)
    {
    int new_index = this->Internals->labels->findText(cur_text);
    if (new_index != -1 &&
      this->Internals->labels->itemData(new_index).toInt() == cur_item_data)
      {
      this->Internals->labels->setCurrentIndex(new_index);
      }
    else
      {
      this->setLabel(0); // i.e. no labels.
      }
    }
}

//-----------------------------------------------------------------------------
void pqQueryDialog::setLabel(int index)
{
  // disabled when not labelling.
  this->Internals->labelColor->setEnabled(index > 0); 

  pqDataRepresentation* repr = this->producer()->getRepresentation(
    pqActiveObjects::instance().activeView());
  if (!repr)
    {
    return;
    }

  BEGIN_UNDO_SET("Label mode changed");
  vtkSMProxy* reprProxy = repr->getProxy();
  int item_data = this->Internals->labels->itemData(index).toInt();
  QString array_name = this->Internals->labels->currentText();

  if (item_data == -2  || item_data == vtkDataObject::POINT)
    {
    vtkSMPropertyHelper(reprProxy, "SelectionPointLabelVisibility", true).Set(1);
    vtkSMPropertyHelper(reprProxy, "SelectionCellLabelVisibility", true).Set(0);

    if (item_data == vtkDataObject::POINT)
      {
      vtkSMPropertyHelper(reprProxy, "SelectionPointFieldDataArrayName",
        true).Set(array_name.toAscii().data());
      }
    else
      {
      vtkSMPropertyHelper(reprProxy, "SelectionPointFieldDataArrayName",
        true).Set("vtkOriginalPointIds");
      }
    // based on whether cell or point labels are selected, we need to link the
    // "Label Color" widget with the right property.
    this->linkLabelColorWidet(reprProxy, "SelectionPointLabelColor");
    }
  else if (item_data == -3  || item_data == vtkDataObject::CELL)
    {
    vtkSMPropertyHelper(reprProxy, "SelectionPointLabelVisibility", true).Set(0);
    vtkSMPropertyHelper(reprProxy, "SelectionCellLabelVisibility", true).Set(1);

    if (item_data == vtkDataObject::CELL)
      {
      vtkSMPropertyHelper(reprProxy, "SelectionCellFieldDataArrayName",
        true).Set(array_name.toAscii().data());
      }
    else
      {
      vtkSMPropertyHelper(reprProxy, "SelectionCellFieldDataArrayName",
        true).Set("vtkOriginalCellIds");
      }
    // based on whether cell or point labels are selected, we need to link the
    // "Label Color" widget with the right property.
    this->linkLabelColorWidet(reprProxy, "SelectionCellLabelColor");
    }
  else
    {
    vtkSMPropertyHelper(reprProxy, "SelectionPointLabelVisibility", true).Set(0);
    vtkSMPropertyHelper(reprProxy, "SelectionCellLabelVisibility", true).Set(0);
    }

  reprProxy->UpdateVTKObjects();
  END_UNDO_SET();
  this->producer()->renderAllViews();
}

//-----------------------------------------------------------------------------
void pqQueryDialog::linkLabelColorWidet(vtkSMProxy* proxy, 
  const QString& propname)
{
  this->Internals->LabelColorLinks.removeAllPropertyLinks();
  this->Internals->LabelColorLinks.addPropertyLink(
    this->Internals->LabelColorAdaptor,
    "color", SIGNAL(colorChanged(const QVariant&)),
    proxy,
    proxy->GetProperty(propname.toAscii().data()));
}

