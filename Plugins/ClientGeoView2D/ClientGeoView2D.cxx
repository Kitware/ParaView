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

#include "ClientGeoView2D.h"

#include <QVTKWidget.h>
#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkDataObject.h>
#include <vtkDataObjectTypes.h>
#include "vtkEventQtSlotConnect.h"
#include <vtkGeoCamera.h>
#include <vtkGeoImageNode.h>
#include <vtkGeoAlignedImageRepresentation.h>
#include <vtkGeoAlignedImageSource.h>
#include <vtkGeoFileImageSource.h>
#include <vtkGeoLineRepresentation.h>
#include <vtkGeometryFilter.h>
#include <vtkGeoProjection.h>
#include <vtkGeoProjectionSource.h>
#include <vtkGeoTerrain2D.h>
#include <vtkGeoTerrainNode.h>
#include <vtkGeoTransform.h>
#include <vtkGeoView2D.h>
#include <vtkGraph.h>
#include <vtkJPEGReader.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkPNGReader.h>
#include <vtkPointSet.h>
#include <vtkPVDataInformation.h>
#include <vtkRenderedGraphRepresentation.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSelection.h>
#include <vtkSelectionLink.h>
#include <vtkSelectionNode.h>
#include <vtkSMIdTypeVectorProperty.h>
#include <vtkSMProxyManager.h>
#include <vtkSMSelectionDeliveryRepresentationProxy.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkSMSourceProxy.h>
#include <vtkSmartPointer.h>
#include <vtkTimerLog.h>
#include <vtkTable.h>
#include <vtkTextProperty.h>
#include <vtkViewTheme.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLPolyDataWriter.h>

#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqFilesystem.h>
#include <pqPropertyHelper.h>
#include <pqOutputPort.h>
#include <pqPipelineSource.h>
#include <pqRepresentation.h>
#include <pqSelectionManager.h>
#include <pqServer.h>

#include <QVBoxLayout>

#include <vtksys/stl/map>

#include "ClientGeoView2DConfig.h" // Overview config file
#include "vtkPQConfig.h" // For PARAVIEW_DATA_ROOT

////////////////////////////////////////////////////////////////////////////////////
// ClientGeoView2D::command

class ClientGeoView2D::command : public vtkCommand
{
public:
  command(ClientGeoView2D& view) : Target(view) { }
  virtual void Execute(vtkObject*, unsigned long, void*)
  {
    Target.selectionChanged();
  }
  ClientGeoView2D& Target;
};

////////////////////////////////////////////////////////////////////////////////////
// ClientGeoView2D::implementation

class ClientGeoView2D::implementation
{
public:
  implementation() :
    Widget(new QVTKWidget())
  {
    new QVBoxLayout(this->Widget);

    this->View = vtkSmartPointer<vtkGeoView2D>::New();
    this->View->SetupRenderWindow(this->Widget->GetRenderWindow());
    this->Transform = vtkSmartPointer<vtkGeoTransform>::New();
    this->ProjectionIndex = 40;
    vtkSmartPointer<vtkGeoProjection> proj =
      vtkSmartPointer<vtkGeoProjection>::New();
    proj->SetName(vtkGeoProjection::GetProjectionName(this->ProjectionIndex));
    this->Transform->SetDestinationProjection(proj);

    this->Theme.TakeReference(vtkViewTheme::CreateMellowTheme());

    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  }

  ~implementation()
  {
    delete this->Widget;
    if (this->ImageRepresentation)
      {
      this->ImageRepresentation->GetSource()->ShutDown();
      this->ImageRepresentation->Delete();
      }
    if (this->Terrain)
      {
      this->Terrain->GetSource()->ShutDown();
      this->Terrain->Delete();
      }
  }

  QVTKWidget* const Widget;
  vtkSmartPointer<vtkGeoView2D> View;
  vtkSmartPointer<vtkGeoTransform> Transform;
  vtkSmartPointer<vtkViewTheme> Theme;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  vtkGeoAlignedImageRepresentation* ImageRepresentation;
  vtkGeoTerrain2D* Terrain;
  int ProjectionIndex;

  typedef vtksys_stl::map<pqRepresentation*, vtkSmartPointer<vtkRenderedGraphRepresentation> >
    RepresentationsT;
  RepresentationsT Representations;
};

////////////////////////////////////////////////////////////////////////////////////
// ClientGeoView2D

ClientGeoView2D::ClientGeoView2D(
  const QString& viewmoduletype,
  const QString& group,
  const QString& name,
  vtkSMViewProxy* viewmodule,
  pqServer* server,
  QObject* p) :
  pqView(viewmoduletype, group, name, viewmodule, server, p),
  Implementation(new implementation()),
  Command(new command(*this))
{
  // We need to know when the visibilty of any pqrepresentation changes, so that
  // we can update corresponding vtk representations' state.
  this->connect(this, SIGNAL(representationVisibilityChanged(pqRepresentation*, bool)),
    this, SLOT(onRepresentationVisibilityChanged(pqRepresentation*, bool)));
  this->connect(this, SIGNAL(representationRemoved(pqRepresentation*)),
    SLOT(onRepresentationRemoved(pqRepresentation*)));

  // We connect to endRender() to trigger the vtkView update.
  this->connect(this, SIGNAL(endRender()), this, SLOT(renderGeoViewInternal()));

  // Set mellow theme
  vtkViewTheme* theme = vtkViewTheme::CreateMellowTheme();
  this->Implementation->View->ApplyViewTheme(theme);
  theme->Delete();

  this->Implementation->ImageRepresentation =
    vtkGeoAlignedImageRepresentation::New();
  vtkStdString tileDatabase(CLIENT_GEO_VIEW_TILE_PATH);
  if (tileDatabase.length() == 0)
    {
    // Set default projection and image
    vtkStdString filename;
    if (pqFilesystem::shareDirectory().exists("NE2_ps_bath.png"))
      {
      filename = pqFilesystem::shareDirectory().filePath("NE2_ps_bath.png").toAscii().data();
      }
    else
      {
      filename = PARAVIEW_DATA_ROOT "/Data/NE2_ps_bath.png";
      }

    // Create image representation.
    vtkSmartPointer<vtkPNGReader> reader = vtkSmartPointer<vtkPNGReader>::New();
    reader->SetFileName(filename.c_str());
    reader->Update();
    vtkImageData* image = reader->GetOutput();
    vtkSmartPointer<vtkGeoAlignedImageSource> imageSource =
      vtkSmartPointer<vtkGeoAlignedImageSource>::New();
    imageSource->SetImage(image);
    imageSource->Initialize();
    this->Implementation->ImageRepresentation->SetSource(imageSource);
    }
  else
    {
    vtkSmartPointer<vtkGeoFileImageSource> imageSource =
      vtkSmartPointer<vtkGeoFileImageSource>::New();
    imageSource->SetPath(tileDatabase.c_str());
    imageSource->Initialize();
    this->Implementation->ImageRepresentation->SetSource(imageSource);
    }

  // Add image representation
  this->Implementation->View->AddRepresentation(
    this->Implementation->ImageRepresentation);

  // Add default terrain
  vtkSmartPointer<vtkGeoProjectionSource> terrainSource =
    vtkSmartPointer<vtkGeoProjectionSource>::New();
  terrainSource->SetProjection(this->Implementation->ProjectionIndex);
  terrainSource->Initialize();
  this->Implementation->Terrain = vtkGeoTerrain2D::New();
  this->Implementation->Terrain->SetSource(terrainSource);
  this->Implementation->View->SetSurface(this->Implementation->Terrain);

  // Set up the viewport
  vtkSmartPointer<vtkGeoTerrainNode> root = vtkSmartPointer<vtkGeoTerrainNode>::New();
  this->Implementation->Terrain->GetSource()->FetchRoot(root);
  double bounds[4];
  root->GetProjectionBounds(bounds);
  this->Implementation->View->GetRenderer()->GetActiveCamera()->
    SetParallelScale((bounds[3] - bounds[2]) / 2.0);

  // Perform initial update
  this->Implementation->Widget->GetRenderWindow()->Render();

  // Load political boundaries
  vtkSmartPointer<vtkGeoLineRepresentation> lineRep =
    vtkSmartPointer<vtkGeoLineRepresentation>::New();
  vtkSmartPointer<vtkXMLPolyDataReader> pbReader =
    vtkSmartPointer<vtkXMLPolyDataReader>::New();
  if (pqFilesystem::shareDirectory().exists("political.vtp"))
    {
    pbReader->SetFileName(pqFilesystem::shareDirectory().filePath("political.vtp").toAscii().data());
    }
  else
    {
    pbReader->SetFileName(PARAVIEW_DATA_ROOT "/Data/political.vtp");
    }
  lineRep->SetTransform(this->Implementation->Transform);
  lineRep->SetInputConnection(pbReader->GetOutputPort());
  lineRep->CoordinatesInArraysOff();
  this->Implementation->View->AddRepresentation(lineRep);

  // Perform another update to show political boundaries
  this->Implementation->View->Update();

  // Listen for the selection changed event
  this->Implementation->View->AddObserver(
    vtkCommand::SelectionChangedEvent, this->Command);
  this->Implementation->View->SetSelectionType(vtkSelectionNode::PEDIGREEIDS);
  emit this->endProgress();
}

ClientGeoView2D::~ClientGeoView2D()
{
  delete this->Implementation;
  this->Command->Delete();
}

void ClientGeoView2D::selectionChanged()
{
  ClientGeoView2D::implementation::RepresentationsT::iterator it, itEnd;
  it = this->Implementation->Representations.begin();
  itEnd = this->Implementation->Representations.end();
  for(; it != itEnd; ++it)
    {
    pqRepresentation* const representation = it->first;
    if(!representation->isVisible())
      {
      continue;
      }
    vtkRenderedGraphRepresentation* const rep = it->second;

    // Get the representaion's source
    pqDataRepresentation* pqRepr =
      qobject_cast<pqDataRepresentation*>(representation);
    pqOutputPort* opPort = pqRepr->getOutputPortFromInput();
    vtkSMSourceProxy* repSource = vtkSMSourceProxy::SafeDownCast(
      opPort->getSource()->getProxy());

    // Fill the selection source with the selection from the view
    rep->GetSelectionLink()->Update();
    vtkSelection* sel = rep->GetSelectionLink()->GetOutput();
    vtkSMSourceProxy* selectionSource =
      pqSelectionManager::createSelectionSource(sel, repSource->GetConnectionID());

    // Set the selection on the representation's source
    repSource->SetSelectionInput(opPort->getPortNumber(),
      selectionSource, 0);
    selectionSource->Delete();
  }
}

QWidget* ClientGeoView2D::getWidget()
{
  return this->Implementation->Widget;
}

bool ClientGeoView2D::canDisplay(pqOutputPort* output_port) const
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

bool ClientGeoView2D::saveImage(int, int, const QString& )
{
  return false;
}

vtkImageData* ClientGeoView2D::captureImage(int)
{
  return 0;
}

void ClientGeoView2D::onRepresentationVisibilityChanged(pqRepresentation* pqrep, bool visible)
{
  // Don't call any updates on anything -- neither the view, nor the
  // representation. ParaView will handle that.
  ClientGeoView2D::implementation::RepresentationsT::iterator iter =
    this->Implementation->Representations.find(pqrep);
  if (iter == this->Implementation->Representations.end())
    {
    // The pqrep has not corresponding vtkrep. This can happen when a new pqrep
    // is added to the view. We create a new vtkrep for it.
    vtkSMClientDeliveryRepresentationProxy* const proxy = 
      vtkSMClientDeliveryRepresentationProxy::SafeDownCast(
        pqrep->getProxy());
    if (!proxy || !proxy->GetOutputPort())
      {
      // sanity check. Don't see how this can really happen.
      return;
      }

    vtkRenderedGraphRepresentation* vtkrep = vtkRenderedGraphRepresentation::New();
    vtkrep->SetInputConnection(proxy->GetOutputPort());
    vtkrep->SetEdgeLayoutStrategyToArcParallel();
    this->Implementation->Representations[pqrep] = vtkrep;
    vtkrep->Delete();
    iter = this->Implementation->Representations.find(pqrep);
    }

  if (visible)
    {
    this->Implementation->View->AddRepresentation(iter->second);
    }
  else
    {
    this->Implementation->View->RemoveRepresentation(iter->second);
    }
}

void ClientGeoView2D::onRepresentationRemoved(pqRepresentation* representation)
{
  ClientGeoView2D::implementation::RepresentationsT::iterator iter = 
    this->Implementation->Representations.find(representation);
  if (iter != this->Implementation->Representations.end())
    {
    this->Implementation->View->RemoveRepresentation(iter->second);
    this->Implementation->Representations.erase(iter);
    }
}

void ClientGeoView2D::renderGeoViewInternal()
{
  ClientGeoView2D::implementation::RepresentationsT::const_iterator it, itEnd;
  it = this->Implementation->Representations.begin();
  itEnd = this->Implementation->Representations.end();
  for(; it != itEnd; ++it)
    {
    pqRepresentation* const representation = it->first;
    vtkRenderedGraphRepresentation* const rep = it->second;
    vtkSMSelectionDeliveryRepresentationProxy* const proxy = 
      vtkSMSelectionDeliveryRepresentationProxy::SafeDownCast(
        representation->getProxy());

    // Update the selection.
    proxy->GetSelectionRepresentation()->Update();
    vtkSelection* sel = vtkSelection::SafeDownCast(
      proxy->GetSelectionRepresentation()->GetOutput());
    rep->GetSelectionLink()->SetSelection(sel);

    // Update the current domain map.
    int useDomainMap = vtkSMPropertyHelper(proxy, "UseDomainMap").GetAsInt();
    rep->GetSelectionLink()->RemoveAllDomainMaps();
    if (useDomainMap)
      {
      vtkSMSourceProxy* domainMap = 0;
      if (vtkSMPropertyHelper(proxy, "DomainMap").GetNumberOfElements() > 0)
        {
        domainMap = vtkSMSourceProxy::SafeDownCast(
          vtkSMPropertyHelper(proxy, "DomainMap").GetAsProxy());
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
          rep->GetSelectionLink()->AddDomainMap(output);
          }
        }
      }

    vtkStdString latArr = pqPropertyHelper(proxy, "LatitudeArrayName").GetAsString().toStdString();
    vtkStdString lonArr = pqPropertyHelper(proxy, "LongitudeArrayName").GetAsString().toStdString();
    rep->SetLayoutStrategyToAssignCoordinates(lonArr, latArr);
    bool vertexLabelVis = vtkSMPropertyHelper(proxy, "VertexLabelVisibility").GetAsInt();
    rep->SetVertexLabelVisibility(vertexLabelVis);
    if (vertexLabelVis)
      {
      vtkStdString vertexLabelArr = pqPropertyHelper(proxy, "VertexLabelArrayName").GetAsString().toStdString();
      rep->SetVertexLabelArrayName(vertexLabelArr);
      }

    rep->GetVertexLabelTextProperty()->SetFontSize(
      vtkSMPropertyHelper(proxy, "VertexLabelFontSize").GetAsInt());

    this->Implementation->Theme->SetPointSize(
      vtkSMPropertyHelper(proxy, "VertexSize").GetAsDouble());

    this->Implementation->Theme->SetPointOpacity(
      vtkSMPropertyHelper(proxy, "VertexOpacity").GetAsDouble());

    if(vtkSMPropertyHelper(proxy, "VertexColorByArray").GetAsInt())
      {
      rep->SetColorVerticesByArray(true);

      rep->SetVertexColorArrayName(
        vtkSMPropertyHelper(proxy, "VertexColorArray").GetAsString());
      }
    else
      {
      rep->SetColorVerticesByArray(false);

      this->Implementation->Theme->SetPointColor(
        vtkSMPropertyHelper(proxy, "VertexColor").GetAsDouble(0),
        vtkSMPropertyHelper(proxy, "VertexColor").GetAsDouble(1),
        vtkSMPropertyHelper(proxy, "VertexColor").GetAsDouble(2));
      }

    rep->SetEdgeLabelVisibility(
      vtkSMPropertyHelper(proxy, "EdgeLabels").GetAsInt());

    rep->SetEdgeLabelArrayName(
      vtkSMPropertyHelper(proxy, "EdgeLabelArray").GetAsString());

    rep->GetEdgeLabelTextProperty()->SetFontSize(
      vtkSMPropertyHelper(proxy, "EdgeLabelFontSize").GetAsInt());

    this->Implementation->Theme->SetLineWidth(
      vtkSMPropertyHelper(proxy, "EdgeWidth").GetAsDouble());

    this->Implementation->Theme->SetCellOpacity(
      vtkSMPropertyHelper(proxy, "EdgeOpacity").GetAsDouble());

    if(vtkSMPropertyHelper(proxy, "EdgeColorByArray").GetAsInt())
      {
      rep->SetColorEdgesByArray(true);

      rep->SetEdgeColorArrayName(
        vtkSMPropertyHelper(proxy, "EdgeColorArray").GetAsString());
      }
    else
      {
      rep->SetColorEdgesByArray(false);

      this->Implementation->Theme->SetCellColor(
        vtkSMPropertyHelper(proxy, "EdgeColor").GetAsDouble(0),
        vtkSMPropertyHelper(proxy, "EdgeColor").GetAsDouble(1),
        vtkSMPropertyHelper(proxy, "EdgeColor").GetAsDouble(2));
      }

    rep->ApplyViewTheme(this->Implementation->Theme);
    }

  emit this->beginProgress();
  this->Implementation->Widget->GetRenderWindow()->Render();
  emit this->endProgress();
}

void ClientGeoView2D::onViewProgressEvent(vtkObject* obj,
  unsigned long vtk_event, void*, void* call_data)
{
  if (vtk_event == vtkCommand::ViewProgressEvent)
    {
    const vtkView::ViewProgressEventCallData* data =
      reinterpret_cast<const vtkView::ViewProgressEventCallData*>(call_data);

    emit this->progress(QString(data->GetProgressMessage()),
      static_cast<int>(data->GetProgress()*100.0));
    }
  else if (vtk_event == vtkCommand::ProgressEvent)
    {
    emit this->progress(obj->GetClassName(),
      static_cast<int>((*reinterpret_cast<double*>(call_data))*100.0));
    }
}
