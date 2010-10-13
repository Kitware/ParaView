/*=========================================================================

   Program: ParaView
   Module:    pqSelectionInspectorPanel.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#include "pqSelectionInspectorPanel.h"
#include "ui_pqSelectionInspectorPanel.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkSMCompositeTreeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMGlobalPropertiesManager.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkUnstructuredGrid.h"

#include <QHeaderView>
#include <QItemDelegate>
#include <QMessageBox>
#include <QPointer>
#include <QScrollArea>
#include <QtDebug>
#include <QTimer>
#include <QVBoxLayout>

#include "pq3DWidgetFactory.h"
#include "pqActiveView.h"
#include "pqApplicationCore.h"
#include "pqComboBoxDomain.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqPipelineRepresentation.h"
#include "pqPropertyLinks.h"
#include "pqRenderView.h"
#include "pqSelectionManager.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerSelectionModel.h"
#include "pqSettings.h"
#include "pqSignalAdaptorCompositeTreeWidget.h"
#include "pqSignalAdaptors.h"
#include "pqSignalAdaptorTreeWidget.h"
#include "pqSMAdaptor.h"
#include "pqTreeWidgetSelectionHelper.h"

//////////////////////////////////////////////////////////////////////////////
class pqSelectionInspectorTreeItem : public QTreeWidgetItem
{
public:
  pqSelectionInspectorTreeItem(QTreeWidget* _parent, const QStringList& l)
    : QTreeWidgetItem(_parent, l, QTreeWidgetItem::UserType+10)
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


QTreeWidgetItem* pqSelectionInspectorPanelNewItem (QTreeWidget* tree, const QStringList& list)
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
    this->BlocksAdaptor = 0;
    this->IndicesAdaptor = 0;
    this->GlobalIDsAdaptor = 0;
    this->LocationsAdaptor = 0;
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
    this->InputPort = 0;
    this->VTKConnectSelInput = vtkEventQtSlotConnect::New();
    this->VTKConnectRep = vtkEventQtSlotConnect::New();
    this->UpdatingGUI = false;

    this->PointLabelArrayDomain = 0;
    this->CellLabelArrayDomain = 0;
    }

  ~pqImplementation()
    {
    this->SelectionLinks->removeAllPropertyLinks();
    this->RepLinks->removeAllPropertyLinks();
    delete this->SelectionLinks;
    delete this->RepLinks;
    delete this->CompositeTreeAdaptor;
    delete this->LocationsAdaptor;
    delete this->BlocksAdaptor;

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
    this->InputPort = 0;
    this->VTKConnectSelInput->Delete();
    this->VTKConnectRep->Delete();

    delete this->PointLabelArrayDomain;
    delete this->CellLabelArrayDomain;
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
  pqSignalAdaptorTreeWidget* LocationsAdaptor;
  pqSignalAdaptorCompositeTreeWidget* BlocksAdaptor;

  QPointer<pqOutputPort> InputPort;
  // The representation whose properties are being edited.
  QPointer<pqDataRepresentation> PrevRepresentation;
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

  pqComboBoxDomain* PointLabelArrayDomain;
  pqComboBoxDomain* CellLabelArrayDomain;

  bool UseProcessID;
  bool UpdatingGUI;

  QList<vtkSmartPointer<vtkSMNewWidgetRepresentationProxy> > LocationWigets;
  vtkSmartPointer<vtkSMProxy> FrustumWidget;

  enum
    {
    IDS = 0,
    FRUSTUM = 1,
    LOCATIONS = 2,
    THRESHOLDS = 3,
    BLOCKS = 4,
    QUERY = 5,
    GLOBALIDS = 6 // GLOBALIDS has to be last
    };

  static const char* getText(int val, int type)
    {
      //type == 0 -> CELL, type == 1 -> POINT
    switch (val)
      {
    case IDS:
      return "IDs";

    case GLOBALIDS:
      if (type == 0)
        {
        return "Global Element IDs";
        }
      else
        {
        return "Global Node IDs";
        }

    case FRUSTUM:
      return "Frustum";

    case LOCATIONS:
      return "Locations";

    case THRESHOLDS:
      return "Thresholds";

    case BLOCKS:
      return "Blocks";

    case QUERY:
      return "Query";
      }
    return "Unknown";
    }
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
  this->setupLocationsSelectionGUI();
  this->setupThresholdSelectionGUI();
  this->setupBlockSelectionGUI();
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
      this, SLOT(onSelectionManagerChanged(pqOutputPort*)));
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::select(pqOutputPort* opport, bool createNew)
{
  if (this->Implementation->InputPort)
    {
    QObject::disconnect(this->Implementation->InputPort->getSource(), 0, this, 0);
    }


  this->Implementation->InputPort = opport;
  this->updateSelectionTypesAvailable();

  bool hasGIDs = this->hasGlobalIDs(opport);
  if (createNew && hasGIDs)
    {
    this->Implementation->comboSelectionType->setCurrentIndex(
      pqImplementation::GLOBALIDS);
    }

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

  // Updates the 3D widgets used.
  this->updateLocationWidgets();

  // update the frustum widget.
  this->updateFrustum();

  // Set up property links between the selection representation and the GUI.
  this->updateDisplayStyleGUI();

  this->updateEnabledState();

  if (opport)
    {
    // Need to determine if GlobalID selection is possible
  // update the enable state of the panel. on this input and
    // disallow the user from creating one if not.
    this->updateSelectionTypesAvailable();
    QObject::connect(opport->getSource(), SIGNAL(dataUpdated(pqPipelineSource*)),
      this, SLOT(updateSelectionTypesAvailable()), Qt::QueuedConnection);
    }

  // TODO: This needs to be changed to use domains where-ever possible.
  this->updateThreholdDataArrays();

  this->Implementation->UpdatingGUI = false;

  if ( opport && hasGIDs && opport->getSelectionInput() &&
       opport->getSelectionInput()->GetXMLName() ==
       QString("CompositeDataIDSelectionSource"))
       {
       this->Implementation->comboSelectionType->setCurrentIndex(
         pqImplementation::GLOBALIDS);
       return;
       }

  if (createNew)
    {
    this->Implementation->SelectionManager->select(opport);
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateSelectionGUI()
{
  // break all links between the GUI and the selection source proxy.
  this->Implementation->SelectionLinks->removeAllPropertyLinks();
  this->Implementation->Indices->clear();

  delete this->Implementation->CompositeTreeAdaptor;
  this->Implementation->CompositeTreeAdaptor = 0;

  delete this->Implementation->BlocksAdaptor;
  this->Implementation->BlocksAdaptor = 0;

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
    this->Implementation->comboSelectionType->setCurrentIndex(
      pqImplementation::FRUSTUM); // FRUSTUM
    }
  else if (proxyname == QString("GlobalIDSelectionSource"))
    {
    this->Implementation->comboSelectionType->setCurrentIndex(
      pqImplementation::GLOBALIDS); // Global IDs
    idsAdaptor = this->Implementation->GlobalIDsAdaptor;
    }
  else if (proxyname == QString("IDSelectionSource"))
    {
    this->Implementation->comboSelectionType->setCurrentIndex(
      pqImplementation::IDS); // IDs.
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
    this->Implementation->comboSelectionType->setCurrentIndex(
      pqImplementation::IDS); // IDs.
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
    this->Implementation->comboSelectionType->setCurrentIndex(
      pqImplementation::IDS); // IDs.
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
    this->Implementation->comboSelectionType->setCurrentIndex(
      pqImplementation::THRESHOLDS); // Thresholds.
    }
  else if (proxyname == QString("LocationSelectionSource"))
    {
    this->Implementation->comboSelectionType->setCurrentIndex(
      pqImplementation::LOCATIONS);
    idsAdaptor = this->Implementation->LocationsAdaptor;
    }
  else if (proxyname == QString("BlockSelectionSource"))
    {
    this->Implementation->comboSelectionType->setCurrentIndex(
      pqImplementation::BLOCKS);
    this->Implementation->BlocksAdaptor =
      new pqSignalAdaptorCompositeTreeWidget(
        this->Implementation->Blocks,
        this->Implementation->InputPort->getOutputPortProxy(),
        vtkSMCompositeTreeDomain::LEAVES,
        pqSignalAdaptorCompositeTreeWidget::INDEX_MODE_FLAT,
        /*selectMultiple=*/true,
        /*autoUpdateVisibility=*/false);
    }
  else if (proxyname == QString("SelectionQuerySource"))
    {
    this->Implementation->comboSelectionType->setCurrentIndex(
      pqImplementation::QUERY); // Query.
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

  if (selSource->GetProperty("Locations"))
    {
    this->Implementation->SelectionLinks->addPropertyLink(
      idsAdaptor, "values", SIGNAL(valuesChanged()),
      selSource, selSource->GetProperty("Locations"));
    }

  if (selSource->GetProperty("Blocks"))
    {
    this->Implementation->SelectionLinks->addPropertyLink(
      this->Implementation->BlocksAdaptor, "values",
      SIGNAL(valuesChanged()),
      selSource, selSource->GetProperty("Blocks"));
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

  if (selSource->GetProperty("UserFriendlyText"))
    {
    selSource->UpdatePropertyInformation();
    this->Implementation->SelectionLinks->addPropertyLink(
      this->Implementation->queryText, "plainText",
      SIGNAL(textChanged()),
      selSource, selSource->GetProperty("UserFriendlyText"));
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

  delete this->Implementation->PointLabelArrayDomain;
  delete this->Implementation->CellLabelArrayDomain;
  this->Implementation->PointLabelArrayDomain = 0;
  this->Implementation->CellLabelArrayDomain = 0;
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
  // Note: we are linking to the global property for selection color here, so
  // that all selection colors are affected instead of simply the current
  // objects selection color. BUG #6816.
  vtkSMGlobalPropertiesManager* gpm =
    pqApplicationCore::instance()->getGlobalPropertiesManager();
  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->SelectionColorAdaptor,
    "color", SIGNAL(colorChanged(const QVariant&)),
    gpm, gpm->GetProperty("SelectionColor"));
  // We also need to save the color change in the settings so that it's
  // preserved across sessions.
  QObject::connect(
    this->Implementation->Sel_buttonColor,
    SIGNAL(chosenColorChanged(const QColor&)),
    this, SLOT(onSelectionColorChanged(const QColor&)));

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


  bool prev = this->Implementation->comboLabelMode_Point->blockSignals(true);
  // the domain is updated only when the labels are visible for the first time.
  this->Implementation->PointLabelArrayDomain = new pqComboBoxDomain(
    this->Implementation->comboLabelMode_Point,
    reprProxy->GetProperty("SelectionPointFieldDataArrayName"));
  this->Implementation->PointLabelArrayDomain->addString("Point IDs");
  this->updateSelectionPointLabelArrayName();
  this->Implementation->comboLabelMode_Point->blockSignals(prev);

  prev = this->Implementation->comboLabelMode_Cell->blockSignals(true);
  // the domain is updated only when the labels are visible for the first time.
  this->Implementation->CellLabelArrayDomain = new pqComboBoxDomain(
    this->Implementation->comboLabelMode_Cell,
    reprProxy->GetProperty("SelectionCellFieldDataArrayName"));
  this->Implementation->CellLabelArrayDomain->addString("Cell IDs");
  this->updateSelectionCellLabelArrayName();
  this->Implementation->comboLabelMode_Cell->blockSignals(prev);
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateThreholdDataArrays()
{
  this->Implementation->ThresholdScalarArray->clear();
  if (!this->Implementation->InputPort)
    {
    //this->Implementation->itemsStackedWidget->
    return;
    }

  vtkPVDataInformation* geomInfo =
    this->Implementation->InputPort->getDataInformation();

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
    vtkPVArrayInformation* info = attrInfo->GetArrayInformation(i);
    if (info->GetNumberOfComponents() == 1)
      {
      this->Implementation->ThresholdScalarArray->addItem(
        info->GetName());
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
    this, SLOT(updatePointLabelMode(const QString&)));
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
void pqSelectionInspectorPanel::setupLocationsSelectionGUI()
{
  this->Implementation->LocationsAdaptor =
    new pqSignalAdaptorTreeWidget(this->Implementation->Locations, true);
  this->Implementation->LocationsAdaptor->setItemCreatorFunction(
    &pqSelectionInspectorPanelNewItem);

  QObject::connect(this->Implementation->Delete_Locations, SIGNAL(clicked()),
    this, SLOT(deleteValue()));
  QObject::connect(this->Implementation->DeleteAll_Locations, SIGNAL(clicked()),
    this, SLOT(deleteAllValues()));
  QObject::connect(this->Implementation->NewValue_Locations, SIGNAL(clicked()),
    this, SLOT(newValue()));

  QObject::connect(this->Implementation->Delete_Locations, SIGNAL(clicked()),
    this, SLOT(updateLocationWidgets()), Qt::QueuedConnection);
  QObject::connect(this->Implementation->DeleteAll_Locations, SIGNAL(clicked()),
    this, SLOT(updateLocationWidgets()), Qt::QueuedConnection);
  QObject::connect(this->Implementation->NewValue_Locations, SIGNAL(clicked()),
    this, SLOT(updateLocationWidgets()), Qt::QueuedConnection);

  QObject::connect(this->Implementation->showLocationWidgets, SIGNAL(toggled(bool)),
    this, SLOT(updateLocationWidgets()), Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::setupBlockSelectionGUI()
{
  // Enable group selection on the widget.
  new pqTreeWidgetSelectionHelper(this->Implementation->Blocks);
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::setupGlobalIDSelectionGUI()
{
  this->Implementation->GlobalIDsAdaptor =
    new pqSignalAdaptorTreeWidget(this->Implementation->GlobalIDs, true);
  this->Implementation->GlobalIDsAdaptor->setItemCreatorFunction(
    &pqSelectionInspectorPanelNewItem);

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
    SIGNAL(tableGrown(QTreeWidgetItem*)),
    this, SLOT(onTableGrown(QTreeWidgetItem*)));

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
  switch (this->Implementation->itemsStackedWidget->currentIndex())
    {
  case pqImplementation::IDS: // IDs
    activeTree = this->Implementation->Indices;
    adaptor = this->Implementation->IndicesAdaptor;
    break;

  case pqImplementation::GLOBALIDS: // GlobalIDs
    activeTree = this->Implementation->GlobalIDs;
    adaptor = this->Implementation->GlobalIDsAdaptor;
    break;

  case pqImplementation::THRESHOLDS: // Thresholds
    activeTree = this->Implementation->thresholdRanges;
    adaptor = this->Implementation->ThresholdsAdaptor;
    break;

  case pqImplementation::LOCATIONS:
    activeTree = this->Implementation->Locations;
    adaptor = this->Implementation->LocationsAdaptor;
    break;

  case pqImplementation::BLOCKS:
  case pqImplementation::FRUSTUM:
  default:
    // don't support newValue().
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
  switch (this->Implementation->itemsStackedWidget->currentIndex())
    {
  case pqImplementation::IDS: // IDs
    activeTree = this->Implementation->Indices;
    break;

  case pqImplementation::GLOBALIDS: // GlobalIDs
    activeTree = this->Implementation->GlobalIDs;
    break;

  case pqImplementation::THRESHOLDS: // Thresholds
    activeTree = this->Implementation->thresholdRanges;
    break;

  case pqImplementation::LOCATIONS:
    activeTree = this->Implementation->Locations;
    break;

  case pqImplementation::BLOCKS:
  case pqImplementation::FRUSTUM: // Frustum
  default:
    // not supported.
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
  switch (this->Implementation->itemsStackedWidget->currentIndex())
    {
  case pqImplementation::IDS: // IDs
    activeTree = this->Implementation->Indices;
    break;

  case pqImplementation::GLOBALIDS: // GlobalIDs
    activeTree = this->Implementation->GlobalIDs;
    break;

  case pqImplementation::THRESHOLDS: // Thresholds
    activeTree = this->Implementation->thresholdRanges;
    break;

  case pqImplementation::LOCATIONS:
    activeTree = this->Implementation->Locations;
    break;

  case pqImplementation::BLOCKS:
  case pqImplementation::FRUSTUM: // Frustum
  default:
    // Not supported.
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
  this->updateSelectionTypesAvailable();
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::setupFrustumSelectionGUI()
{
  // TODO: add widgets to interact with the the Frutum box
  QObject::connect(this->Implementation->checkboxShowFrustum,
    SIGNAL(toggled(bool)),
    this, SLOT(updateFrustum()), Qt::QueuedConnection);
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
  // remove frustum widget from the old view.
  this->updateFrustumInternal(false);

  pqRenderView* renView = qobject_cast<pqRenderView*>(view);
  this->Implementation->ActiveView = renView;

  // update the frustum widget.
  QTimer::singleShot(10, this, SLOT(updateFrustum()));

  // Update the "Display Style" GUI since it shows the representation in the
  // active view.
  this->updateDisplayStyleGUI();

  // Update enabled state.
  this->updateEnabledState();
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::onTableGrown(QTreeWidgetItem* item)
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
void pqSelectionInspectorPanel::onSelectionTypeChanged(const QString&)
{
  if (this->Implementation->UpdatingGUI)
    {
    return;
    }

  // create new selection proxy based on the type that the user choose. If
  // possible try to preserve old selection properties.
  this->select(this->Implementation->InputPort, true);
  if (this->Implementation->InputPort)
    {
    this->Implementation->InputPort->renderAllViews();
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::onSelectionManagerChanged(pqOutputPort* opport)
{
  this->select(opport, false);
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
  port->renderAllViews();
}

//-----------------------------------------------------------------------------
int pqSelectionInspectorPanel::getContentType() const
{
  switch (this->Implementation->itemsStackedWidget->currentIndex())
    {
  case pqImplementation::IDS: // IDs
    return vtkSelectionNode::INDICES;

  case pqImplementation::GLOBALIDS: // GlobalsIDs
    return vtkSelectionNode::GLOBALIDS;

  case pqImplementation::FRUSTUM: // Frustum
    return vtkSelectionNode::FRUSTUM;

  case pqImplementation::THRESHOLDS: // Threshold
    return vtkSelectionNode::THRESHOLDS;

  case pqImplementation::LOCATIONS:
    return vtkSelectionNode::LOCATIONS;

  case pqImplementation::BLOCKS:
    return vtkSelectionNode::BLOCKS;

  default:
    qDebug() << "Case not handled.";
    }

  return vtkSelectionNode::INDICES;
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

  if (curSelSource &&
    port->getServer()->isRemote() &&
    (outputType == vtkSelectionNode::INDICES || outputType == vtkSelectionNode::GLOBALIDS))
    {
    // BUG: 6783. Warn user when converting a Frustum|Threshold selection to
    // an id based selection.
    if (strcmp(curSelSource->GetXMLName(), "FrustumSelectionSource") == 0 ||
      strcmp(curSelSource->GetXMLName(), "ThresholdSelectionSource") == 0)
      {
      // We need to determine how many ids are present approximately.
      vtkSMSourceProxy* sourceProxy =
        vtkSMSourceProxy::SafeDownCast(port->getSource()->getProxy());
      vtkPVDataInformation* selectedInformation = sourceProxy->GetSelectionOutput(
        port->getPortNumber())->GetDataInformation();
      int fdType = pqSMAdaptor::getElementProperty(
        curSelSource->GetProperty("FieldType")).toInt();
      if ((fdType == vtkSelectionNode::POINT &&
          selectedInformation->GetNumberOfPoints() > 10000) ||
        (fdType == vtkSelectionNode::CELL &&
         selectedInformation->GetNumberOfCells() > 10000))
        {
        if (QMessageBox::warning(this, tr("Convert Selection"),
            tr("This selection converion can potentially result in fetching a "
              "large amount of data to the client.\n"
              "Are you sure you want to continue?"),
            QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) !=
          QMessageBox::Ok)
          {
          curSelSource = 0;
          }
        }
      }
    }

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

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::removeWidgetsFromView()
{
  if (!this->Implementation->ActiveView)
    {
    return;
    }

  vtkSMRenderViewProxy* view =
    this->Implementation->ActiveView->getRenderViewProxy();

  foreach (vtkSMNewWidgetRepresentationProxy* widget,
    this->Implementation->LocationWigets)
    {
    pqSMAdaptor::setElementProperty(widget->GetProperty("Enabled"), 0);
    widget->UpdateVTKObjects();
    vtkSMPropertyHelper(view, "HiddenRepresentations").Remove(widget);
    view->UpdateVTKObjects();
    }
  this->Implementation->ActiveView->render();
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::addWidgetsToView()
{
  if (!this->Implementation->ActiveView)
    {
    return;
    }

  vtkSMRenderViewProxy* view =
    this->Implementation->ActiveView->getRenderViewProxy();

  foreach (vtkSMNewWidgetRepresentationProxy* widget,
    this->Implementation->LocationWigets)
    {
    // this method avoids duplicate addition if the widget has been already added.
    vtkSMPropertyHelper(view, "HiddenRepresentations").Add(widget);
    view->UpdateVTKObjects();

    pqSMAdaptor::setElementProperty(widget->GetProperty("Enabled"), 1);
    widget->UpdateVTKObjects();
    }
  this->Implementation->ActiveView->render();
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::allocateWidgets(unsigned int numWidgets)
{
  pq3DWidgetFactory* factory = pqApplicationCore::instance()->get3DWidgetFactory();

  while (static_cast<unsigned int>(this->Implementation->LocationWigets.size()) > numWidgets)
    {
    vtkSmartPointer<vtkSMNewWidgetRepresentationProxy> widget =
      this->Implementation->LocationWigets.takeLast();
    if (this->Implementation->ActiveView)
      {
      pqSMAdaptor::setElementProperty(widget->GetProperty("Enabled"), 0);
      vtkSMPropertyHelper(
        this->Implementation->ActiveView->getProxy(), "HiddenRepresentations").Remove(
        widget);
      this->Implementation->ActiveView->getProxy()->UpdateVTKObjects();
      }
    this->Implementation->VTKConnectSelInput->Disconnect(widget, 0, this, 0);
    factory->free3DWidget(widget);
    }

  for (unsigned int kk = this->Implementation->LocationWigets.size();
    kk < numWidgets; kk++)
    {
    vtkSMNewWidgetRepresentationProxy* widget =
      factory->get3DWidget("HandleWidgetRepresentation",
        this->Implementation->InputPort->getServer());
    widget->UpdateVTKObjects();

    this->Implementation->VTKConnectSelInput->Connect(widget,
      vtkCommand::EndInteractionEvent,
      this, SLOT(updateLocationFromWidgets()), 0, 0, Qt::QueuedConnection);
    this->Implementation->LocationWigets.push_back(widget);
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateLocationWidgets()
{
  bool show_widgets =
    (this->Implementation->showLocationWidgets->checkState() == Qt::Checked);
  if (!show_widgets || this->getContentType() != vtkSelectionNode::LOCATIONS ||
    !this->Implementation->getSelectionSource())
    {
    this->removeWidgetsFromView();
    this->allocateWidgets(0);
    return;
    }

  pqSignalAdaptorTreeWidget* adaptor = this->Implementation->LocationsAdaptor;
  const QList<QVariant>& values = adaptor->values();

  unsigned int numLocations = values.size()/3;

  // this will allocate new widgets if needed and delete old widgets.
  this->allocateWidgets(numLocations);

  this->addWidgetsToView();

  for (unsigned int cc=0; cc < numLocations; cc++)
    {
    vtkSMNewWidgetRepresentationProxy* widget =
      this->Implementation->LocationWigets[cc];
    QList<QVariant> posValues;
    posValues << values[3*cc] << values[3*cc+1] << values[3*cc+2];
    pqSMAdaptor::setMultipleElementProperty(widget->GetProperty("WorldPosition"),
      posValues);
    widget->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateLocationFromWidgets()
{
  bool show_widgets =
    (this->Implementation->showLocationWidgets->checkState() == Qt::Checked);
  if (!show_widgets || this->getContentType() != vtkSelectionNode::LOCATIONS ||
    !this->Implementation->getSelectionSource())
    {
    return;
    }

  int numLocations = this->Implementation->LocationWigets.size();
  if (numLocations > 0)
    {
    pqSignalAdaptorTreeWidget* adaptor = this->Implementation->LocationsAdaptor;
    QList<QVariant> values;
    for (int kk = 0; kk < numLocations; kk++)
      {
      vtkSMNewWidgetRepresentationProxy* widget =
        this->Implementation->LocationWigets[kk];
      widget->UpdatePropertyInformation();
      QList<QVariant> posValues = pqSMAdaptor::getMultipleElementProperty(
        widget->GetProperty("WorldPosition"));
      values += posValues;
      }
    adaptor->setValues(values);
    }
}

//-----------------------------------------------------------------------------
/// Called when ShowFrustum checkbox is toggled.
void pqSelectionInspectorPanel::updateFrustum()
{
  bool showFrustum = this->Implementation->checkboxShowFrustum->isChecked();
  this->updateFrustumInternal(showFrustum);
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateFrustumInternal(bool showFrustum)
{
  if (!this->Implementation->ActiveView)
    {
    showFrustum = false;
    }
  vtkSMSourceProxy* selSource = this->Implementation->getSelectionSource();

  if (!selSource || strcmp(selSource->GetXMLName(), "FrustumSelectionSource") != 0)
    {
    showFrustum = false;
    }

  if (!showFrustum)
    {
    if (this->Implementation->FrustumWidget)
      {
      if (this->Implementation->ActiveView)
        {
        vtkSMPropertyHelper(
          this->Implementation->ActiveView->getProxy(),
          "HiddenProps").Remove(
          this->Implementation->FrustumWidget);
        this->Implementation->ActiveView->getProxy()->UpdateVTKObjects();
        }
      this->Implementation->FrustumWidget = 0;
      this->updateRepresentationViews();
      }
    return;
    }

  if (!this->Implementation->FrustumWidget)
    {
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    vtkSMProxy* repr = pxm->NewProxy("representations", "FrustumWidget");
    this->Implementation->FrustumWidget.TakeReference(repr);
    repr->SetConnectionID(
      this->Implementation->ActiveView->getServer()->GetConnectionID());
    repr->UpdateVTKObjects();
    }

  vtkSMPropertyHelper(
    this->Implementation->ActiveView->getProxy(),
    "HiddenProps").Add(this->Implementation->FrustumWidget);
  this->Implementation->ActiveView->getProxy()->UpdateVTKObjects();
  QList<QVariant> values32 = pqSMAdaptor::getMultipleElementProperty(
    selSource->GetProperty("Frustum"));
  QList<QVariant> values24;
  for (int cc=0; cc < 8; cc++)
    {
    for (int kk=0; kk < 3; kk++)
      {
      values24.push_back(values32[cc*4+kk]);
      }
    }
  pqSMAdaptor::setMultipleElementProperty(
    this->Implementation->FrustumWidget->GetProperty("Frustum"),
    values24);
  this->Implementation->FrustumWidget->UpdateVTKObjects();
  this->updateRepresentationViews();
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateSelectionTypesAvailable()
{
  this->updateSelectionTypesAvailable(this->Implementation->InputPort);
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateSelectionTypesAvailable(pqOutputPort* port)
{
  int cur_index = this->Implementation->comboSelectionType->currentIndex();

  // If currently we are showing a GlobalID based selection and the global IDs
  // disappear from under us, we don't remove them from the combo.
  bool has_gids = this->hasGlobalIDs(port)||
    (cur_index == pqImplementation::GLOBALIDS);

  bool prev = this->Implementation->comboSelectionType->blockSignals(true);
  this->Implementation->comboSelectionType->clear();
  for (int cc = pqImplementation::IDS; cc <= pqImplementation::GLOBALIDS; cc++)
    {
    if (!has_gids && cc == pqImplementation::GLOBALIDS)
      {
      continue;
      }
    this->Implementation->comboSelectionType->addItem(
      pqImplementation::getText(cc,
        this->Implementation->comboFieldType->currentText() ==
          QString("POINT")? 1 : 0));
    }
  this->Implementation->comboSelectionType->setCurrentIndex(cur_index);
  this->Implementation->comboSelectionType->blockSignals(prev);
}

bool pqSelectionInspectorPanel::hasGlobalIDs(pqOutputPort* port)
{
  if (!port)
    {
    return false;
    }

  vtkPVDataInformation* info = port->getDataInformation();
  vtkPVDataSetAttributesInformation* attrInfo = 0;

  if (this->Implementation->comboFieldType->currentText() == QString("POINT"))
    {
    attrInfo = info->GetPointDataInformation();
    }
  else
    {
    attrInfo = info->GetCellDataInformation();
    }
  return attrInfo->GetAttributeInformation(vtkDataSetAttributes::GLOBALIDS);
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::onSelectionColorChanged(const QColor& color)
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->setValue("GlobalProperties/SelectionColor", color);
}

void pqSelectionInspectorPanel::setGlobalIDs()
{
  this->Implementation->comboSelectionType->setCurrentIndex(
    pqImplementation::GLOBALIDS); // Global IDs
  // the celllabelarraydomain and pointlabelarraydomain are valid only when
  // the user turn the visibility to ON.
  // Hence we connect to the event when the domain is valid to select the
  // Global {Element|node} Id array.
  // But before this event, we artificially add a "Global {Element|Node} Id"
  // entrie in the combobox (that will be cleared when the domain is updated)
  if(!this->Implementation->comboLabelMode_Cell->count())
    {
    this->Implementation->CellLabelArrayDomain->addString("GlobalElementId");
    this->Implementation->PointLabelArrayDomain->addString("GlobalNodeId");

    this->Implementation->comboLabelMode_Cell->addItem("GlobalElementId", QString("GlobalElementId"));
    this->Implementation->comboLabelMode_Cell->setCurrentIndex(
      this->Implementation->comboLabelMode_Cell->count()-1);
    this->Implementation->comboLabelMode_Point->addItem("GlobalNodeId", QString("GlobalNodeId"));
    this->Implementation->comboLabelMode_Point->setCurrentIndex(
      this->Implementation->comboLabelMode_Point->count()-1);
    vtkSMProxy* reprProxy = this->Implementation->getSelectionRepresentation()
      ->getProxy();
    this->Implementation->VTKConnectRep->Connect(
      reprProxy->GetProperty("SelectionPointFieldDataArrayName")->FindDomain("vtkSMDomain"),
      vtkCommand::DomainModifiedEvent, this,
      SLOT(forceLabelGlobalId(vtkObject*)),
      NULL, 0.0,//reprProxy->GetProperty("SelectionPointFieldDataArrayName")->FindDomain("vtkSMDomain"), 0.0,
      Qt::QueuedConnection);
    this->Implementation->VTKConnectRep->Connect(
      reprProxy->GetProperty("SelectionCellFieldDataArrayName")->FindDomain("vtkSMDomain"),
      vtkCommand::DomainModifiedEvent, this,
      SLOT(forceLabelGlobalId(vtkObject*)),
      NULL, 0.0,//reprProxy->GetProperty("SelectionCellFieldDataArrayName")->FindDomain("vtkSMDomain"), 0.0,
      Qt::QueuedConnection);
    }
  else
    {
    this->Implementation->comboLabelMode_Cell->setCurrentIndex(
      this->Implementation->comboLabelMode_Cell->findText("Global",
                                                          Qt::MatchStartsWith));
    this->Implementation->comboLabelMode_Point->setCurrentIndex(
      this->Implementation->comboLabelMode_Point->findText("Global",
                                                           Qt::MatchStartsWith));
    }
}

void pqSelectionInspectorPanel::forceLabelGlobalId(vtkObject* object)
{
  pqDataRepresentation* repr = this->Implementation->getSelectionRepresentation();
  vtkSMProxy* reprProxy = repr? repr->getProxy() : 0;
  if (!reprProxy)
    {
    return;
    }

  if( dynamic_cast<vtkSMDomain*>(object) ==
      reprProxy->GetProperty("SelectionCellFieldDataArrayName")
      ->FindDomain("vtkSMDomain") )
    {
    this->Implementation->comboLabelMode_Cell->setCurrentIndex(
      this->Implementation->comboLabelMode_Cell->findText("GlobalElementId",
                                                          Qt::MatchStartsWith));
    // one shot forcing
    this->Implementation->VTKConnectRep->Disconnect(
      reprProxy->GetProperty("SelectionCellFieldDataArrayName")
      ->FindDomain("vtkSMDomain"), vtkCommand::DomainModifiedEvent,
      this, SLOT(forceLabelGlobalId(vtkObject*)));
    this->Implementation->CellLabelArrayDomain->removeString("GlobalElementId");
    }
  else //== reprProxy->GetProperty("SelectionPointFieldDataArrayName")->FindDomain..
    {
    this->Implementation->comboLabelMode_Point->setCurrentIndex(
      this->Implementation->comboLabelMode_Point->findText("GlobalNodeId",
                                                           Qt::MatchStartsWith));
    // one shot forcing
    this->Implementation->VTKConnectRep->Disconnect(
      reprProxy->GetProperty("SelectionPointFieldDataArrayName")
      ->FindDomain("vtkSMDomain"), vtkCommand::DomainModifiedEvent,
      this, SLOT(forceLabelGlobalId(vtkObject*)));
    this->Implementation->PointLabelArrayDomain->removeString("GlobalNodeId");
    }
}
