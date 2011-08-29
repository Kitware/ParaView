/*=========================================================================

   Program: ParaView
   Module:    pqContextView.cxx

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
#include "pqContextView.h"

#include "pqEventDispatcher.h"
#include "pqImageUtil.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqDataRepresentation.h"
#include "pqServer.h"
#include "pqImageUtil.h"
#include "pqSMAdaptor.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"

#include "vtkContextView.h"
#include "vtkSMContextViewProxy.h"
#include "vtkChartXY.h"
#include "vtkAnnotationLink.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkAbstractArray.h"
#include "vtkIdTypeArray.h"
#include "vtkVariant.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMPropertyHelper.h"

#include "vtkCommand.h"

#include "pqQVTKWidget.h"

#include <QList>
#include <QVariant>
#include <QPointer>
#include <QDebug>

// Command implementation
class pqContextView::command : public vtkCommand
{
public:
  static command* New(pqContextView &view)
  {
    return new command(view);
  }

  command(pqContextView &view) : Target(view) { }

  virtual void Execute(vtkObject*, unsigned long, void*)
  {
    Target.selectionChanged();
  }

  pqContextView& Target;
};

class pqContextView::pqInternal
{
public:
  QPointer<QWidget> Viewport;
  bool InitializedAfterObjectsCreated;

  pqInternal()
    {
    this->InitializedAfterObjectsCreated=false;
    }
  ~pqInternal()
    {
    delete this->Viewport;
    }
};

//-----------------------------------------------------------------------------
pqContextView::pqContextView(
  const QString& type, const QString& group,
  const QString& name,
  vtkSMViewProxy* viewProxy,
  pqServer* server,
  QObject* parentObject)
: Superclass(type, group, name, viewProxy, server, parentObject)
{
  this->Internal = new pqContextView::pqInternal();
  viewProxy->UpdateVTKObjects(); // this results in calling CreateVTKObjects().
  this->Command = command::New(*this);
  vtkObject::SafeDownCast(viewProxy->GetClientSideObject())->AddObserver(
    vtkCommand::SelectionChangedEvent, this->Command);
}

//-----------------------------------------------------------------------------
pqContextView::~pqContextView()
{
  this->Command->Delete();
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QWidget* pqContextView::createWidget()
{
  pqQVTKWidget* vtkwidget = new pqQVTKWidget();
  vtkwidget->setViewProxy(this->getProxy());
  vtkwidget->setObjectName("Viewport");

  // do image caching for performance
  // For now, we are doing this only on Apple because it can render
  // and capture a frame buffer even when it is obstructred by a
  // window. This does not work as well on other platforms.
#if defined(__APPLE__)
  vtkwidget->setAutomaticImageCacheEnabled(true);

  // help the QVTKWidget know when to clear the cache
  this->getConnector()->Connect(
    this->getProxy(), vtkCommand::ModifiedEvent,
    vtkwidget, SLOT(markCachedImageAsDirty()));
#endif

  return vtkwidget;
}

//-----------------------------------------------------------------------------
void pqContextView::initialize()
{
  this->Superclass::initialize();

  // The render module needs to obtain client side objects
  // for the RenderWindow etc. to initialize the QVTKWidget
  // correctly. It cannot do this unless the underlying proxy
  // has been created. Since any pqProxy should never call
  // UpdateVTKObjects() on itself in the constructor, we
  // do the following.
  vtkSMProxy* proxy = this->getProxy();
  if (!proxy->GetObjectsCreated())
    {
    // Wait till first UpdateVTKObjects() call on the render module.
    // Under usual circumstances, after UpdateVTKObjects() the
    // render module objects will be created.
    this->getConnector()->Connect(proxy, vtkCommand::UpdateEvent,
      this, SLOT(initializeAfterObjectsCreated()));
    }
  else
    {
    this->initializeAfterObjectsCreated();
    }
}

//-----------------------------------------------------------------------------
void pqContextView::initializeAfterObjectsCreated()
{
  if (!this->Internal->InitializedAfterObjectsCreated)
    {
    this->Internal->InitializedAfterObjectsCreated = true;
    // Initialize the interactors and all global settings. global-settings
    // override the values specified in state files or through python client.
    this->initializeInteractors();
    }
}

//-----------------------------------------------------------------------------
void pqContextView::initializeInteractors()
{
  vtkSMContextViewProxy* proxy =
    vtkSMContextViewProxy::SafeDownCast(this->getProxy());
  QVTKWidget* qvtk = qobject_cast<QVTKWidget*>(this->Internal->Viewport);

  if(proxy && qvtk)
    {
    vtkContextView* view = proxy->GetChartView();
    view->SetInteractor(qvtk->GetInteractor());
    qvtk->SetRenderWindow(view->GetRenderWindow());
    }
}

//-----------------------------------------------------------------------------
/// Return a widget associated with this view.
QWidget* pqContextView::getWidget()
{
  if(!this->Internal->Viewport)
    {
    this->Internal->Viewport = this->createWidget();
    // we manage the context menu ourself, so it doesn't interfere with
    // render window interactions
    this->Internal->Viewport->setContextMenuPolicy(Qt::NoContextMenu);
    this->Internal->Viewport->installEventFilter(this);
    this->Internal->Viewport->setObjectName("Viewport");
    this->initializeInteractors();
    }
  return this->Internal->Viewport;
}

//-----------------------------------------------------------------------------
/// Returns the internal vtkChartView that provides the implementation for
/// the chart rendering.
vtkContextView* pqContextView::getVTKContextView() const
{
  return vtkSMContextViewProxy::SafeDownCast(this->getProxy())->GetChartView();
}

//-----------------------------------------------------------------------------
vtkSMContextViewProxy* pqContextView::getContextViewProxy() const
{
  return vtkSMContextViewProxy::SafeDownCast(this->getProxy());
}

//-----------------------------------------------------------------------------
/// This view does not support saving to image.
bool pqContextView::saveImage(int width, int height, const QString& filename)
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

//-----------------------------------------------------------------------------
/// Capture the view image into a new vtkImageData with the given magnification
/// and returns it. The caller is responsible for freeing the returned image.
vtkImageData* pqContextView::captureImage(int magnification)
{
  if (this->getWidget()->isVisible())
    {
    return this->getContextViewProxy()->CaptureWindow(magnification);
    }

  // Don't return any image when the view is not visible.
  return NULL;
}

//-----------------------------------------------------------------------------
/// Called to undo interaction.
void pqContextView::undo()
{
}

//-----------------------------------------------------------------------------
/// Called to redo interaction.
void pqContextView::redo()
{
}

//-----------------------------------------------------------------------------
/// Returns true if undo can be done.
bool pqContextView::canUndo() const
{
  return false;
}

//-----------------------------------------------------------------------------
/// Returns true if redo can be done.
bool pqContextView::canRedo() const
{
  return false;
}

//-----------------------------------------------------------------------------
/// Resets the zoom level to 100%.
void pqContextView::resetDisplay()
{
  vtkSMContextViewProxy *proxy = this->getContextViewProxy();
  if (proxy)
    {
    proxy->ResetDisplay();
    }
}

//-----------------------------------------------------------------------------
/// Returns true if data on the given output port can be displayed by this view.
bool pqContextView::canDisplay(pqOutputPort* opPort) const
{
  if(this->Superclass::canDisplay(opPort))
    {
    return true;
    }

  pqPipelineSource* source = opPort? opPort->getSource() :0;
  vtkSMSourceProxy* sourceProxy = source ?
    vtkSMSourceProxy::SafeDownCast(source->getProxy()) : 0;
  if(!opPort || !source ||
     opPort->getServer()->GetConnectionID() !=
     this->getServer()->GetConnectionID() || !sourceProxy ||
     sourceProxy->GetOutputPortsCreated()==0)
    {
    return false;
    }

  if (sourceProxy->GetHints() &&
    sourceProxy->GetHints()->FindNestedElementByName("Plotable"))
    {
    return true;
    }

  vtkPVDataInformation* dataInfo = opPort->getDataInformation();
  if ( !dataInfo )
    {
    return false;
    }

  QString className = dataInfo->GetDataClassName();
  if( className == "vtkTable" )
    {
    return true;
    }
  else if(className == "vtkImageData" || className == "vtkRectilinearGrid")
    {
    int extent[6];
    dataInfo->GetExtent(extent);
    int temp[6]={0, 0, 0, 0, 0, 0};
    int dimensionality = vtkStructuredData::GetDataDimension(
      vtkStructuredData::SetExtent(extent, temp));
    if (dimensionality == 1)
      {
      return true;
      }
    }
  return false;
}

void pqContextView::selectionChanged()
{
  // Get the representation's source
  pqDataRepresentation* pqRepr = 0;

  for (int i = 0; i < this->getNumberOfRepresentations(); ++i)
    {
    if (this->getRepresentation(i)->isVisible())
      {
      pqRepr = qobject_cast<pqDataRepresentation*>(this->getRepresentation(i));
      }
    }

  if (!pqRepr)
    {
    return;
    }

  pqOutputPort* opPort = pqRepr->getOutputPortFromInput();
  vtkSMSourceProxy* repSource = vtkSMSourceProxy::SafeDownCast(
    opPort->getSource()->getProxy());
  vtkSMSourceProxy* selectionSource = opPort->getSelectionInput();

  int selectionType = vtkSelectionNode::POINT;
  if (QString(opPort->getDataClassName()) == "vtkTable")
    {
    selectionType = vtkSelectionNode::ROW;
    }

  if (!selectionSource)
    {
    vtkSMProxyManager* pxm = this->proxyManager();
    selectionSource =
      vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("sources", "IDSelectionSource"));
    vtkSMPropertyHelper(selectionSource, "FieldType").Set(selectionType);
    selectionSource->UpdateVTKObjects();
    }
  else
    {
    selectionSource->Register(repSource);
    }

  // Fill the selection source with the selection from the view
  vtkSelection* sel = this->getContextViewProxy()->GetChart()
                      ->GetAnnotationLink()->GetCurrentSelection();

  // Fill the selection source with the selection
  vtkSMVectorProperty* vp = vtkSMVectorProperty::SafeDownCast(
    selectionSource->GetProperty("IDs"));
  QList<QVariant> ids = pqSMAdaptor::getMultipleElementProperty(vp);

  vtkSelectionNode* node = 0;

  if (sel->GetNumberOfNodes())
    {
    node = sel->GetNode(0);
    }
  else
    {
    node = vtkSelectionNode::New();
    sel->AddNode(node);
    node->Delete();
    }

  vtkIdTypeArray *arr = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
  ids.clear();
  if (arr)
    {
    for (vtkIdType i = 0; i < arr->GetNumberOfTuples(); ++i)
      {
      ids.push_back(-1);
      ids.push_back(arr->GetValue(i));
      }
    }

  pqSMAdaptor::setMultipleElementProperty(vp, ids);
  selectionSource->UpdateVTKObjects();

  // Set the selection on the representation's source
  repSource->CleanSelectionInputs(opPort->getPortNumber());
  repSource->SetSelectionInput(opPort->getPortNumber(), selectionSource, 0);
  selectionSource->Delete();

  emit this->selected(opPort);
}
