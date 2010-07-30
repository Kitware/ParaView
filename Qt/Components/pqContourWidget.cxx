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

#include "vtkEventQtSlotConnect.h"
#include "vtkSmartPointer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"

#include "vtkClientServerStream.h"

class pqContourWidget::pqInternals : public Ui::ContourWidget
{
public:
  vtkSmartPointer<vtkEventQtSlotConnect> ClosedLoopConnect;
};

//-----------------------------------------------------------------------------
pqContourWidget::pqContourWidget(
  vtkSMProxy* _smproxy, vtkSMProxy* pxy, QWidget* p) :
  Superclass(_smproxy, pxy, p)
{
  this->Internals = new pqInternals();
  this->Internals->ClosedLoopConnect =
    vtkSmartPointer<vtkEventQtSlotConnect>::New();

  this->Internals->setupUi(this);

  this->Internals->Visibility->setChecked(this->widgetVisible());
  QObject::connect(this, SIGNAL(widgetVisibilityChanged(bool)),
    this->Internals->Visibility, SLOT(setChecked(bool)));

  QObject::connect(this->Internals->Visibility,
    SIGNAL(toggled(bool)), this, SLOT(setWidgetVisible(bool)));

  QObject::connect(this->Internals->Closed,
    SIGNAL(toggled(bool)), this, SLOT(closeLoop(bool)));

  QObject::connect(this->Internals->Delete, SIGNAL(clicked()),
    this, SLOT(removeAllNodes()));

  QObject::connect(this->Internals->EditMode, SIGNAL(toggled(bool)),
    this, SLOT(updateMode()));
  QObject::connect(this->Internals->ModifyMode, SIGNAL(toggled(bool)),
    this, SLOT(updateMode()));
  QObject::connect(this->Internals->Finished, SIGNAL(clicked()),
    this, SLOT(finishContour()));


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
  vtkSMNewWidgetRepresentationProxy* widget = NULL;
  widget = pqApplicationCore::instance()->get3DWidgetFactory()->
    get3DWidget("ContourWidgetRepresentation2", server);
  if ( !widget )
    {
    widget = pqApplicationCore::instance()->get3DWidgetFactory()->
    get3DWidget("ContourWidgetRepresentation", server);
    }

  this->setWidgetProxy(widget);

  widget->UpdateVTKObjects();
  widget->UpdatePropertyInformation();

  this->Internals->ClosedLoopConnect->Connect(
    widget, vtkCommand::EndInteractionEvent,
    this, SLOT(checkContourLoopClosed()));
}

//-----------------------------------------------------------------------------
void pqContourWidget::cleanupWidget()
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();

  if (widget)
    {
    widget->InvokeCommand("Initialize");
    pqApplicationCore::instance()->get3DWidgetFactory()->
      free3DWidget(widget);
    }
  this->setWidgetProxy(0);
}

//-----------------------------------------------------------------------------
void pqContourWidget::select()
{
  this->setWidgetVisible(true);
  this->setLineColor(QColor::fromRgbF(1.0,0.0,1.0));
  this->Superclass::select();
  this->Superclass::updatePickShortcut(true);
  this->getWidgetProxy()->SetEnabled(1);
}

//-----------------------------------------------------------------------------
bool pqContourWidget::getBounds(double bounds[6]) const
{
  return this->getWidgetProxy()->GetBounds(bounds);
}

//-----------------------------------------------------------------------------
void pqContourWidget::deselect()
{
  this->setLineColor(QColor::fromRgbF(1.0,1.0,1.0));
  this->Superclass::updatePickShortcut(false);
  this->setEnabled(0);
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
    widget->InvokeCommand("Initialize");
    }
  this->setModified();
  this->render();
}

//-----------------------------------------------------------------------------
void pqContourWidget::checkContourLoopClosed()
{
  if(!this->Internals->Closed->isChecked())
    {
    vtkSMProxy* repProxy = this->getWidgetProxy()->GetRepresentationProxy();
    repProxy->UpdatePropertyInformation();
    int loopClosed = pqSMAdaptor::getElementProperty(
      repProxy->GetProperty("ClosedLoopInfo")).toInt();
    if(loopClosed)
      {
      this->Internals->Closed->blockSignals(true);
      this->Internals->Closed->setChecked(1);
      this->Internals->Closed->blockSignals(false);
      this->Internals->ModifyMode->setChecked(1);
      emit this->contourLoopClosed();
      }
    }

}

//-----------------------------------------------------------------------------
void pqContourWidget::closeLoop(bool val)
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  if (widget)
    {
    vtkSMProxy* repProxy = widget->GetRepresentationProxy();
    repProxy->UpdatePropertyInformation();
    bool loopClosed = pqSMAdaptor::getElementProperty(
      repProxy->GetProperty("ClosedLoopInfo")).toBool();
    if(loopClosed != val)
      {
      if(val)
        {
        widget->InvokeCommand("CloseLoop");
        }
      this->Internals->ModifyMode->setChecked(val);
      pqSMAdaptor::setElementProperty(
        widget->GetRepresentationProxy()->GetProperty("ClosedLoop"), val);
      widget->GetRepresentationProxy()->UpdateVTKObjects();
      this->setModified();
      this->render();
      }
    }
}
//-----------------------------------------------------------------------------
void pqContourWidget::updateMode()
{
  //the text should always be updated to this.
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  if (widget)
    {
    if (this->Internals->EditMode->isChecked() )
      {
       pqSMAdaptor::setElementProperty(
        widget->GetProperty("WidgetState"), 1);
      }
    else if (this->Internals->ModifyMode->isChecked() )
      {
      pqSMAdaptor::setElementProperty(
        widget->GetProperty("WidgetState"), 2);
      }
    widget->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void pqContourWidget::finishContour( )
{
  emit this->contourDone();
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
void pqContourWidget::reset()
{
  this->Superclass::reset();

  //update our mode
  this->Internals->EditMode->setChecked(true);
  this->Internals->Closed->blockSignals(true);
  this->Internals->Closed->setChecked(false);
  this->Internals->Closed->blockSignals(false);
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
