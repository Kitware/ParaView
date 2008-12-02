/*
* Copyright (c) 2007, Sandia Corporation
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the Sandia Corporation nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Sandia Corporation ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Sandia Corporation BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ClientHierarchyView.h"

#include "ui_ClientHierarchyView.h"

#include <vtkAlgorithmOutput.h>
#include <vtkDataObjectTypes.h>
#include <vtkDataRepresentation.h>
#include <vtkDataSetAttributes.h>
#include <vtkEdgeListIterator.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkExtractSelectedGraph.h>
#include <vtkGraphLayoutView.h>
#include <vtkHierarchicalGraphView.h>
#include <vtkIdTypeArray.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkIntArray.h>
#include <vtkProcessModule.h>
#include <vtkPVDataInformation.h>
#include <vtkPVUpdateSuppressor.h>
#include <vtkQtTreeModelAdapter.h>
#include <vtkQtTreeView.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSelectionLink.h>
#include <vtkSelectionNode.h>
#include <vtkSelectionSource.h>
#include <vtkSmartPointer.h>
#include <vtkSMClientDeliveryRepresentationProxy.h>
#include <vtkSMIdTypeVectorProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxyManager.h>
#include <vtkSMSelectionDeliveryRepresentationProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkSMViewProxy.h>
#include <vtkTable.h>
#include <vtkTexture.h>
#include <vtkTree.h>
#include <vtkVariantArray.h>
#include <vtkViewTheme.h>
#include <vtkWindowToImageFilter.h>

#include <pqDataRepresentation.h>
#include <pqPropertyHelper.h>
#include <pqOutputPort.h>
#include <pqPipelineSource.h>
#include <pqRepresentation.h>
#include <pqSelectionManager.h>
#include <pqServer.h>

#include <QTimer>

////////////////////////////////////////////////////////////////////////////////////
// ClientHierarchyView::command

class ClientHierarchyView::command : public vtkCommand
{
public:
  command(ClientHierarchyView& view) : Target(view) { }
  virtual void Execute(vtkObject*, unsigned long, void*)
  {
    Target.selectionChanged();
  }
  ClientHierarchyView& Target;
};

////////////////////////////////////////////////////////////////////////////////////
// ClientHierarchyView::implementation

class ClientHierarchyView::implementation
{
public:
  implementation() :
    TreeRepresentation(0),
    GraphRepresentation(0),
    UpdateFlags(0)
  {
    this->Widgets.setupUi(&this->Widget);
    this->Widgets.horizontalSplitter->setSizes(QList<int>() << 100 << 400);
    this->Widgets.graphView->setVisible(false);

    this->HierarchicalGraphTheme.TakeReference(vtkViewTheme::CreateMellowTheme());
    
    this->TreeView = vtkSmartPointer<vtkQtTreeView>::New();
    this->TreeView->SetItemView(this->Widgets.treeView);
    this->TreeView->SetItemModelAdapter(&this->TreeAdapter);

    this->HierarchicalGraphView = vtkSmartPointer<vtkHierarchicalGraphView>::New();
    this->HierarchicalGraphView->SetupRenderWindow(this->Widgets.hierarchicalGraphView->GetRenderWindow());
    this->HierarchicalGraphView->ApplyViewTheme(this->HierarchicalGraphTheme);

    this->UpdateTimer.setInterval(0);
    this->UpdateTimer.setSingleShot(true);

    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  }

  pqRepresentation* TreeRepresentation;
  pqRepresentation* GraphRepresentation;

  vtkSmartPointer<vtkQtTreeView> TreeView;

  QWidget Widget;
  Ui::ClientHierarchyView Widgets;
 
  vtkSmartPointer<vtkViewTheme> HierarchicalGraphTheme; 

  vtkQtTreeModelAdapter TreeAdapter;
  vtkSmartPointer<vtkHierarchicalGraphView> HierarchicalGraphView;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;

  /// Used to "collapse" redundant updates
  QTimer UpdateTimer;
  /// Used to store operations to be performed by the next update
  int UpdateFlags;
};

enum
{
  DELIVER_TREE = 1 << 0,
  DELIVER_GRAPH = 1 << 1,
  SYNC_ATTRIBUTES = 1 << 6,
  RESET_HGRAPH_CAMERA = 1 << 7, 
  UPDATE_TREE_VIEW = 1 << 9, 
  UPDATE_HGRAPH_VIEW = 1 << 10,
};


////////////////////////////////////////////////////////////////////////////////////
// ClientHierarchyView

ClientHierarchyView::ClientHierarchyView(
    const QString& viewmoduletype, 
    const QString& group, 
    const QString& name, 
    vtkSMViewProxy* viewmodule, 
    pqServer* server, 
    QObject* p) :
  pqMultiInputView(viewmoduletype, group, name, viewmodule, server, p),
  Implementation(new implementation()),
  Command(new command(*this))
{
  QObject::connect(this->Implementation->Widgets.treeView, SIGNAL(expanded(const QModelIndex&)), this, SLOT(treeVisibilityChanged()));
  QObject::connect(this->Implementation->Widgets.treeView, SIGNAL(collapsed(const QModelIndex&)), this, SLOT(treeVisibilityChanged()));

  QObject::connect(&this->Implementation->UpdateTimer, SIGNAL(timeout()), this, SLOT(synchronizeViews()));

  // Listen to all views that may fire progress events during updating.
  this->Implementation->VTKConnect->Connect(
    this->Implementation->HierarchicalGraphView, vtkCommand::ViewProgressEvent,
    this, SLOT(onViewProgressEvent(vtkObject*, unsigned long, void*, void*)));

  this->Implementation->HierarchicalGraphView->AddObserver(
    vtkCommand::SelectionChangedEvent, this->Command);
  this->Implementation->TreeView->AddObserver(
    vtkCommand::SelectionChangedEvent, this->Command);

  this->Implementation->TreeView->SetSelectionType(
    vtkSelectionNode::PEDIGREEIDS);
  this->Implementation->HierarchicalGraphView->SetSelectionType(
    vtkSelectionNode::PEDIGREEIDS);

  this->showTree(false);

}

ClientHierarchyView::~ClientHierarchyView()
{
  delete this->Implementation;
  this->Command->Delete();
}

void ClientHierarchyView::selectionChanged()
{
  // Get the representaion's source
  pqDataRepresentation* pqRepr =
    qobject_cast<pqDataRepresentation*>(
      this->Implementation->GraphRepresentation);
  
  // Sanity check: There are cases where I might not
  // have a graph in this view.
  if (pqRepr == 0)
    {
    return;
    }
    
  pqOutputPort* opPort = pqRepr->getOutputPortFromInput();
  vtkSMSourceProxy* repSource = vtkSMSourceProxy::SafeDownCast(
    opPort->getSource()->getProxy());

  // Fill the selection source with the selection from the view
  vtkSelection* sel = this->Implementation->HierarchicalGraphView->
    GetGraphRepresentation()->GetSelectionLink()->GetSelection();
  vtkSMSourceProxy* selectionSource =
    pqSelectionManager::createSelectionSource(sel, repSource->GetConnectionID());

  // Set the selection on the representation's source
  repSource->SetSelectionInput(opPort->getPortNumber(),
    selectionSource, 0);
  selectionSource->Delete();

  // Make sure the views update
  this->Implementation->TreeView->Update();
  this->Implementation->HierarchicalGraphView->Update();
}

QWidget* ClientHierarchyView::getWidget()
{
  return &this->Implementation->Widget;
}

bool ClientHierarchyView::canDisplay(pqOutputPort* output_port) const
{
  if(!output_port)
    return false;

  pqPipelineSource* const source = output_port->getSource();
  if(!source)
    return false;

  if(this->getServer()->GetConnectionID() != source->getServer()->GetConnectionID())
    return false;

  vtkSMSourceProxy* source_proxy =
    vtkSMSourceProxy::SafeDownCast(source->getProxy());
  if (!source_proxy ||
     source_proxy->GetOutputPortsCreated() == 0)
    {
    return false;
    }

  const char* name = output_port->getDataClassName();
  int type = vtkDataObjectTypes::GetTypeIdFromClassName(name);
  switch(type)
    {
    case VTK_DIRECTED_ACYCLIC_GRAPH:
    case VTK_DIRECTED_GRAPH:
    case VTK_GRAPH:
    case VTK_TREE:
    case VTK_UNDIRECTED_GRAPH:
      return true;
    }

  return false;
}

const bool ClientHierarchyView::treeVisible()
{
  return this->Implementation->Widgets.treeView->isVisible();
}

const bool ClientHierarchyView::hierarchicalGraphVisible()
{
  return this->Implementation->Widgets.hierarchicalGraphView->isVisible();
}

void ClientHierarchyView::showTree(bool visible)
{
  this->Implementation->Widgets.treeView->setVisible(visible);
}

void ClientHierarchyView::showHierarchicalGraph(bool visible)
{
  this->Implementation->Widgets.hierarchicalGraphView->setVisible(visible);
}

void ClientHierarchyView::setHierarchicalGraphLayout(const QString& layout)
{
  this->Implementation->HierarchicalGraphView->SetLayoutStrategy(layout.toAscii().data());
}

void ClientHierarchyView::setRadialLayout(bool value)
{
  this->Implementation->HierarchicalGraphView->SetRadialLayout(value);
}

void ClientHierarchyView::setTreeLeafSpacing(double spacing)
{
  this->Implementation->HierarchicalGraphView->SetLeafSpacing(spacing);
}

void ClientHierarchyView::setTreeLevelSpacing(double spacing)
{
  this->Implementation->HierarchicalGraphView->SetLogSpacingFactor(spacing);
}

void ClientHierarchyView::setRadialAngle(int angle)
{
  this->Implementation->HierarchicalGraphView->SetRadialAngle(angle);
}

void ClientHierarchyView::setEdgeBundlingStrength(double strength)
{
  this->Implementation->HierarchicalGraphView->SetBundlingStrength(strength);
}

void ClientHierarchyView::showRepresentation(pqRepresentation* representation)
{
  vtkSMClientDeliveryRepresentationProxy* const proxy = vtkSMClientDeliveryRepresentationProxy::SafeDownCast(representation->getProxy());
  proxy->Update();
  vtkDataObject* output = proxy->GetOutput();

  if(vtkTree::SafeDownCast(output))
    {
    this->Implementation->TreeRepresentation = representation;
    vtkDataRepresentation* qttree_rep = this->Implementation->TreeView->
      SetRepresentationFromInputConnection(output->GetProducerPort());
    vtkDataRepresentation* htree_rep = this->Implementation->
      HierarchicalGraphView->SetHierarchyFromInput(output);

    // Set tree selection link.
    vtkSelectionLink *link = htree_rep->GetSelectionLink();
    qttree_rep->SetSelectionLink(link);

    this->scheduleSynchronization(DELIVER_TREE | DELIVER_GRAPH | SYNC_ATTRIBUTES | RESET_HGRAPH_CAMERA | UPDATE_TREE_VIEW | UPDATE_HGRAPH_VIEW );
    }
  else if(vtkGraph::SafeDownCast(output))
    {
    this->Implementation->GraphRepresentation = representation;
    this->Implementation->HierarchicalGraphView->SetGraphFromInput(output);

    this->scheduleSynchronization(DELIVER_TREE | DELIVER_GRAPH | SYNC_ATTRIBUTES | RESET_HGRAPH_CAMERA | UPDATE_HGRAPH_VIEW );
    }
}

void ClientHierarchyView::updateRepresentation(pqRepresentation* representation)
{
  if(this->Implementation->TreeRepresentation == representation)
    {
    this->scheduleSynchronization(DELIVER_TREE | RESET_HGRAPH_CAMERA | UPDATE_TREE_VIEW | UPDATE_HGRAPH_VIEW);
    }
  else if(this->Implementation->GraphRepresentation == representation)
    {
    this->scheduleSynchronization(DELIVER_GRAPH | RESET_HGRAPH_CAMERA | UPDATE_HGRAPH_VIEW );
    }
}

void ClientHierarchyView::hideRepresentation(pqRepresentation* representation)
{
  if(this->Implementation->TreeRepresentation == representation)
    {
    this->Implementation->TreeRepresentation = 0;
    this->scheduleSynchronization(UPDATE_TREE_VIEW | UPDATE_HGRAPH_VIEW);
    }
  else if(this->Implementation->GraphRepresentation == representation)
    {
    this->Implementation->GraphRepresentation = 0;
    this->Implementation->HierarchicalGraphView->RemoveAllRepresentations();

    this->scheduleSynchronization(UPDATE_HGRAPH_VIEW);
    }
}

void ClientHierarchyView::render()
{
  this->scheduleSynchronization(DELIVER_GRAPH | DELIVER_TREE | SYNC_ATTRIBUTES | UPDATE_HGRAPH_VIEW );
}

void ClientHierarchyView::treeVisibilityChanged()
{
  this->scheduleSynchronization(UPDATE_HGRAPH_VIEW);
}

void ClientHierarchyView::scheduleSynchronization(int update_flags)
{
  this->Implementation->UpdateFlags |= update_flags;
  this->Implementation->UpdateTimer.start();
}

void ClientHierarchyView::synchronizeViews()
{
  // Enable progress handling.
  emit this->beginProgress();

  // Deliver tree data from the server ...
  vtkSMSelectionDeliveryRepresentationProxy* const tree_proxy = this->Implementation->TreeRepresentation
    ? vtkSMSelectionDeliveryRepresentationProxy::SafeDownCast(this->Implementation->TreeRepresentation->getProxy())
    : 0;

  // This should not do anything if upstream filters have not changed
  if(tree_proxy)
    {
    tree_proxy->Update();
    }

  // Deliver graph data from the server ...
  vtkSMSelectionDeliveryRepresentationProxy* const graph_proxy = this->Implementation->GraphRepresentation
    ? vtkSMSelectionDeliveryRepresentationProxy::SafeDownCast(this->Implementation->GraphRepresentation->getProxy())
    : 0;

  // This should not do anything if upstream filters have not changed
  if(graph_proxy)
    {
    graph_proxy->Update();
    }

  if(tree_proxy && graph_proxy)
    {
    vtkSelectionLink* graph_link = this->Implementation->HierarchicalGraphView->
      GetGraphRepresentation()->GetSelectionLink();
    vtkSelectionLink* tree_link = this->Implementation->HierarchicalGraphView->
      GetTreeRepresentation()->GetSelectionLink();

    // Update the selection.
    tree_proxy->GetSelectionRepresentation()->Update();
    vtkSelection* sel = vtkSelection::SafeDownCast(
      tree_proxy->GetSelectionRepresentation()->GetOutput());
    graph_link->SetSelection(sel);
    tree_link->SetSelection(sel);
    this->Implementation->TreeView->Update();
    this->Implementation->HierarchicalGraphView->Update();

    // Set the current domain map
    int useDomainMap = vtkSMPropertyHelper(tree_proxy, "UseDomainMap").GetAsInt();
    graph_link->RemoveAllDomainMaps();
    tree_link->RemoveAllDomainMaps();
    if (useDomainMap)
      {
      vtkSMSourceProxy* domainMap = 0;
      if (vtkSMPropertyHelper(tree_proxy, "DomainMap").GetNumberOfElements() > 0)
        {
        domainMap = vtkSMSourceProxy::SafeDownCast(
          vtkSMPropertyHelper(tree_proxy, "DomainMap").GetAsProxy());
        }
      if (domainMap)
        {
        vtkSMProxyManager* pm = vtkSMProxyManager::GetProxyManager();
        vtkSMClientDeliveryRepresentationProxy* delivery =
          vtkSMClientDeliveryRepresentationProxy::SafeDownCast(
            pm->NewProxy("representations", "ClientDeliveryRepresentation"));
        delivery->SetConnectionID(domainMap->GetConnectionID());
        delivery->AddInput(domainMap, "AddInput");
        delivery->Update();
        vtkTable* output = vtkTable::SafeDownCast(delivery->GetOutput());
        if (output)
          {
          graph_link->AddDomainMap(output);
          tree_link->AddDomainMap(output);
          }
        }
      }
    }

  // Synchronize attributes ...
  if(this->Implementation->UpdateFlags & SYNC_ATTRIBUTES)
    {
    // Sync hgraph attributes
    if(this->Implementation->TreeRepresentation && this->Implementation->GraphRepresentation)
      {
      this->Implementation->HierarchicalGraphView->SetVertexLabelVisibility(vtkSMPropertyHelper(tree_proxy, "VertexLabels").GetAsInt());
      this->Implementation->HierarchicalGraphView->SetVertexLabelArrayName(vtkSMPropertyHelper(tree_proxy, "VertexLabelArray").GetAsString());

      this->Implementation->HierarchicalGraphView->SetVertexLabelFontSize(static_cast<int>(vtkSMPropertyHelper(tree_proxy, "VertexLabelFontSize").GetAsDouble()));
      this->Implementation->HierarchicalGraphTheme->SetPointSize(vtkSMPropertyHelper(tree_proxy, "VertexSize").GetAsDouble());
      this->Implementation->HierarchicalGraphTheme->SetPointOpacity(vtkSMPropertyHelper(tree_proxy, "VertexOpacity").GetAsDouble());

      if(vtkSMPropertyHelper(tree_proxy, "VertexColorByArray").GetAsInt())
        {
        this->Implementation->HierarchicalGraphView->SetColorVertices(true);
        this->Implementation->HierarchicalGraphView->SetVertexColorArrayName(vtkSMPropertyHelper(tree_proxy, "VertexColorArray").GetAsString());
        }
      else
        {
        this->Implementation->HierarchicalGraphView->SetColorVertices(false);

        this->Implementation->HierarchicalGraphTheme->SetPointColor(
          vtkSMPropertyHelper(tree_proxy, "VertexColor").GetAsDouble(0),
          vtkSMPropertyHelper(tree_proxy, "VertexColor").GetAsDouble(1),
          vtkSMPropertyHelper(tree_proxy, "VertexColor").GetAsDouble(2));
        }

      this->Implementation->HierarchicalGraphView->SetEdgeLabelVisibility(vtkSMPropertyHelper(tree_proxy, "EdgeLabels").GetAsInt());
      this->Implementation->HierarchicalGraphView->SetEdgeLabelArrayName(vtkSMPropertyHelper(tree_proxy, "EdgeLabelArray").GetAsString());
      this->Implementation->HierarchicalGraphView->SetEdgeLabelFontSize(static_cast<int>(vtkSMPropertyHelper(tree_proxy, "EdgeLabelFontSize").GetAsDouble()));
      this->Implementation->HierarchicalGraphTheme->SetLineWidth(vtkSMPropertyHelper(tree_proxy, "EdgeWidth").GetAsDouble());
      this->Implementation->HierarchicalGraphTheme->SetCellOpacity(vtkSMPropertyHelper(tree_proxy, "EdgeOpacity").GetAsDouble());

      if(vtkSMPropertyHelper(graph_proxy, "EdgeColorByArray").GetAsInt())
        {
        this->Implementation->HierarchicalGraphView->SetColorEdges(true);
        this->Implementation->HierarchicalGraphView->SetEdgeColorArrayName(vtkSMPropertyHelper(graph_proxy, "EdgeColorArray").GetAsString());
        }
      else
        {
        this->Implementation->HierarchicalGraphView->SetColorEdges(false);

        this->Implementation->HierarchicalGraphTheme->SetCellColor(
          vtkSMPropertyHelper(graph_proxy, "EdgeColor").GetAsDouble(0),
          vtkSMPropertyHelper(graph_proxy, "EdgeColor").GetAsDouble(1),
          vtkSMPropertyHelper(graph_proxy, "EdgeColor").GetAsDouble(2));
        }

      bool iconVisibility = vtkSMPropertyHelper(tree_proxy, "IconVisibility").GetAsInt();

      this->Implementation->HierarchicalGraphView->SetIconVisibility(
        iconVisibility);

      if(iconVisibility && !pqPropertyHelper(tree_proxy, "IconFile").GetAsString().isEmpty())
        {
        vtkSMProxy *textureProxy = vtkSMPropertyHelper(tree_proxy, "IconTexture").GetAsProxy(0);
        vtkTexture *texture = vtkTexture::SafeDownCast(vtkProcessModule::GetProcessModule()->GetObjectFromID(textureProxy->GetID()));
        this->Implementation->HierarchicalGraphView->SetIconTexture( texture );

        int iconSize[2];
        iconSize[0] = iconSize[1] = vtkSMPropertyHelper(tree_proxy, "IconSize").GetAsInt();
        this->Implementation->HierarchicalGraphView->SetIconSize(iconSize);

        this->Implementation->HierarchicalGraphView->SetIconAlignment(vtkSMPropertyHelper(tree_proxy, "IconAlignment").GetAsInt()+1);

        vtkSMStringVectorProperty *iconTypes = vtkSMStringVectorProperty::SafeDownCast(tree_proxy->GetProperty("IconTypes"));
        vtkSMIntVectorProperty *iconIndices = vtkSMIntVectorProperty::SafeDownCast(tree_proxy->GetProperty("IconIndices"));
        this->Implementation->HierarchicalGraphView->ClearIconTypes();
        for(unsigned int i=0; i<iconTypes->GetNumberOfElements(); i++)
          {
          this->Implementation->HierarchicalGraphView->AddIconType((char*)iconTypes->GetElement(i), iconIndices->GetElement(i));
          }

        this->Implementation->HierarchicalGraphView->SetIconArrayName(
          vtkSMPropertyHelper(tree_proxy, "IconArray").GetAsString());
        }

      this->Implementation->HierarchicalGraphView->ApplyViewTheme(this->Implementation->HierarchicalGraphTheme);
      }
    }

  // Reset the hgraph camera ...
  if(this->Implementation->UpdateFlags & RESET_HGRAPH_CAMERA)
    {
    this->Implementation->HierarchicalGraphView->GetRenderer()->ResetCamera();
    }

  // Update the tree view ...
  if(this->Implementation->UpdateFlags & UPDATE_TREE_VIEW)
    {
    this->Implementation->TreeView->Update();
    this->Implementation->Widgets.treeView->expandToDepth(2);
    this->Implementation->Widgets.treeView->resizeColumnToContents(0);
    this->Implementation->Widgets.treeView->resizeColumnToContents(1);
    }

  // Update the hierarchical graph view ...
  if(this->Implementation->UpdateFlags & UPDATE_HGRAPH_VIEW)
    {
    this->Implementation->HierarchicalGraphView->Update();
    }

  this->Implementation->UpdateFlags = 0;

  // Done with progresses.
  emit this->endProgress();
}


vtkImageData* ClientHierarchyView::captureImage(int magnification)
{

  // Offscreen rendering is not functioning properly on the mac.
  // Do not use it.

#if !defined(__APPLE__)
  //int useOffscreenRenderingForScreenshots = this->UseOffscreenRenderingForScreenshots;
  int prevOffscreen = this->Implementation->HierarchicalGraphView->GetRenderWindow()->GetOffScreenRendering();
  if (!prevOffscreen) //useOffscreenRenderingForScreenshots && !prevOffscreen)
    {
    this->Implementation->HierarchicalGraphView->GetRenderWindow()->SetOffScreenRendering(1);
    }
#endif

  vtkRenderWindow *renWin = this->Implementation->HierarchicalGraphView->GetRenderWindow();

  renWin->SwapBuffersOff();
  //this->getViewProxy()->StillRender();
  renWin->Render();

  vtkWindowToImageFilter* w2i = vtkWindowToImageFilter::New();
  w2i->SetInput(renWin);
  w2i->SetMagnification(magnification);
  w2i->ReadFrontBufferOff();
  w2i->ShouldRerenderOff();
  w2i->Update();

  vtkImageData* capture = vtkImageData::New();
  capture->ShallowCopy(w2i->GetOutput());
  w2i->Delete();

  renWin->SwapBuffersOn();
  renWin->Frame();

#if !defined(__APPLE__)
  if (!prevOffscreen) //useOffscreenRenderingForScreenshots && !prevOffscreen)
    {
    this->Implementation->HierarchicalGraphView->GetRenderWindow()->SetOffScreenRendering(0);
    }
#endif

  // Update image extents based on ViewPosition
  int extents[6];
  capture->GetExtent(extents);

  for (int cc=0; cc < 4; cc++)
    {
    extents[cc] += this->getViewProxy()->GetViewPosition()[cc/2]*magnification;
    }
  capture->SetExtent(extents);

  return capture;
}

void ClientHierarchyView::onViewProgressEvent(vtkObject*, 
  unsigned long vtk_event, void*, void* call_data)
{
  if (vtk_event == vtkCommand::ViewProgressEvent)
    {
    const vtkView::ViewProgressEventCallData* data = 
      reinterpret_cast<const vtkView::ViewProgressEventCallData*>(call_data);

    emit this->progress(QString(data->GetProgressMessage()), 
      static_cast<int>(data->GetProgress()*100.0));
    }
}
