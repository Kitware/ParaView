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

#include "ClientGraphView.h"

#include <QVTKWidget.h>
#include <vtkAnnotationLayers.h>
#include <vtkAnnotationLink.h>
#include <vtkCommand.h>
#include <vtkDataObject.h>
#include <vtkDataObjectTypes.h>
#include <vtkDataRepresentation.h>
#include <vtkErrorCode.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkGraphLayoutView.h>
#include <vtkGraphLayoutStrategy.h>
#include <vtkIdTypeArray.h>
#include <vtkProcessModule.h>
#include <vtkPVDataInformation.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkTable.h>
#include <vtkTexture.h>
#include <vtkVariantArray.h>
#include <vtkViewTheme.h>
#include <vtkWindowToImageFilter.h>

#include <vtkSMIdTypeVectorProperty.h>
#include <vtkSMInputProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMEnumerationDomain.h>
#include <vtkSMProxy.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMSelectionDeliveryRepresentationProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkSMViewProxy.h>

#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqGraphLayoutStrategyInterface.h>
#include "pqImageUtil.h"
#include <pqPluginManager.h>
#include <pqPropertyHelper.h>
#include <pqOutputPort.h>
#include <pqPipelineSource.h>
#include <pqRepresentation.h>
#include <pqSelectionManager.h>
#include <pqServer.h>
#include <pqSettings.h>

#include <QColor>
#include <QVBoxLayout>
#include <QtDebug>

////////////////////////////////////////////////////////////////////////////////////
// ClientGraphView::command

class ClientGraphView::command : public vtkCommand
{
public:
  static command* New(ClientGraphView& view)
  {
    return new command(view);
  }
  command(ClientGraphView& view) : Target(view) { }
  virtual void Execute(vtkObject*, unsigned long, void*)
  {
    Target.selectionChanged();
  }
  ClientGraphView& Target;
};

////////////////////////////////////////////////////////////////////////////////////
// ClientGraphView::implementation

class ClientGraphView::implementation
{
public:
  implementation() :
    Widget(new QVTKWidget()),
    ResetCamera(false),
    EdgeColorsFlipped(0),
    VisibleMTime(0)
  {

    new QVBoxLayout(this->Widget);
    this->Widget->GetInteractor()->EnableRenderOff();

    this->Theme.TakeReference(vtkViewTheme::CreateNeonTheme());
    this->View = vtkSmartPointer<vtkGraphLayoutView>::New();
    this->View->SetLayoutStrategyToFast2D();
    this->Widget->SetRenderWindow(this->View->GetRenderWindow());
    this->View->ApplyViewTheme(this->Theme);
    
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  }

  ~implementation()
  {
    if(this->Widget)
      delete this->Widget;
  }

  const QPointer<QVTKWidget> Widget;
  vtkSmartPointer<vtkViewTheme> Theme;
  vtkSmartPointer<vtkGraphLayoutView> View;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;

  bool ResetCamera;
  int EdgeColorsFlipped;
  unsigned long VisibleMTime;
};

////////////////////////////////////////////////////////////////////////////////////
// ClientGraphView

ClientGraphView::ClientGraphView(
  const QString& viewmoduletype, 
  const QString& group, 
  const QString& name, 
  vtkSMViewProxy* viewmodule, 
  pqServer* server, 
  QObject* p) :
  pqSingleInputView(viewmoduletype, group, name, viewmodule, server, p),
  Implementation(new implementation())
{
  this->Command = command::New(*this);
  this->Implementation->View->AddObserver(
    vtkCommand::SelectionChangedEvent, this->Command);

  // Listen to all views that may fire progress events during updating.
  this->Implementation->VTKConnect->Connect(
    this->Implementation->View, vtkCommand::ViewProgressEvent,
    this, SLOT(onViewProgressEvent(vtkObject*, unsigned long, void*, void*)));

  this->Implementation->VTKConnect->Connect(
    this->Implementation->View->GetInteractor(), vtkCommand::RenderEvent,
    this, SLOT(forceRender()));

  this->Implementation->View->Render();
}

ClientGraphView::~ClientGraphView()
{
  delete this->Implementation;
  this->Command->Delete();
}

void ClientGraphView::selectionChanged()
{
  // Get the representaion's source
  pqDataRepresentation* pqRepr =
    qobject_cast<pqDataRepresentation*>(this->visibleRepresentation());
  if (!pqRepr)
    {
    return;
    }
  pqOutputPort* opPort = pqRepr->getOutputPortFromInput();
  vtkSMSourceProxy* repSource = vtkSMSourceProxy::SafeDownCast(
    opPort->getSource()->getProxy());

  // Fill the selection source with the selection from the view
  vtkSelection* sel = this->Implementation->View->GetRepresentation()->
    GetAnnotationLink()->GetCurrentSelection();
  vtkSMSourceProxy* selectionSource = pqSelectionManager::createSelectionSource(
    sel, repSource->GetConnectionID());

  // Set the selection on the representation's source
  repSource->SetSelectionInput(opPort->getPortNumber(),
    selectionSource, 0);
  selectionSource->Delete();

  // Mark the annotation link as modified so it will be updated
  if (this->getAnnotationLink())
    {
    this->getAnnotationLink()->MarkModified(0);
    }
}

QWidget* ClientGraphView::getWidget()
{
  return this->Implementation->Widget;
}

bool ClientGraphView::canDisplay(pqOutputPort* output_port) const
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
    case VTK_SELECTION:
      return true;
    }

  return false;
}

vtkImageData* ClientGraphView::captureImage(int magnification)
{

  // Offscreen rendering is not functioning properly on the mac.
  // Do not use it.

#if !defined(__APPLE__)
  //int useOffscreenRenderingForScreenshots = this->UseOffscreenRenderingForScreenshots;
  int prevOffscreen = this->Implementation->View->GetRenderWindow()->GetOffScreenRendering();
  if (!prevOffscreen) //useOffscreenRenderingForScreenshots && !prevOffscreen)
    {
    this->Implementation->View->GetRenderWindow()->SetOffScreenRendering(1);
    }
#endif

  vtkRenderWindow *renWin = this->Implementation->View->GetRenderWindow();

  renWin->SwapBuffersOff();
  //this->getViewProxy()->StillRender();
  this->Implementation->View->Render();

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
    this->Implementation->View->GetRenderWindow()->SetOffScreenRendering(0);
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

//-----------------------------------------------------------------------------
bool ClientGraphView::saveImage(int width, int height, const QString& filename)
{
  QWidget* vtkwidget = this->getWidget();
  QSize cursize = vtkwidget->size();
  QSize fullsize = QSize(width, height);
  QSize newsize = cursize;
  int magnification = 1;
  if (width>0 && height>0)
    {
    magnification = pqView::computeMagnification(fullsize, newsize);
    vtkwidget->resize(newsize);
    }
  this->render();

  int error_code = vtkErrorCode::UnknownError;
  vtkImageData* vtkimage = this->captureImage(magnification);
  if (vtkimage)
    {
    error_code = pqImageUtil::saveImage(vtkimage, filename);
    vtkimage->Delete();
    }

  switch (error_code)
    {
  case vtkErrorCode::UnrecognizedFileTypeError:
    qCritical() << "Failed to determine file type for file:" 
      << filename.toAscii().data();
    break;

  case vtkErrorCode::NoError:
    // success.
    break;

  default:
    qCritical() << "Failed to save image.";
    }

  if (width>0 && height>0)
    {
    vtkwidget->resize(newsize);
    vtkwidget->resize(cursize);
    this->render();
    }
  return (error_code == vtkErrorCode::NoError);
}

void ClientGraphView::onViewProgressEvent(vtkObject*,
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

void ClientGraphView::showRepresentation(pqRepresentation* representation)
{
  vtkSMClientDeliveryRepresentationProxy* const proxy = vtkSMClientDeliveryRepresentationProxy::SafeDownCast(representation->getProxy());
  vtkDataRepresentation* rep =
    this->Implementation->View->SetRepresentationFromInputConnection(proxy->GetOutputPort());
  rep->SetSelectionType(vtkSelectionNode::PEDIGREEIDS);
  // If we have an associated annotation link proxy, set the client side
  // object as the annotation link on the representation.
  if (this->getAnnotationLink())
    {
    vtkAnnotationLink* link = static_cast<vtkAnnotationLink*>(this->getAnnotationLink()->GetClientSideObject());
    rep->SetAnnotationLink(link);
    }

  this->Implementation->View->Update();
  this->Implementation->ResetCamera = true;
}

void ClientGraphView::hideRepresentation(pqRepresentation* representation)
{
  vtkSMClientDeliveryRepresentationProxy* const proxy = vtkSMClientDeliveryRepresentationProxy::SafeDownCast(representation->getProxy());
  this->Implementation->View->RemoveRepresentation(proxy->GetOutputPort());
}

void ClientGraphView::renderInternal()
{
  unsigned long oldVisibleMTime = this->Implementation->VisibleMTime;

  emit this->beginProgress();
  if (this->visibleRepresentation())
    {
    vtkSMSelectionDeliveryRepresentationProxy* const proxy = 
      vtkSMSelectionDeliveryRepresentationProxy::SafeDownCast(this->visibleRepresentation()->getProxy());

    // Force a camera reset if the input data has been modified ...
    if(proxy->GetOutput()->GetMTime() > this->Implementation->VisibleMTime)
      this->Implementation->ResetCamera = true;

    this->Implementation->VisibleMTime = proxy->GetOutput()->GetMTime();

    // Set the current domain map
    int useDomainMap = vtkSMPropertyHelper(proxy, "UseDomainMap").GetAsInt();
    vtkAnnotationLink* link = this->Implementation->View->GetRepresentation()->GetAnnotationLink();
    link->RemoveAllDomainMaps();
    if (useDomainMap)
      {
      vtkSMSourceProxy* domainMap = 0;
      if (vtkSMPropertyHelper(proxy, "DomainMap").GetNumberOfElements() > 0)
        {
        domainMap = vtkSMSourceProxy::SafeDownCast(vtkSMPropertyHelper(proxy, "DomainMap").GetAsProxy());
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
          link->AddDomainMap(output);
          }
        }
      }

    // Update the selection.
    // Only use the source proxy's selection if we're not using vtkAnnotationLink directly
    if(!this->getAnnotationLink())
      {
      proxy->GetSelectionRepresentation()->Update();
      vtkSelection* sel = vtkSelection::SafeDownCast(
      proxy->GetSelectionRepresentation()->GetOutput());
      this->Implementation->View->GetRepresentation()->GetAnnotationLink()->
        SetCurrentSelection(sel);
      }

    QObjectList ifaces =
      pqApplicationCore::instance()->getPluginManager()->interfaces();
    foreach(QObject* iface, ifaces)
      {
      pqGraphLayoutStrategyInterface* glsi = qobject_cast<pqGraphLayoutStrategyInterface*>(iface);
      if(glsi && glsi->graphLayoutStrategies().contains(vtkSMPropertyHelper(proxy, "LayoutStrategy").GetAsString()))
        {
        glsi->graphLayoutStrategies().contains(vtkSMPropertyHelper(proxy, "LayoutStrategy").GetAsString());
        vtkGraphLayoutStrategy *layout = glsi->getGraphLayoutStrategy(vtkSMPropertyHelper(proxy, "LayoutStrategy").GetAsString());
        // Force a camera reset if the layout strategy has been changed ...
        if( strcmp(layout->GetClassName(), this->Implementation->View->GetLayoutStrategy()->GetClassName()) != 0 )
          this->Implementation->ResetCamera = true;
        layout->SetWeightEdges(vtkSMPropertyHelper(proxy, "WeightEdges").GetAsInt());
        layout->SetEdgeWeightField(vtkSMPropertyHelper(proxy, "EdgeWeightArray").GetAsString());
        this->Implementation->View->SetLayoutStrategy(layout);
        break;
        }
      }

    // Don't reset the camera if the layout is set to PassThrough. 
    // The user may have fixed the layout upstream and doesn't want 
    // a camera reset to occur even if the input data has changed.
    if(strcmp(vtkSMPropertyHelper(proxy, "LayoutStrategy").GetAsString(),"None") == 0)
      this->Implementation->ResetCamera = false;

    this->Implementation->View->SetVertexLabelVisibility(
      vtkSMPropertyHelper(proxy, "VertexLabels").GetAsInt());

    //this->Implementation->View->SetHideVertexLabelsOnInteraction(
    //  vtkSMPropertyHelper(proxy, "AutoHideVertexLabels").GetAsInt());

    this->Implementation->View->SetVertexLabelArrayName(
      vtkSMPropertyHelper(proxy, "VertexLabelArray").GetAsString());

    bool iconVisibility = vtkSMPropertyHelper(proxy, "IconVisibility").GetAsInt();

    this->Implementation->View->SetIconVisibility(
      iconVisibility);

    if(iconVisibility && !pqPropertyHelper(proxy, "IconFile").GetAsString().isEmpty())
      {
      vtkSMProxy *textureProxy = vtkSMPropertyHelper(proxy, "IconTexture").GetAsProxy(0);
      vtkTexture *texture = vtkTexture::SafeDownCast(vtkProcessModule::GetProcessModule()->GetObjectFromID(textureProxy->GetID()));
      this->Implementation->View->SetIconTexture( texture );

      int iconSize[2];
      iconSize[0] = iconSize[1] = vtkSMPropertyHelper(proxy, "IconSize").GetAsInt();
      this->Implementation->View->SetIconSize(iconSize);

      this->Implementation->View->SetIconAlignment(vtkSMPropertyHelper(proxy, "IconAlignment").GetAsInt()+1);

      vtkSMStringVectorProperty *iconTypes = vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("IconTypes"));
      vtkSMIntVectorProperty *iconIndices = vtkSMIntVectorProperty::SafeDownCast(proxy->GetProperty("IconIndices"));
      this->Implementation->View->ClearIconTypes();
      for(unsigned int i=0; i<iconTypes->GetNumberOfElements(); i++)
        {
        this->Implementation->View->AddIconType((char*)iconTypes->GetElement(i), iconIndices->GetElement(i));
        }

      this->Implementation->View->SetIconArrayName(
        vtkSMPropertyHelper(proxy, "IconArray").GetAsString());
      }

    this->Implementation->View->SetVertexLabelFontSize(
      static_cast<int>(vtkSMPropertyHelper(proxy, "VertexLabelFontSize").GetAsDouble()));

    this->Implementation->Theme->SetPointSize(
      vtkSMPropertyHelper(proxy, "VertexSize").GetAsDouble());

    this->Implementation->Theme->SetPointOpacity(
      vtkSMPropertyHelper(proxy, "VertexOpacity").GetAsDouble());

    if(vtkSMPropertyHelper(proxy, "VertexColorByArray").GetAsInt())
      {
      this->Implementation->View->SetColorVertices(true);

      this->Implementation->View->SetVertexColorArrayName(
        vtkSMPropertyHelper(proxy, "VertexColorArray").GetAsString());

      this->Implementation->View->SetVertexScalarBarVisibility(
        vtkSMPropertyHelper(proxy, "VertexScalarBarVisibility").GetAsInt());
      }
    else
      {
      this->Implementation->View->SetColorVertices(false);

      this->Implementation->Theme->SetPointColor(
        vtkSMPropertyHelper(proxy, "VertexColor").GetAsDouble(0),
        vtkSMPropertyHelper(proxy, "VertexColor").GetAsDouble(1),
        vtkSMPropertyHelper(proxy, "VertexColor").GetAsDouble(2));

      this->Implementation->View->SetVertexScalarBarVisibility(false);
      }

    this->Implementation->View->SetEdgeLabelVisibility(
      vtkSMPropertyHelper(proxy, "EdgeLabels").GetAsInt());

    //this->Implementation->View->SetHideEdgeLabelsOnInteraction(
    //  vtkSMPropertyHelper(proxy, "AutoHideEdgeLabels").GetAsInt());

    this->Implementation->View->SetEdgeLabelArrayName(
      vtkSMPropertyHelper(proxy, "EdgeLabelArray").GetAsString());

    this->Implementation->View->SetEdgeLabelFontSize(
      static_cast<int>(vtkSMPropertyHelper(proxy, "EdgeLabelFontSize").GetAsDouble()));

    this->Implementation->Theme->SetLineWidth(
      vtkSMPropertyHelper(proxy, "EdgeWidth").GetAsDouble());

    this->Implementation->Theme->SetCellOpacity(
      vtkSMPropertyHelper(proxy, "EdgeOpacity").GetAsDouble());

    if(vtkSMPropertyHelper(proxy, "EdgeColorByArray").GetAsInt())
      {
      this->Implementation->View->SetColorEdges(true);

      this->Implementation->View->SetEdgeColorArrayName(
        vtkSMPropertyHelper(proxy, "EdgeColorArray").GetAsString());

      this->Implementation->View->SetEdgeScalarBarVisibility(
        vtkSMPropertyHelper(proxy, "EdgeScalarBarVisibility").GetAsInt());

      if(this->Implementation->EdgeColorsFlipped != 
          vtkSMPropertyHelper(proxy, "FlipEdgeColorMap").GetAsInt())
        {
        double edgeHueRange[2];
        double edgeAlphaRange[2];
        double edgeSatRange[2];
        double edgeValueRange[2];
        this->Implementation->Theme->GetCellHueRange(edgeHueRange);
        this->Implementation->Theme->SetCellHueRange(edgeHueRange[1], edgeHueRange[0]);
        this->Implementation->Theme->GetCellAlphaRange(edgeAlphaRange);
        this->Implementation->Theme->SetCellAlphaRange(edgeAlphaRange[1], edgeAlphaRange[0]);
        this->Implementation->Theme->GetCellSaturationRange(edgeSatRange);
        this->Implementation->Theme->SetCellSaturationRange(edgeSatRange[1], edgeSatRange[0]);
        this->Implementation->Theme->GetCellValueRange(edgeValueRange);
        this->Implementation->Theme->SetCellValueRange(edgeValueRange[1], edgeValueRange[0]);
        this->Implementation->EdgeColorsFlipped = 
            vtkSMPropertyHelper(proxy, "FlipEdgeColorMap").GetAsInt();
        }
      }
    else
      {
      this->Implementation->View->SetColorEdges(false);

      this->Implementation->Theme->SetCellColor(
        vtkSMPropertyHelper(proxy, "EdgeColor").GetAsDouble(0),
        vtkSMPropertyHelper(proxy, "EdgeColor").GetAsDouble(1),
        vtkSMPropertyHelper(proxy, "EdgeColor").GetAsDouble(2));

      this->Implementation->View->SetEdgeScalarBarVisibility(false);
      }

    bool arc_edges = static_cast<bool>(vtkSMPropertyHelper(proxy, "ArcEdges").GetAsInt());
    if (arc_edges)
      {
      this->Implementation->View->SetEdgeLayoutStrategyToArcParallel();
      }
    else
      {
      this->Implementation->View->SetEdgeLayoutStrategyToPassThrough();
      }
    }

  this->Implementation->View->ApplyViewTheme(this->Implementation->Theme);

  if(vtkSMPropertyHelper(this->getProxy(),"ZoomToSelection").GetAsInt())
    {
    this->Implementation->View->ZoomToSelection();
    this->Implementation->ResetCamera = false;
    vtkSMPropertyHelper(this->getProxy(),"ZoomToSelection").Set(0);
    }
  else if(vtkSMPropertyHelper(this->getProxy(),"ResetCamera").GetAsInt())
    {
    this->Implementation->ResetCamera = true;
    vtkSMPropertyHelper(this->getProxy(),"ResetCamera").Set(0);
    }

  if(this->Implementation->ResetCamera)
    this->Implementation->View->ResetCamera();

  this->Implementation->ResetCamera = false;

  this->Implementation->View->Render();
  emit this->endProgress();
}

