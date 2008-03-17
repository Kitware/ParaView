/*=========================================================================

   Program: ParaView
   Module:    pqSelectionInspectorPanel.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#include "pqSelectionInspectorPanel.h"
#include "ui_pqSelectionInspectorPanel.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSelection.h"
#include "vtkSmartPointer.h"
#include "vtkSMClientDeliveryRepresentationProxy.h"
#include "vtkSMCompositeTreeDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkUnstructuredGrid.h"

#include <QHeaderView>
#include <QItemDelegate>
#include <QPointer>
#include <QScrollArea>
#include <QtDebug>
#include <QVBoxLayout>

#include "pqActiveView.h"
#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqPipelineRepresentation.h"
#include "pqPropertyLinks.h"
#include "pqRenderView.h"
#include "pqSelectionManager.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerSelectionModel.h"
#include "pqSignalAdaptorCompositeTreeWidget.h"
#include "pqSignalAdaptors.h"
#include "pqSignalAdaptorTreeWidget.h"
#include "pqSMAdaptor.h"
#include "pqTreeWidgetItemObject.h"



//////////////////////////////////////////////////////////////////////////////
class pqSelectionInspectorTreeItem : public pqTreeWidgetItemObject
{
public:
  pqSelectionInspectorTreeItem(QTreeWidget* _parent, const QStringList& l)
    : pqTreeWidgetItemObject(_parent, l, QTreeWidgetItem::UserType+10)
    {
    }

  virtual bool operator< ( const QTreeWidgetItem & other ) const  
    {
    int sortCol = this->treeWidget()? this->treeWidget()->sortColumn() : 0;
    double myNumber = this->text(sortCol).toDouble();
    double otherNumber = other.text(sortCol).toDouble();
    if (myNumber == otherNumber)
      {
      int numCols = this->columnCount();
      for (int cc=0; cc < numCols; cc++)
        {
        if (cc == sortCol)
          {
          continue;
          }

        double num1 = this->text(cc).toDouble();
        double num2 = other.text(cc).toDouble();
        if (num1 != num2)
          {
          return (num1 < num2);
          }
        }
      }
    return myNumber < otherNumber;
    }
};


pqTreeWidgetItemObject* pqSelectionInspectorPanelNewItem (QTreeWidget* tree, const QStringList& list)
{
 return new pqSelectionInspectorTreeItem(tree, list);
}

//////////////////////////////////////////////////////////////////////////////
// pqSelectionInspectorPanel::pqImplementation

struct pqSelectionInspectorPanel::pqImplementation : public Ui::SelectionInspectorPanel
{
public:
  pqImplementation() 
    {
    this->SelectionLinks = new pqPropertyLinks;
    this->RepLinks = new pqPropertyLinks;
    this->IndicesAdaptor = 0;
    this->GlobalIDsAdaptor = 0;
    this->SelectionSource = 0;
    this->CompositeTreeAdaptor = 0;
    // Selection Labels Properties
    this->SelectionColorAdaptor = 0;
    this->PointColorAdaptor = 0;
    this->PointFontFamilyAdaptor = 0;
    this->PointLabelAlignmentAdaptor = 0;
    this->CellColorAdaptor = 0;
    this->CellFontFamilyAdaptor = 0;
    this->CellLabelAlignmentAdaptor = 0;

    this->FieldTypeAdaptor = 0;
    this->ThresholdsAdaptor = 0;
    this->ThresholdScalarArrayAdaptor = 0;
    this->SelectionSource = 0;
    this->InputPort = 0;
    this->VTKConnectSelInput = vtkEventQtSlotConnect::New();
    this->VTKConnectRep = vtkEventQtSlotConnect::New();
    this->UpdatingGUI = false;
    }

  ~pqImplementation()
    {
    this->SelectionLinks->removeAllPropertyLinks();
    this->RepLinks->removeAllPropertyLinks();
    delete this->SelectionLinks;
    delete this->RepLinks;
    delete this->CompositeTreeAdaptor;

    delete this->SelectionColorAdaptor;
    delete this->PointColorAdaptor;
    delete this->PointFontFamilyAdaptor;
    delete this->PointLabelAlignmentAdaptor;
    delete this->CellColorAdaptor;
    delete this->CellFontFamilyAdaptor;
    delete this->CellLabelAlignmentAdaptor;
    delete this->FieldTypeAdaptor;
    delete this->ThresholdsAdaptor;
    delete this->ThresholdScalarArrayAdaptor;
    this->SelectionSource = 0;
    this->InputPort = 0;
    this->VTKConnectSelInput->Delete();
    this->VTKConnectRep->Delete();
    }

  // returns the algorithm that is producing the selection we are extracting
  // from this->InputPort.
  vtkSMSourceProxy* getSelectionSource() const
    {
    return (this->InputPort? this->InputPort->getSelectionInput() : 
      static_cast<vtkSMSourceProxy*>(NULL));
    }

  pqDataRepresentation* getSelectionRepresentation() const
    {
    if (this->InputPort && this->ActiveView)
      {
      return this->InputPort->getRepresentation(this->ActiveView);
      }
    return NULL;
    }

  QPointer<pqSelectionManager> SelectionManager;

  pqSignalAdaptorTreeWidget* IndicesAdaptor;
  pqSignalAdaptorTreeWidget* GlobalIDsAdaptor;

  QPointer<pqOutputPort> InputPort;
  // The representation whose properties are being edited.
  QPointer<pqDataRepresentation> PrevRepresentation;
  vtkSmartPointer<vtkSMSourceProxy> SelectionSource;
  QPointer<pqRenderView> ActiveView;

  // Selection Labels Properties
  vtkEventQtSlotConnect* VTKConnectSelInput;
  vtkEventQtSlotConnect* VTKConnectRep;
  pqPropertyLinks* SelectionLinks;
  pqPropertyLinks* RepLinks;

  pqSignalAdaptorColor* SelectionColorAdaptor;
  pqSignalAdaptorColor *PointColorAdaptor;
  pqSignalAdaptorComboBox *PointFontFamilyAdaptor;
  pqSignalAdaptorComboBox *PointLabelAlignmentAdaptor;

  pqSignalAdaptorColor *CellColorAdaptor;
  pqSignalAdaptorComboBox *CellFontFamilyAdaptor;
  pqSignalAdaptorComboBox *CellLabelAlignmentAdaptor;

  pqSignalAdaptorComboBox *FieldTypeAdaptor;
  pqSignalAdaptorTreeWidget* ThresholdsAdaptor;
  pqSignalAdaptorComboBox *ThresholdScalarArrayAdaptor;
  pqSignalAdaptorCompositeTreeWidget* CompositeTreeAdaptor;

  bool UseProcessID;
  bool UpdatingGUI;
};

/////////////////////////////////////////////////////////////////////////////////
// pqSelectionInspectorPanel

pqSelectionInspectorPanel::pqSelectionInspectorPanel(QWidget *p) :
  QWidget(p),
  Implementation(new pqImplementation())
{
  QVBoxLayout* vboxlayout = new QVBoxLayout(this);
  vboxlayout->setSpacing(0);
  vboxlayout->setMargin(0);
  vboxlayout->setObjectName("vboxLayout");

  QWidget* container = new QWidget(this);
  container->setObjectName("scrollWidget");
  container->setSizePolicy(QSizePolicy::MinimumExpanding,
    QSizePolicy::MinimumExpanding);

  QScrollArea* s = new QScrollArea(this);
  s->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  s->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  s->setWidgetResizable(true);
  s->setObjectName("scrollArea");
  s->setFrameShape(QFrame::NoFrame);
  s->setWidget(container);
  vboxlayout->addWidget(s);

  this->Implementation->setupUi(container);
  this->setupGUI();

  // Connect the view manager to the pqActiveView.
  QObject::connect(&pqActiveView::instance(),
    SIGNAL(changed(pqView*)),
    this, SLOT(onActiveViewChanged(pqView*)));

  this->updateEnabledState();
}

//-----------------------------------------------------------------------------
pqSelectionInspectorPanel::~pqSelectionInspectorPanel()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::setupGUI()
{
  QObject::connect(this->Implementation->comboSelectionType,
    SIGNAL(currentIndexChanged(const QString&)),
    this, SLOT(onSelectionTypeChanged(const QString&)));

  this->Implementation->FieldTypeAdaptor = new pqSignalAdaptorComboBox(
    this->Implementation->comboFieldType);

  // Updates the enable state of "Containing Cells" check box based on whether
  // we are doing a point selection or cell selection. 
  QObject::connect(this->Implementation->FieldTypeAdaptor, 
    SIGNAL(currentTextChanged(const QString&)),
    this, SLOT(onFieldTypeChanged(const QString&)), 
    Qt::QueuedConnection);

  this->setupIDSelectionGUI();
  this->setupGlobalIDSelectionGUI();
  this->setupFrustumSelectionGUI();
  this->setupThresholdSelectionGUI();
  this->setupSelectionLabelGUI();

  QObject::connect(this->Implementation->SelectionLinks, SIGNAL(qtWidgetChanged()),
    this, SLOT(updateAllSelectionViews()));
  QObject::connect(this->Implementation->RepLinks, SIGNAL(qtWidgetChanged()),
    this, SLOT(updateRepresentationViews()));

  // If user explictly wants us to create a selection for the current object,
  // then create it.
  QObject::connect(this->Implementation->createSelection, SIGNAL(clicked(bool)),
    this, SLOT(createSelectionForCurrentObject()),
    Qt::QueuedConnection);

  // Hide the tree for starters.
  this->Implementation->compositeTree->setVisible(false);
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateEnabledState()
{
  if (this->Implementation->getSelectionSource())
    {
    this->Implementation->groupActiveSelection->setEnabled(true);
    }
  else
    {
    this->Implementation->groupActiveSelection->setEnabled(false);
    }

  if (this->Implementation->getSelectionRepresentation())
    {
    this->Implementation->groupSelectionLabel->setEnabled(true);
    }
  else
    {
    this->Implementation->groupSelectionLabel->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
// Set the active server. We need the active server only to determine if this is
// a multiprocess server connection or not.
void pqSelectionInspectorPanel::setServer(pqServer* server)
{
  this->Implementation->UseProcessID =
    (server && server->getNumberOfPartitions() > 1);

  this->Implementation->ProcessIDRange->setVisible(
    this->Implementation->UseProcessID);
  if (server)
    {
    this->Implementation->ProcessIDRange->setText(
      QString("Process ID Range: 0 - %1").arg(server->getNumberOfPartitions()-1));
    }
}

//-----------------------------------------------------------------------------
/// Set the selection manager.
void pqSelectionInspectorPanel::setSelectionManager(pqSelectionManager* mgr)
{
  if (this->Implementation->SelectionManager == mgr)
    {
    return;
    }
  if (this->Implementation->SelectionManager)
    {
    QObject::disconnect(this->Implementation->SelectionManager, 0, this, 0);
    }
  this->Implementation->SelectionManager = mgr;
  if (mgr)
    {
    QObject::connect(
      mgr, SIGNAL(selectionChanged(pqOutputPort*)),
      this, SLOT(select(pqOutputPort* /*, bool*/)));
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::select(pqOutputPort* opport, bool createNew)
{
  this->Implementation->InputPort = opport;

  QString selectedObjectLabel = "<b>[none]</b>";
  if (opport)
    {
    pqPipelineSource* source = opport->getSource();
    if (source->getNumberOfOutputPorts() > 1)
      {
      selectedObjectLabel = QString("<b>%1 (%2)</b>").arg(source->getSMName()).arg(
        opport->getPortName());
      }
    else
      {
      selectedObjectLabel = QString("<b>%1</b>").arg(source->getSMName());
      }
    }
  this->Implementation->selectedObject->setText(selectedObjectLabel);

  if (createNew)
    {
    this->createNewSelectionSourceIfNeeded();
    }
  this->Implementation->UpdatingGUI = true;

  // Set up property links between the selection source and the GUI.
  this->updateSelectionGUI();

  // Set up property links between the selection representation and the GUI.
  this->updateDisplayStyleGUI();

  // update the enable state of the panel.
  this->updateEnabledState();

  // TODO: Need to determine if GlobalID selection is possible on this input and
  // disallow the user from creating one if not.

  // TODO: This needs to be changed to use domains where-ever possible.
  this->updateThreholdDataArrays();
  this->updateSelectionLabelModes();

  this->Implementation->UpdatingGUI = false;
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateSelectionGUI()
{
  // break all links between the GUI and the selection source proxy.
  this->Implementation->SelectionLinks->removeAllPropertyLinks();
  this->Implementation->Indices->clear();

  delete this->Implementation->CompositeTreeAdaptor;
  this->Implementation->CompositeTreeAdaptor = 0;

  // composite tree is only shown for "CompositeDataIDSelectionSource".
  this->Implementation->compositeTree->setVisible(false);
   
  vtkSMSourceProxy* selSource = this->Implementation->getSelectionSource();
  if (!selSource)
    {
    return;
    }

  pqSignalAdaptorTreeWidget* idsAdaptor = 0;
  const char* proxyname = selSource->GetXMLName();
  if (proxyname == QString("FrustumSelectionSource"))
    {
    this->Implementation->comboSelectionType->setCurrentIndex(2); // FRUSTUM
    }
  else if (proxyname == QString("GlobalIDSelectionSource"))
    {
    this->Implementation->comboSelectionType->setCurrentIndex(1); // Global IDs
    idsAdaptor = this->Implementation->GlobalIDsAdaptor;
    }
  else if (proxyname == QString("IDSelectionSource"))
    {
    this->Implementation->comboSelectionType->setCurrentIndex(0); // IDs.
    this->Implementation->Indices->setColumnCount(2);
    this->Implementation->Indices->setHeaderLabels(
      QStringList() << "Process ID" << "Index");
      
    this->Implementation->Indices->setColumnHidden(0,
      !this->Implementation->UseProcessID);
    this->Implementation->Indices->setColumnHidden(1, false);
    idsAdaptor = this->Implementation->IndicesAdaptor;
    // resize is needed to ensure that all columns are of minimum size possible
    // since we have messed around with column visibility.
    this->Implementation->Indices->header()->resizeSections(
      QHeaderView::ResizeToContents);
    }
  else if (proxyname == QString("CompositeDataIDSelectionSource"))
    {
    this->Implementation->comboSelectionType->setCurrentIndex(0); // IDs.
    this->Implementation->Indices->setColumnCount(3);
    this->Implementation->Indices->setHeaderLabels(
      QStringList() << "Composite ID" << "Process ID" << "Index");
      
    this->Implementation->Indices->setColumnHidden(0, false);
    this->Implementation->Indices->setColumnHidden(1,
      !this->Implementation->UseProcessID);
    this->Implementation->Indices->setColumnHidden(2, false);
    idsAdaptor = this->Implementation->IndicesAdaptor;
    this->Implementation->CompositeTreeAdaptor = 
      new pqSignalAdaptorCompositeTreeWidget(
        this->Implementation->compositeTree, 
        this->Implementation->InputPort->getOutputPortProxy(),
        vtkSMCompositeTreeDomain::NONE,
        pqSignalAdaptorCompositeTreeWidget::INDEX_MODE_FLAT);
    this->Implementation->compositeTree->setVisible(true);
    // resize is needed to ensure that all columns are of minimum size possible
    // since we have messed around with column visibility.
    this->Implementation->Indices->header()->resizeSections(
      QHeaderView::ResizeToContents);
    }
  else if (proxyname == QString("HierarchicalDataIDSelectionSource"))
    {
    this->Implementation->comboSelectionType->setCurrentIndex(0); // IDs.
    this->Implementation->Indices->setColumnCount(3);
    this->Implementation->Indices->setHeaderLabels(
      QStringList() << "Level" << "DataSet" << "Index");
      
    this->Implementation->Indices->setColumnHidden(0, false);
    this->Implementation->Indices->setColumnHidden(1, false);
    this->Implementation->Indices->setColumnHidden(2, false);
    idsAdaptor = this->Implementation->IndicesAdaptor;

    // resize is needed to ensure that all columns are of minimum size possible
    // since we have messed around with column visibility.
    this->Implementation->Indices->header()->resizeSections(
      QHeaderView::ResizeToContents);
    }
  else if (proxyname == QString("ThresholdSelectionSource"))
    {
    this->Implementation->comboSelectionType->setCurrentIndex(3); // Thresholds.
    }
  else
    {
    qDebug() << proxyname << "is not handled by the pqSelectionInspectorPanel yet.";
    return;
    }

  this->Implementation->SelectionLinks->addPropertyLink(
    this->Implementation->FieldTypeAdaptor, "currentText", 
    SIGNAL(currentTextChanged(const QString&)),
    selSource, selSource->GetProperty("FieldType"));

  this->Implementation->SelectionLinks->addPropertyLink(
    this->Implementation->checkboxContainCell, "checked", SIGNAL(toggled(bool)),
    selSource, selSource->GetProperty("ContainingCells"));

  this->Implementation->SelectionLinks->addPropertyLink(
    this->Implementation->checkboxInsideOut, "checked", SIGNAL(toggled(bool)),
    selSource, selSource->GetProperty("InsideOut"));

  if (selSource->GetProperty("IDs"))
    {
    this->Implementation->SelectionLinks->addPropertyLink(
      idsAdaptor, "values", SIGNAL(valuesChanged()),
      selSource, selSource->GetProperty("IDs"));
    }

  if (selSource->GetProperty("Thresholds"))
    {
    // Link Threshold selection properties
    this->Implementation->SelectionLinks->addPropertyLink(
      this->Implementation->ThresholdScalarArrayAdaptor, "currentText", 
      SIGNAL(currentTextChanged(const QString&)),
      selSource, selSource->GetProperty("ArrayName"));

    this->Implementation->SelectionLinks->addPropertyLink(
      this->Implementation->ThresholdsAdaptor, "values", SIGNAL(valuesChanged()),
      selSource, selSource->GetProperty("Thresholds"));
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateDisplayStyleGUI()
{
  pqDataRepresentation* selRepresentation = 
    this->Implementation->getSelectionRepresentation();

  if (this->Implementation->PrevRepresentation == selRepresentation)
    {
    return;
    }

  this->Implementation->RepLinks->removeAllPropertyLinks();
  this->Implementation->VTKConnectRep->Disconnect();
  this->Implementation->PrevRepresentation = selRepresentation;
  if (!selRepresentation)
    {
    return;
    }

  vtkSMProxy* reprProxy = selRepresentation->getProxy();

  // This updates the Combo-box for the array name based on the property value.
  this->updateSelectionPointLabelArrayName();
  this->updateSelectionCellLabelArrayName();
  this->Implementation->VTKConnectRep->Connect(
    reprProxy->GetProperty("SelectionPointFieldDataArrayName"),
    vtkCommand::ModifiedEvent, this, 
    SLOT(updateSelectionPointLabelArrayName()),
    NULL, 0.0,
    Qt::QueuedConnection);
  this->Implementation->VTKConnectRep->Connect(
    reprProxy->GetProperty("SelectionCellFieldDataArrayName"),
    vtkCommand::ModifiedEvent, this, 
    SLOT(updateSelectionCellLabelArrayName()),
    NULL, 0.0,
    Qt::QueuedConnection);

  // ---------------Selection properties------------------------
  // setup for line width and point size
  this->Implementation->RepLinks->addPropertyLink(this->Implementation->Sel_StyleLineWidth,
    "value", SIGNAL(valueChanged(double)),
    reprProxy, reprProxy->GetProperty("SelectionLineWidth"));
  this->Implementation->RepLinks->addPropertyLink(this->Implementation->Sel_StylePointSize,
    "value", SIGNAL(valueChanged(double)),
    reprProxy, reprProxy->GetProperty("SelectionPointSize"));

  // setup for opacity
  this->Implementation->RepLinks->addPropertyLink(this->Implementation->Sel_StyleOpacity,
    "value", SIGNAL(valueChanged(double)),
    reprProxy, reprProxy->GetProperty("SelectionOpacity"));
  // setup for choosing color
  this->Implementation->RepLinks->addPropertyLink(this->Implementation->SelectionColorAdaptor,
    "color", SIGNAL(colorChanged(const QVariant&)),
    reprProxy, reprProxy->GetProperty("SelectionColor"));

  // Selection Label Properties

  // Point labels properties
  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->checkBoxLabelPoints, "checked", SIGNAL(stateChanged(int)),
    reprProxy, reprProxy->GetProperty("SelectionPointLabelVisibility"));

  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->toolButtonBold_Point, "checked", SIGNAL(toggled(bool)),
    reprProxy, reprProxy->GetProperty("SelectionPointLabelBold"), 1);
  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->toolButtonItalic_Point, "checked", SIGNAL(toggled(bool)),
    reprProxy, reprProxy->GetProperty("SelectionPointLabelItalic"), 1);
  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->toolButtonShadow_Point, "checked", SIGNAL(toggled(bool)),
    reprProxy, reprProxy->GetProperty("SelectionPointLabelShadow"), 1);

  this->Implementation->RepLinks->addPropertyLink(this->Implementation->PointColorAdaptor, 
    "color", SIGNAL(colorChanged(const QVariant&)),
    reprProxy, reprProxy->GetProperty("SelectionPointLabelColor"));
  this->Implementation->RepLinks->addPropertyLink(this->Implementation->PointFontFamilyAdaptor,
    "currentText", SIGNAL(currentTextChanged(const QString&)),
    reprProxy, reprProxy->GetProperty("SelectionPointLabelFontFamily"));
  this->Implementation->RepLinks->addPropertyLink(this->Implementation->PointLabelAlignmentAdaptor,
    "currentText", SIGNAL(currentTextChanged(const QString&)),
    reprProxy, reprProxy->GetProperty("SelectionPointLabelJustification"));

  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->spinBoxSize_Point, "value", SIGNAL(valueChanged(int)),
    reprProxy, reprProxy->GetProperty("SelectionPointLabelFontSize"), 1);

  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->spinBoxOpacity_Point, "value", SIGNAL(valueChanged(double)),
    reprProxy, reprProxy->GetProperty("SelectionPointLabelOpacity"));

  // Cell Labels properties
  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->checkBoxLabelCells, "checked", SIGNAL(stateChanged(int)),
    reprProxy, reprProxy->GetProperty("SelectionCellLabelVisibility"));

  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->toolButtonBold_Cell, "checked", SIGNAL(toggled(bool)),
    reprProxy, reprProxy->GetProperty("SelectionCellLabelBold"), 1);
  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->toolButtonItalic_Cell, "checked", SIGNAL(toggled(bool)),
    reprProxy, reprProxy->GetProperty("SelectionCellLabelItalic"), 1);
  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->toolButtonShadow_Cell, "checked", SIGNAL(toggled(bool)),
    reprProxy, reprProxy->GetProperty("SelectionCellLabelShadow"), 1);

  this->Implementation->RepLinks->addPropertyLink(this->Implementation->CellColorAdaptor, 
    "color", SIGNAL(colorChanged(const QVariant&)),
    reprProxy, reprProxy->GetProperty("SelectionCellLabelColor"));
  this->Implementation->RepLinks->addPropertyLink(this->Implementation->CellFontFamilyAdaptor,
    "currentText", SIGNAL(currentTextChanged(const QString&)),
    reprProxy, reprProxy->GetProperty("SelectionCellLabelFontFamily"));
  this->Implementation->RepLinks->addPropertyLink(this->Implementation->CellLabelAlignmentAdaptor,
    "currentText", SIGNAL(currentTextChanged(const QString&)),
    reprProxy, reprProxy->GetProperty("SelectionCellLabelJustification"));

  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->spinBoxSize_Cell, "value", SIGNAL(valueChanged(int)),
    reprProxy, reprProxy->GetProperty("SelectionCellLabelFontSize"), 1);

  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->spinBoxOpacity_Cell, "value", SIGNAL(valueChanged(double)),
    reprProxy, reprProxy->GetProperty("SelectionCellLabelOpacity"));
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateThreholdDataArrays()
{
  this->Implementation->ThresholdScalarArray->clear();
  if (!this->Implementation->InputPort)
    {
    //this->Implementation->stackedWidget->
    return;
    }

  vtkPVDataInformation* geomInfo = 
    this->Implementation->InputPort->getDataInformation(true);

  vtkPVDataSetAttributesInformation* attrInfo;

  if (this->Implementation->comboFieldType->currentText() == QString("POINT"))
    {
    attrInfo = geomInfo->GetPointDataInformation();
    }
  else
    {
    attrInfo = geomInfo->GetCellDataInformation();
    }

  for(int i=0; i<attrInfo->GetNumberOfArrays(); i++)
    {
    if(attrInfo->IsArrayAnAttribute(i) == vtkDataSetAttributes::SCALARS)
      {
      this->Implementation->ThresholdScalarArray->addItem(
        attrInfo->GetArrayInformation(i)->GetName());
      }
    }
}
//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::setupSelectionLabelGUI()
{
  // Selection Labels properties
  this->Implementation->SelectionColorAdaptor = new pqSignalAdaptorColor(
    this->Implementation->Sel_buttonColor,
    "chosenColor",
    SIGNAL(chosenColorChanged(const QColor&)), false);
  this->Implementation->PointColorAdaptor = new pqSignalAdaptorColor(
    this->Implementation->buttonColor_Point, "chosenColor", 
    SIGNAL(chosenColorChanged(const QColor&)), false);
  this->Implementation->PointFontFamilyAdaptor = new pqSignalAdaptorComboBox(
    this->Implementation->comboFontFamily_Point);

  QObject::connect(this->Implementation->comboLabelMode_Point, 
    SIGNAL(currentIndexChanged(const QString&)),
    this, SLOT(updatePointLabelMode(const QString&)), Qt::QueuedConnection);
  QObject::connect(this->Implementation->comboLabelMode_Point, 
    SIGNAL(currentIndexChanged(const QString&)), 
    this, SLOT(updateRepresentationViews()), Qt::QueuedConnection);

  this->Implementation->PointLabelAlignmentAdaptor = new pqSignalAdaptorComboBox(
    this->Implementation->comboTextAlign_Point);


  this->Implementation->CellColorAdaptor = new pqSignalAdaptorColor(
    this->Implementation->buttonColor_Cell, "chosenColor", 
    SIGNAL(chosenColorChanged(const QColor&)), false);
  this->Implementation->CellFontFamilyAdaptor = new pqSignalAdaptorComboBox(
    this->Implementation->comboFontFamily_Cell);

  QObject::connect(this->Implementation->comboLabelMode_Cell, 
    SIGNAL(currentIndexChanged(const QString&)),
    this, SLOT(updateCellLabelMode(const QString&)), Qt::QueuedConnection);
  QObject::connect(this->Implementation->comboLabelMode_Cell, 
    SIGNAL(currentIndexChanged(const QString&)), 
    this, SLOT(updateRepresentationViews()), Qt::QueuedConnection);

  this->Implementation->CellLabelAlignmentAdaptor = new pqSignalAdaptorComboBox(
    this->Implementation->comboTextAlign_Cell);

}

//-----------------------------------------------------------------------------
// Called whne the SMProperty for SelectionPointFieldDataArrayName changes. We
// update the Qt combobox accordingly.
void pqSelectionInspectorPanel::updateSelectionPointLabelArrayName()
{
  pqDataRepresentation* repr = this->Implementation->getSelectionRepresentation();
  vtkSMProxy* reprProxy = repr? repr->getProxy() : NULL;
  if (!reprProxy)
    {
    return;
    }

  // Point Label
  vtkSMProperty* svp = reprProxy->
    GetProperty("SelectionPointFieldDataArrayName");
  if(!svp)
    {
    return;
    }

  QString text = pqSMAdaptor::getElementProperty(svp).toString();
  if(text.isEmpty())
    {
    return;
    }

  if(text == "vtkOriginalPointIds")
    {
    text = "Point IDs";
    }

  this->Implementation->comboLabelMode_Point->setCurrentIndex(
    this->Implementation->comboLabelMode_Point->findText(text));
} 

//-----------------------------------------------------------------------------
// Called when the SMProperty for SelectionCellFieldDataArrayName changes.
// We update the Qt combobox accordingly.
void pqSelectionInspectorPanel::updateSelectionCellLabelArrayName()
{
  pqDataRepresentation* repr = this->Implementation->getSelectionRepresentation();
  vtkSMProxy* reprProxy = repr? repr->getProxy() : NULL;
  if (!reprProxy)
    {
    return;
    }

  // Cell Label
  vtkSMProperty* svp = reprProxy->GetProperty("SelectionCellFieldDataArrayName");
  if (!svp)
    {
    return;
    }

  QString text = pqSMAdaptor::getElementProperty(svp).toString();
  if (text.isEmpty())
    {
    return;
    }

  if (text == "vtkOriginalCellIds")
    {
    text = "Cell IDs";
    }

  this->Implementation->comboLabelMode_Cell->setCurrentIndex(
    this->Implementation->comboLabelMode_Cell->findText(text));
} 

//-----------------------------------------------------------------------------
// Called when the Qt combobox for point label mode changes. We update the 
// SMProperty accordingly.
void pqSelectionInspectorPanel::updatePointLabelMode(const QString& text)
{
  if (text.isEmpty())
    {
    return;
    }

  pqDataRepresentation* repr = this->Implementation->getSelectionRepresentation();
  vtkSMProxy* reprProxy = repr? repr->getProxy() : 0;
  if (!reprProxy)
    {
    return;
    }

  if (text == "Point IDs")
    {
    pqSMAdaptor::setElementProperty(
      reprProxy->GetProperty("SelectionPointFieldDataArrayName"),"vtkOriginalPointIds");
    }
  else
    {
    pqSMAdaptor::setElementProperty(
      reprProxy->GetProperty("SelectionPointFieldDataArrayName"),text);
    }
  reprProxy->UpdateVTKObjects();
} 

//-----------------------------------------------------------------------------
// Called when the Qt combobox for cell label mode changes. We update the
// SMProperty accordingly.
void pqSelectionInspectorPanel::updateCellLabelMode(const QString& text)
{
  if(text.isEmpty())
    {
    return;
    }

  pqDataRepresentation* repr = this->Implementation->getSelectionRepresentation();
  vtkSMProxy* reprProxy = repr? repr->getProxy() : 0;
  if (!reprProxy)
    {
    return;
    }

  if(text == "Cell IDs")
    {
    pqSMAdaptor::setElementProperty(
      reprProxy->GetProperty("SelectionCellFieldDataArrayName"),"vtkOriginalCellIds");
    }
  else
    {
    pqSMAdaptor::setElementProperty(
      reprProxy->GetProperty("SelectionCellFieldDataArrayName"),text);
    }

  reprProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateSelectionLabelEnableState()
{
  if (this->Implementation->InputPort)
    {
    this->Implementation->groupSelectionLabel->setEnabled(true);
    if(this->Implementation->checkBoxLabelCells->isChecked())
      {
      this->Implementation->groupBox_CellLabelStyle->setEnabled(true);
      }
    else
      {
      this->Implementation->groupBox_CellLabelStyle->setEnabled(false);
      }
    if(this->Implementation->checkBoxLabelPoints->isChecked())
      {
      this->Implementation->groupBox_PointLabelStyle->setEnabled(true);
      }
    else
      {
      this->Implementation->groupBox_PointLabelStyle->setEnabled(false);
      }
    }
  else
    { 
    this->Implementation->groupSelectionLabel->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateSelectionLabelModes()
{
  //TODO: Fix this to use domains instead.
  if (!this->Implementation->InputPort)
    {
    return;
    }

  vtkSMSourceProxy* sourceProxy = vtkSMSourceProxy::SafeDownCast(
    this->Implementation->InputPort->getSource()->getProxy());
  vtkPVDataInformation* geomInfo = sourceProxy->GetDataInformation(
    this->Implementation->InputPort->getPortNumber());

  vtkPVDataSetAttributesInformation* attrInfo;

  this->Implementation->comboLabelMode_Point->clear();
  this->Implementation->comboLabelMode_Point->addItem("Point IDs");
  attrInfo = geomInfo->GetPointDataInformation();
  for(int i=0; i<attrInfo->GetNumberOfArrays(); i++)
    {
    QString arrayName = attrInfo->GetArrayInformation(i)->GetName();
    if(arrayName != "vtkOriginalPointIds") // "Point IDs"
      {
      this->Implementation->comboLabelMode_Point->addItem(arrayName);
      }
    }

  this->Implementation->comboLabelMode_Cell->clear();
  this->Implementation->comboLabelMode_Cell->addItem("Cell IDs");
  attrInfo = geomInfo->GetCellDataInformation();
  for(int i=0; i<attrInfo->GetNumberOfArrays(); i++)
    {
    QString arrayName = attrInfo->GetArrayInformation(i)->GetName();
    if(arrayName != "vtkOriginalCellIds") // "Cell IDs"
      {
      this->Implementation->comboLabelMode_Cell->addItem(arrayName);
      }
    }      
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::setupGlobalIDSelectionGUI()
{
  this->Implementation->GlobalIDsAdaptor =
    new pqSignalAdaptorTreeWidget(this->Implementation->GlobalIDs, true);
  this->Implementation->GlobalIDsAdaptor->setItemCreatorFunction(
    &pqSelectionInspectorPanelNewItem);


  // Link surface selection properties
  QObject::connect(this->Implementation->Delete_GlobalIDs, SIGNAL(clicked()),
    this, SLOT(deleteValue()));
  QObject::connect(this->Implementation->DeleteAll_GlobalIDs, SIGNAL(clicked()),
    this, SLOT(deleteAllValues()));
  QObject::connect(this->Implementation->NewValue_GlobalIDs, SIGNAL(clicked()),
    this, SLOT(newValue()));
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::setupIDSelectionGUI()
{
  this->Implementation->IndicesAdaptor=
    new pqSignalAdaptorTreeWidget(this->Implementation->Indices, true);

  this->Implementation->IndicesAdaptor->setItemCreatorFunction(
    &pqSelectionInspectorPanelNewItem);

  // Update the newly added items composite index to the index of the current
  // selected node, if applicable.
  QObject::connect(this->Implementation->IndicesAdaptor, 
    SIGNAL(tableGrown(pqTreeWidgetItemObject*)),
    this,
    SLOT(onTableGrown(pqTreeWidgetItemObject*)));

  // Link surface selection properties
  QObject::connect(this->Implementation->Delete, SIGNAL(clicked()),
    this, SLOT(deleteValue()));
  QObject::connect(this->Implementation->DeleteAll, SIGNAL(clicked()),
    this, SLOT(deleteAllValues()));
  QObject::connect(this->Implementation->NewValue, SIGNAL(clicked()),
    this, SLOT(newValue()));

  QObject::connect(this->Implementation->Indices,
    SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
    this, SLOT(onCurrentIndexChanged(QTreeWidgetItem*)));
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::newValue()
{
  QTreeWidget* activeTree = 0;
  pqSignalAdaptorTreeWidget* adaptor = 0;
  switch (this->Implementation->stackedWidget->currentIndex())
    {
  case 0: // IDs
    activeTree = this->Implementation->Indices;
    adaptor = this->Implementation->IndicesAdaptor;
    break;

  case 1: // GlobalIDs
    activeTree = this->Implementation->GlobalIDs;
    adaptor = this->Implementation->GlobalIDsAdaptor;
    break;

  case 3: // Thresholds
    activeTree = this->Implementation->thresholdRanges;
    adaptor = this->Implementation->ThresholdsAdaptor;
    break;

  case 2: // Frustum
  default:
    return;
    }

  adaptor->growTable();
  if (activeTree->topLevelItemCount() > 0)
    {
    QTreeWidgetItem* item = activeTree->topLevelItem(
      activeTree->topLevelItemCount()-1);
    activeTree->setCurrentItem(item);
    // edit the first visible column.
    for (int cc=0; cc < activeTree->columnCount(); cc++)
      {
      if (!activeTree->isColumnHidden(cc))
        {
        activeTree->editItem(item, cc);
        break;
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::deleteValue()
{
  QTreeWidget* activeTree = 0;
  switch (this->Implementation->stackedWidget->currentIndex())
    {
  case 0: // IDs
    activeTree = this->Implementation->Indices;
    break;

  case 1: // GlobalIDs
    activeTree = this->Implementation->GlobalIDs;
    break;

  case 3: // Thresholds
    activeTree = this->Implementation->thresholdRanges;
    break;

  case 2: // Frustum
  default:
    return;
    }

  QList<QTreeWidgetItem*> items = activeTree->selectedItems(); 
  foreach (QTreeWidgetItem* item, items)
    {
    delete item;
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::deleteAllValues()
{
  QTreeWidget* activeTree = 0;
  switch (this->Implementation->stackedWidget->currentIndex())
    {
  case 0: // IDs
    activeTree = this->Implementation->Indices;
    break;

  case 1: // GlobalIDs
    activeTree = this->Implementation->GlobalIDs;
    break;

  case 3: // Thresholds
    activeTree = this->Implementation->thresholdRanges;
    break;

  case 2: // Frustum
  default:
    return;
    }

  activeTree->clear();
}

//-----------------------------------------------------------------------------
// Updates the enable state of "Containing Cells" check box based on whether
// we are doing a point selection or cell selection. 
void pqSelectionInspectorPanel::onFieldTypeChanged(const QString& type)
{
  if(type == QString("POINT"))
    {
    this->Implementation->checkboxContainCell->setEnabled(true);
    }
  else 
    {
    this->Implementation->checkboxContainCell->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::setupFrustumSelectionGUI()
{
  // TODO: add widgets to interact with the the Frutum box 
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::setupThresholdSelectionGUI()
{
  this->Implementation->ThresholdScalarArrayAdaptor = new pqSignalAdaptorComboBox(
    this->Implementation->ThresholdScalarArray);
  this->Implementation->ThresholdsAdaptor=
    new pqSignalAdaptorTreeWidget(this->Implementation->thresholdRanges, true);

  this->Implementation->ThresholdsAdaptor->setItemCreatorFunction(
    &pqSelectionInspectorPanelNewItem);


  QObject::connect(this->Implementation->buttonAddThresholds, SIGNAL(clicked()),
    this, SLOT(newValue()));
  QObject::connect(this->Implementation->Delete_Threshold, SIGNAL(clicked()),
    this, SLOT(deleteValue()));
  QObject::connect(this->Implementation->DeleteAll_Threshold, SIGNAL(clicked()),
    this, SLOT(deleteAllValues()));
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateRepresentationViews()
{
  pqDataRepresentation* repr = this->Implementation->getSelectionRepresentation();
  if (repr)
    {
    repr->renderView(false);
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateAllSelectionViews()
{
  pqOutputPort* port = this->Implementation->InputPort;
  if (port)
    {
    port->renderAllViews();
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::onActiveViewChanged(pqView* view)
{
  pqRenderView* renView = qobject_cast<pqRenderView*>(view);
  this->Implementation->ActiveView = renView;

  // Update the "Display Style" GUI since it shows the representation in the
  // active view.
  this->updateDisplayStyleGUI();

  // Update enabled state.
  this->updateEnabledState();
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::convertSelection(bool toGIDs)
{
  vtkSMProxy* selectionSource = this->Implementation->getSelectionSource();
  if (!selectionSource)
    {
    return;
    }

  if (toGIDs) // Convert INDICES to GLOBALIDS.
    {
    QList<vtkIdType> globalIds = 
      this->Implementation->SelectionManager->getGlobalIDs();

    // Now to set gids on the IDs property, we need to insert process numbers.
    QList<QVariant> ids;
    foreach (vtkIdType gid, globalIds)
      {
      ids.push_back(gid);
      }
    pqSMAdaptor::setMultipleElementProperty(
      selectionSource->GetProperty("IDs"), ids);
    }
  else  // Convert GLOBALIDS to INDICES.
    {
    QList<QPair<int, vtkIdType> > indices =
      this->Implementation->SelectionManager->getIndices();

    QList<QVariant> ids;
    for(int cc=0; cc < indices.size(); cc++)
      {
      QPair<int, vtkIdType> pair = indices[cc];
      ids.push_back(pair.first);
      ids.push_back(pair.second);
      }
    pqSMAdaptor::setMultipleElementProperty(
      selectionSource->GetProperty("IDs"), ids);
    }

  selectionSource->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::onTableGrown(pqTreeWidgetItemObject* item)
{
  if (this->Implementation->CompositeTreeAdaptor)
    {
    bool valid = false;
    unsigned int composite_index = 
      this->Implementation->CompositeTreeAdaptor->getCurrentFlatIndex(&valid);
    if (valid)
      {
      item->setText(0, QString::number(composite_index));
      }
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::onSelectionTypeChanged(const QString& )
{
  if (this->Implementation->UpdatingGUI)
    {
    return;
    }

  // create new selection proxy based on the type that the user choose. If
  // possible try to preserve old selection properties.
  this->select(this->Implementation->InputPort, true);
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::createSelectionForCurrentObject()
{
  pqOutputPort* port = this->Implementation->currentObject->currentPort();
  if (!port)
    {
    return;
    }

  if (this->Implementation->InputPort != port && 
    this->Implementation->getSelectionSource())
    {
    // remove old selection since ParaView can only have 1 active selection at a
    // time.
    this->Implementation->InputPort->setSelectionInput(0, 0);
    }

  this->select(port, true);
}

//-----------------------------------------------------------------------------
int pqSelectionInspectorPanel::getContentType() const
{
  switch (this->Implementation->comboSelectionType->currentIndex())
    {
  case 0: // IDs
    return vtkSelection::INDICES; 

  case 1: // GlobalsIDs
    return vtkSelection::GLOBALIDS;

  case 2: // Frustum
    return vtkSelection::FRUSTUM;

  case 3: // Threshold
    return vtkSelection::THRESHOLDS;

  default:
    qDebug() << "Case not handled.";
    }

  return vtkSelection::INDICES;
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::createNewSelectionSourceIfNeeded()
{
  pqOutputPort* port = this->Implementation->InputPort;
  if (!port)
    {
    return;
    }

  int outputType = this->getContentType();

  vtkSMSourceProxy* curSelSource = this->Implementation->getSelectionSource();

  vtkSMSourceProxy* selSource = vtkSMSourceProxy::SafeDownCast(
    vtkSMSelectionHelper::ConvertSelection(outputType,
      curSelSource,
      vtkSMSourceProxy::SafeDownCast(port->getSource()->getProxy()),
      port->getPortNumber()));

  if (selSource)
    {
    if (selSource != curSelSource)
      {
      if (!selSource->GetObjectsCreated())
        {
        selSource->SetServers(vtkProcessModule::DATA_SERVER);
        selSource->SetConnectionID(port->getServer()->GetConnectionID());
        }
      selSource->UpdateVTKObjects();
      port->setSelectionInput(selSource, 0);
      }

    selSource->Delete();
    }
}

//-----------------------------------------------------------------------------
// Called when the current item in the "Indices" table changes. If composite
// tree is visible, we update the composite tree selection to match the
// current item.
void pqSelectionInspectorPanel::onCurrentIndexChanged(QTreeWidgetItem* item)
{
  if (this->Implementation->CompositeTreeAdaptor && item &&
    item->columnCount() == 3)
    {
    int cid = item->text(0).toInt();
    this->Implementation->CompositeTreeAdaptor->select(cid);
    }
}
