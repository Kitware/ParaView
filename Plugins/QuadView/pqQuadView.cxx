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
#include "pqQuadView.h"

#include "pqCoreUtilities.h"
#include "pqProxy.h"
#include "pqQVTKWidget.h"
#include "pqRepresentation.h"
#include "pqUndoStack.h"
#include "vtkPVDataInformation.h"
#include "vtkPVQuadRenderView.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMViewProxy.h"

#include <QGridLayout>
#include <QWidget>

namespace
{
  /// override QWidget to update the "ViewSize" property whenever the widget
  /// resizes.
  class pqSizableWidget : public QWidget
  {
  typedef QWidget Superclass;
  vtkWeakPointer<vtkSMProxy> Proxy;
public:
  pqSizableWidget(vtkSMProxy* proxy) : Proxy(proxy) { }
protected:
  virtual void resizeEvent(QResizeEvent* evt)
    {
    this->Superclass::resizeEvent(evt);
    if (this->Proxy)
      {
      BEGIN_UNDO_EXCLUDE();
      int view_size[2];
      view_size[0] = this->size().width();
      view_size[1] = this->size().height();
      vtkSMPropertyHelper(this->Proxy, "ViewSize").Set(view_size, 2);
      this->Proxy->UpdateProperty("ViewSize");
      END_UNDO_EXCLUDE();
      }
    }
  };
}

//-----------------------------------------------------------------------------
pqQuadView::pqQuadView(
   const QString& group, const QString& name,
    vtkSMProxy* viewProxy, pqServer* server, QObject* p)
  : Superclass(pqQuadView::quadViewType(), group, name,
    vtkSMViewProxy::SafeDownCast(viewProxy), server, p)
{
  this->ObserverId =
      pqCoreUtilities::connect(
        viewProxy->GetProperty("SlicesCenter"), vtkCommand::ModifiedEvent,
        this, SIGNAL(fireSliceOriginChanged()));

  for(int i=0; i < 21; ++i)
    {
    this->DataHolder[i] = 0.0;
    }

  QObject::connect( this, SIGNAL(representationAdded(pqRepresentation*)),
                    this, SLOT(resetSliceOrigin()));
  QObject::connect( this, SIGNAL(representationVisibilityChanged(pqRepresentation*,bool)),
                    this, SLOT(resetSliceOrigin()));
  QObject::connect( this, SIGNAL(representationRemoved(pqRepresentation*)),
                    this, SLOT(resetSliceOrigin()));
}

//-----------------------------------------------------------------------------
pqQuadView::~pqQuadView()
{
}

//-----------------------------------------------------------------------------
QWidget* pqQuadView::createWidget()
{
  vtkSMProxy* viewProxy = this->getProxy();
  vtkPVQuadRenderView* clientView = vtkPVQuadRenderView::SafeDownCast(
    viewProxy->GetClientSideObject());

  QWidget* container = new pqSizableWidget(viewProxy);
  container->setObjectName("QuadView");
  container->setStyleSheet("background-color: white");
  container->setAutoFillBackground(true);

  QGridLayout* gLayout = new QGridLayout(container);
  gLayout->setSpacing(2);
  gLayout->setContentsMargins(0,0,0,0);

  pqQVTKWidget* vtkwidget = new pqQVTKWidget();
  vtkwidget->setSizePropertyName("ViewSizeTopLeft");
  vtkwidget->setViewProxy(viewProxy);
  vtkwidget->SetRenderWindow(clientView->GetOrthoViewWindow(vtkPVQuadRenderView::TOP_LEFT));
  gLayout->addWidget(vtkwidget, 0, 0);

  vtkwidget = new pqQVTKWidget();
  vtkwidget->setSizePropertyName("ViewSizeBottomLeft");
  vtkwidget->setViewProxy(viewProxy);
  vtkwidget->SetRenderWindow(clientView->GetOrthoViewWindow(vtkPVQuadRenderView::BOTTOM_LEFT));
  gLayout->addWidget(vtkwidget, 1, 0);

  vtkwidget = new pqQVTKWidget();
  vtkwidget->setSizePropertyName("ViewSizeTopRight");
  vtkwidget->setViewProxy(viewProxy);
  vtkwidget->SetRenderWindow(clientView->GetOrthoViewWindow(vtkPVQuadRenderView::TOP_RIGHT));
  gLayout->addWidget(vtkwidget, 0, 1);

  vtkwidget = qobject_cast<pqQVTKWidget*>(this->Superclass::createWidget());
  vtkwidget->setParent(container);
  vtkwidget->setSizePropertyName("ViewSizeBottomRight");
  vtkwidget->setObjectName("View3D");
  vtkwidget->SetRenderWindow(clientView->GetRenderWindow());
  gLayout->addWidget(vtkwidget, 1, 1);

  return container;
}
//-----------------------------------------------------------------------------
const double* pqQuadView::getVector(const char* propertyName, int offset)
{
  std::vector<double> values =
      vtkSMPropertyHelper(this->getViewProxy(), propertyName).GetDoubleArray();
  for(int i=0;i<3;++i)
    {
    this->DataHolder[i+ (3*offset)] = values[i];
    }
  return &this->DataHolder[(3*offset)];
}

//-----------------------------------------------------------------------------
const double* pqQuadView::setVector(const char* propertyName, int offset, double x, double y, double z)
{
  this->DataHolder[0 + (3*offset)] = x;
  this->DataHolder[1 + (3*offset)] = y;
  this->DataHolder[2 + (3*offset)] = z;

  vtkSMPropertyHelper(this->getViewProxy(), propertyName).Set(&this->DataHolder[(3*offset)], 3);
  this->getViewProxy()->UpdateVTKObjects();

  return &this->DataHolder[(3*offset)];
}

//-----------------------------------------------------------------------------
const double* pqQuadView::getTopLeftNormal()
{
  return this->getVector("XSlicesNormal", 1);
}
//-----------------------------------------------------------------------------
const double* pqQuadView::getTopRightNormal()
{
  return this->getVector("YSlicesNormal",2);
}
//-----------------------------------------------------------------------------
const double* pqQuadView::getBottomLeftNormal()
{
  return this->getVector("ZSlicesNormal",3);
}
//-----------------------------------------------------------------------------
const double* pqQuadView::getTopLeftViewUp()
{
  return this->getVector("TopLeftViewUp",4);
}
//-----------------------------------------------------------------------------
const double* pqQuadView::getTopRightViewUp()
{
  return this->getVector("TopRightViewUp",5);
}
//-----------------------------------------------------------------------------
const double* pqQuadView::getBottomLeftViewUp()
{
  return this->getVector("BottomLeftViewUp",6);
}
//-----------------------------------------------------------------------------
const double* pqQuadView::getSlicesOrigin()
{
  return this->getVector("SlicesCenter",0);
}
//-----------------------------------------------------------------------------
void pqQuadView::setTopLeftNormal(double x, double y, double z)
{
  this->setVector("XSlicesNormal", 1, x, y, z);
}
//-----------------------------------------------------------------------------
void pqQuadView::setTopRightNormal(double x, double y, double z)
{
  this->setVector("YSlicesNormal", 2, x, y, z);
}
//-----------------------------------------------------------------------------
void pqQuadView::setBottomLeftNormal(double x, double y, double z)
{
  this->setVector("ZSlicesNormal", 3, x, y, z);
}
//-----------------------------------------------------------------------------
void pqQuadView::setTopLeftViewUp(double x, double y, double z)
{
  this->setVector("TopLeftViewUp", 4, x, y, z);
}
//-----------------------------------------------------------------------------
void pqQuadView::setTopRightViewUp(double x, double y, double z)
{
  this->setVector("TopRightViewUp", 5, x, y, z);
}
//-----------------------------------------------------------------------------
void pqQuadView::setBottomLeftViewUp(double x, double y, double z)
{
  this->setVector("BottomLeftViewUp", 6, x, y, z);
}
//-----------------------------------------------------------------------------
void pqQuadView::setSlicesOrigin(double x, double y, double z)
{
  this->setVector("SlicesCenter", 0, x, y, z);
}

//-----------------------------------------------------------------------------
void pqQuadView::resetDefaultSettings()
{
  double value = 0;
  vtkSMPropertyHelper(this->getViewProxy(), "XSlicesValues").Set(value);
  vtkSMPropertyHelper(this->getViewProxy(), "YSlicesValues").Set(value);
  vtkSMPropertyHelper(this->getViewProxy(), "YSlicesValues").Set(value);
  this->setSlicesOrigin(0,0,0);
  this->setTopLeftNormal(1,0,0);
  this->setTopRightNormal(0,1,0);
  this->setBottomLeftNormal(0,0,1);
  this->setTopLeftViewUp(0,1,0);
  this->setTopRightViewUp(-1,0,0);
  this->setBottomLeftViewUp(0,1,0);
}

//-----------------------------------------------------------------------------
void pqQuadView::resetSliceOrigin()
{
  // We only reset slice origin when only one representation is registered and visible
  if( this->getRepresentations().size() == 1 &&
      this->getNumberOfVisibleRepresentations() == 1)
    {
    vtkSMRepresentationProxy* representation =
        vtkSMRepresentationProxy::SafeDownCast(
          this->getRepresentation(0)->getProxy());
    double * bounds =
        representation->GetRepresentedDataInformation()->GetBounds();

    double center[3];
    for(int i=0; i<3; ++i)
      {
      center[i] = (bounds[2*i] + bounds[2*i+1])/2;
      }

    this->setSlicesOrigin(center[0],center[1],center[2]);
    }
}

//-----------------------------------------------------------------------------
void pqQuadView::setCursor(const QCursor &c)
{
  if(this->getWidget())
    {
    pqQVTKWidget* widget = this->getWidget()->findChild<pqQVTKWidget*>("View3D");
     if(widget)
       {
       widget->setCursor(c);
       }
    }
}

//-----------------------------------------------------------------------------
void pqQuadView::setLabelFontSize(int size)
{
  vtkSMPropertyHelper(this->getViewProxy(), "LabelFontSize").Set(size);
  this->getViewProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int pqQuadView::getLabelFontSize()
{
  return vtkSMPropertyHelper(this->getViewProxy(), "LabelFontSize").GetAsInt();
}

//-----------------------------------------------------------------------------
bool pqQuadView::getCubeAxesVisibility()
{
  return vtkSMPropertyHelper(this->getViewProxy(), "ShowCubeAxes").GetAsInt() != 0;
}

//-----------------------------------------------------------------------------
void pqQuadView::setCubeAxesVisibility(bool visible)
{
  vtkSMPropertyHelper(this->getViewProxy(), "ShowCubeAxes").Set(visible ? 1 : 0);
  this->getViewProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
bool pqQuadView::getOutlineVisibility()
{
  return vtkSMPropertyHelper(this->getViewProxy(), "ShowOutline").GetAsInt() != 0;
}

//-----------------------------------------------------------------------------
void pqQuadView::setOutlineVisibility(bool visible)
{
  vtkSMPropertyHelper(this->getViewProxy(), "ShowOutline").Set(visible ? 1 : 0);
  this->getViewProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
bool pqQuadView::getSliceOrientationAxesVisibility()
{
  return vtkSMPropertyHelper(this->getViewProxy(), "SliceOrientationAxesVisibility").GetAsInt() != 0;
}

//-----------------------------------------------------------------------------
void pqQuadView::setSliceOrientationAxesVisibility(bool visible)
{
  vtkSMPropertyHelper(this->getViewProxy(), "SliceOrientationAxesVisibility").Set(visible ? 1 : 0);
  this->getViewProxy()->UpdateVTKObjects();
}
