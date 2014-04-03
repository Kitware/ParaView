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

#include "pqDataRepresentation.h"
#include "pqEventDispatcher.h"
#include "pqImageUtil.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqQVTKWidget.h"
#include "pqServer.h"
#include "pqSMAdaptor.h"
#include "vtkAnnotationLink.h"
#include "vtkChartXY.h"
#include "vtkCommand.h"
#include "vtkContextView.h"
#include "vtkErrorCode.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSMContextViewProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkVariant.h"

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

private:
  void operator=(const pqContextView::command&);
};

class pqContextView::pqInternal
{
public:
  QPointer<QWidget> Viewport;
  bool InitializedAfterObjectsCreated;
  int SelectionAction;

  pqInternal()
    {
    this->InitializedAfterObjectsCreated=false;
    this->SelectionAction = vtkChart::SELECT_RECTANGLE;
    }
  ~pqInternal()
    {
    delete this->Viewport;
    }

  vtkNew<vtkEventQtSlotConnect> VTKConnect;
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
  // don't use caching for charts since the charts don't seem to render
  // correctly when an overlapping window is present, unlike 3D views.
  vtkwidget->setAutomaticImageCacheEnabled(false);
  vtkwidget->setViewProxy(this->getProxy());
  vtkwidget->setObjectName("Viewport");
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
    vtkContextView* view = proxy->GetContextView();
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
  return vtkSMContextViewProxy::SafeDownCast(this->getProxy())->GetContextView();
}

//-----------------------------------------------------------------------------
vtkSMContextViewProxy* pqContextView::getContextViewProxy() const
{
  return vtkSMContextViewProxy::SafeDownCast(this->getProxy());
}

//-----------------------------------------------------------------------------
bool pqContextView::supportsSelection() const
{
  return true;
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

void pqContextView::selectionChanged()
{
  // Fill the selection source with the selection from the view
  vtkSelection* sel = 0;

  if(vtkChart *chart = vtkChart::SafeDownCast(this->getContextViewProxy()->GetContextItem()))
    {
    sel = chart->GetAnnotationLink()->GetCurrentSelection();
    }

  if(!sel)
    {
    return;
    }
  this->setSelection(sel);
}

void pqContextView::setSelection(vtkSelection* sel)
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

  vtkSMProxy* selectionSource =
    vtkSMSelectionHelper::NewSelectionSourceFromSelection(
      repSource->GetSession(), sel);

  // Set the selection on the representation's source
  repSource->CleanSelectionInputs(opPort->getPortNumber());
  repSource->SetSelectionInput(opPort->getPortNumber(),
    vtkSMSourceProxy::SafeDownCast(selectionSource), 0);
  selectionSource->Delete();

  emit this->selected(opPort);
}

void pqContextView::setSelectionAction(int selAction)
{
  if (this->Internal->SelectionAction == selAction ||
    selAction < vtkChart::SELECT ||
    selAction > vtkChart::SELECT_POLYGON)
    {
    return;
    }
  this->Internal->SelectionAction = selAction;
}

int pqContextView::selectionAction()
{
  return this->Internal->SelectionAction;
}
