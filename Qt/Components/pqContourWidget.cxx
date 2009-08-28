/*=========================================================================

   Program: ParaView
   Module:    pqContourWidget.cxx

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
#include "pqContourWidget.h"
#include "ui_pqContourWidget.h"

#include "pq3DWidgetFactory.h"
#include "pqApplicationCore.h"
#include "pqPropertyLinks.h"
#include "pqServerManagerModel.h"
#include "pqSignalAdaptorTreeWidget.h"
#include "pqSMAdaptor.h"

#include <QDoubleValidator>
#include <QShortcut>
#include <QtDebug>

#include "vtkSmartPointer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"

#include "vtkClientServerStream.h"

class pqContourWidget::pqInternals : public Ui::ContourWidget
{
public:
  pqPropertyLinks Links;
};

//-----------------------------------------------------------------------------
pqContourWidget::pqContourWidget(
  vtkSMProxy* _smproxy, vtkSMProxy* pxy, QWidget* p) :
  Superclass(_smproxy, pxy, p)
{
  this->Internals = new pqInternals();
  this->Internals->setupUi(this);

  this->Internals->Visibility->setChecked(this->widgetVisible());
  QObject::connect(this, SIGNAL(widgetVisibilityChanged(bool)),
    this->Internals->Visibility, SLOT(setChecked(bool)));

  QObject::connect(this->Internals->Visibility,
    SIGNAL(toggled(bool)), this, SLOT(setWidgetVisible(bool)));

  QObject::connect(&this->Internals->Links,
    SIGNAL(qtWidgetChanged()),
    this, SLOT(setModified()));

  QObject::connect(&this->Internals->Links,
    SIGNAL(qtWidgetChanged()),
    this, SLOT(render()));

  QObject::connect(this->Internals->Delete, SIGNAL(clicked()),
    this, SLOT(removeAllNodes()));

  pqServerManagerModel* smmodel =
    pqApplicationCore::instance()->getServerManagerModel();
  this->createWidget(smmodel->findServer(_smproxy->GetConnectionID()));
}

//-----------------------------------------------------------------------------
pqContourWidget::~pqContourWidget()
{
  this->cleanupWidget();
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqContourWidget::createWidget(pqServer* server)
{
  vtkSMNewWidgetRepresentationProxy* widget =
    pqApplicationCore::instance()->get3DWidgetFactory()->
    get3DWidget("ContourWidgetRepresentation", server);
  this->setWidgetProxy(widget);
  
  widget->UpdateVTKObjects();
  widget->UpdatePropertyInformation();

  this->Internals->Links.addPropertyLink(
    this->Internals->Closed, "checked",
    SIGNAL(toggled(bool)),
    widget->GetRepresentationProxy(), 
    widget->GetRepresentationProxy()->GetProperty("ClosedLoop"));

}

//-----------------------------------------------------------------------------
void pqContourWidget::cleanupWidget()
{
  this->Internals->Links.removeAllPropertyLinks();
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  
  if (widget)
    {
    widget->InvokeCommand("Initialize");
    pqApplicationCore::instance()->get3DWidgetFactory()->
      free3DWidget(widget);
//    vtkSMProxyManager* pxm=vtkSMProxyManager::GetProxyManager();
//    pxm->UnRegisterProxy(widget);
    }
  this->setWidgetProxy(0);
}

//-----------------------------------------------------------------------------
void pqContourWidget::select()
{
  this->setWidgetVisible(true);
  this->setVisible(true);
  this->setLineColor(QColor::fromRgbF(1.0,0.0,1.0));
  this->Superclass::select();
  this->Superclass::updatePickShortcut(true);
  this->getWidgetProxy()->SetEnabled(1);
}

//-----------------------------------------------------------------------------
void pqContourWidget::getBounds(double bounds[6]) const
{
  this->getWidgetProxy()->GetBounds(bounds);
}

//-----------------------------------------------------------------------------
void pqContourWidget::deselect()
{
  // this->Superclass::deselect();
  this->setVisible(0);
  this->setLineColor(QColor::fromRgbF(1.0,1.0,1.0));
  this->Superclass::updatePickShortcut(false);
  //this->getWidgetProxy()->SetEnabled(0);
}

//-----------------------------------------------------------------------------
void pqContourWidget::updateWidgetVisibility()
{
  const bool widget_visible = this->widgetVisible();
  const bool widget_enabled = this->widgetSelected();
  this->updateWidgetState(widget_visible,  widget_enabled);
}

//-----------------------------------------------------------------------------
void pqContourWidget::removeAllNodes()
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  if (widget)
    {
    widget->InvokeCommand("ClearAllNodes");
    }
  this->setModified();
  this->render();
}

//-----------------------------------------------------------------------------
void pqContourWidget::closeLoop()
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  if (widget)
    {
    widget->InvokeCommand("CloseLoop");
    }
  this->setModified();
  this->render();
}

//----------------------------------------------------------------------------
void pqContourWidget::setPointPlacer(vtkSMProxy* placerProxy)
{
  this->updateRepProperty(placerProxy, "PointPlacer");
}

//-----------------------------------------------------------------------------
void pqContourWidget::setLineInterpolator(vtkSMProxy* interpProxy)
{
  this->updateRepProperty(interpProxy, "LineInterpolator");
}

//-----------------------------------------------------------------------------
void pqContourWidget::setLineColor(const QColor& color)
{
  vtkSMProxy* widget = this->getWidgetProxy();
  vtkSMPropertyHelper(widget,
    "LineColor").Set(0, color.redF());
  vtkSMPropertyHelper(widget,
    "LineColor").Set(1,color.greenF());
  vtkSMPropertyHelper(widget,
    "LineColor").Set(2 , color.blueF());
  widget->UpdateVTKObjects(); 
}

//-----------------------------------------------------------------------------
void pqContourWidget::updateRepProperty(
  vtkSMProxy* smProxy, const char* propertyName)
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  if (widget && propertyName && *propertyName)
    {
    vtkSMProxyProperty* proxyProp = 
      vtkSMProxyProperty::SafeDownCast(
        widget->GetProperty(propertyName));
    if (proxyProp)
      {
      proxyProp->RemoveAllProxies();
      proxyProp->AddProxy(smProxy);
      widget->UpdateProperty(propertyName);
      }
    }
}
