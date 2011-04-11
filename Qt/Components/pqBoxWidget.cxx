/*=========================================================================

   Program: ParaView
   Module:    pqBoxWidget.cxx

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
#include "pqBoxWidget.h"
#include "ui_pqBoxWidget.h"

// Server Manager Includes.
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"

// Qt Includes.
#include <QDoubleValidator>

// ParaView Includes.
#include "pq3DWidgetFactory.h"
#include "pqApplicationCore.h"
#include "pqPropertyLinks.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"

class pqBoxWidget::pqImplementation : public Ui::pqBoxWidget
{
public:
  pqPropertyLinks Links;
};


#define PVBOXWIDGET_TRIGGER_RENDER(ui)  \
  QObject::connect(this->Implementation->ui,\
    SIGNAL(editingFinished()),\
    this, SLOT(render()), Qt::QueuedConnection);
//-----------------------------------------------------------------------------
pqBoxWidget::pqBoxWidget(vtkSMProxy* refProxy, vtkSMProxy* pxy, QWidget* _parent) :
  Superclass(refProxy, pxy, _parent)
{
  this->Implementation = new pqImplementation();
  this->Implementation->setupUi(this);
  this->Implementation->show3DWidget->setChecked(this->widgetVisible());  

  // Setup validators for all line edits.
  QDoubleValidator* validator = new QDoubleValidator(this);
  this->Implementation->positionX->setValidator(validator);
  this->Implementation->positionY->setValidator(validator);
  this->Implementation->positionZ->setValidator(validator);
  this->Implementation->scaleX->setValidator(validator);
  this->Implementation->scaleY->setValidator(validator);
  this->Implementation->scaleZ->setValidator(validator);
  this->Implementation->rotationX->setValidator(validator);
  this->Implementation->rotationY->setValidator(validator);
  this->Implementation->rotationZ->setValidator(validator);

  PVBOXWIDGET_TRIGGER_RENDER(positionX);
  PVBOXWIDGET_TRIGGER_RENDER(positionY);
  PVBOXWIDGET_TRIGGER_RENDER(positionZ);
  PVBOXWIDGET_TRIGGER_RENDER(scaleX);
  PVBOXWIDGET_TRIGGER_RENDER(scaleY);
  PVBOXWIDGET_TRIGGER_RENDER(scaleZ);
  PVBOXWIDGET_TRIGGER_RENDER(rotationX);
  PVBOXWIDGET_TRIGGER_RENDER(rotationY);
  PVBOXWIDGET_TRIGGER_RENDER(rotationZ);

  QObject::connect(this->Implementation->show3DWidget,
    SIGNAL(toggled(bool)), this, SLOT(setWidgetVisible(bool)));

  QObject::connect(this, SIGNAL(widgetVisibilityChanged(bool)),
    this, SLOT(onWidgetVisibilityChanged(bool)));

  QObject::connect(this->Implementation->resetBounds,
    SIGNAL(clicked()), this, SLOT(resetBounds()));

  //QObject::connect(this, SIGNAL(widgetStartInteraction()),
  //  this, SLOT(showHandles()));

  QObject::connect(&this->Implementation->Links, SIGNAL(qtWidgetChanged()),
    this, SLOT(setModified()));

  pqServerManagerModel* smmodel =
    pqApplicationCore::instance()->getServerManagerModel();
  this->createWidget(smmodel->findServer(refProxy->GetSession()));
}

//-----------------------------------------------------------------------------
pqBoxWidget::~pqBoxWidget()
{
  delete this->Implementation;
}

#define PVBOXWIDGET_LINK(ui, smproperty, index)\
{\
  this->Implementation->Links.addPropertyLink(\
    this->Implementation->ui, "text2",\
    SIGNAL(textChanged(const QString&)),\
    widget, widget->GetProperty(smproperty), index);\
}

//-----------------------------------------------------------------------------
void pqBoxWidget::createWidget(pqServer* server)
{
  vtkSMNewWidgetRepresentationProxy* widget =
    pqApplicationCore::instance()->get3DWidgetFactory()->
    get3DWidget("BoxWidgetRepresentation", server);
  this->setWidgetProxy(widget);

  widget->UpdateVTKObjects();
  widget->UpdatePropertyInformation();

  PVBOXWIDGET_LINK(positionX, "Position", 0);
  PVBOXWIDGET_LINK(positionY, "Position", 1);
  PVBOXWIDGET_LINK(positionZ, "Position", 2);

  PVBOXWIDGET_LINK(rotationX, "Rotation", 0);
  PVBOXWIDGET_LINK(rotationY, "Rotation", 1);
  PVBOXWIDGET_LINK(rotationZ, "Rotation", 2);

  PVBOXWIDGET_LINK(scaleX, "Scale", 0);
  PVBOXWIDGET_LINK(scaleY, "Scale", 1);
  PVBOXWIDGET_LINK(scaleZ, "Scale", 2);
}

//-----------------------------------------------------------------------------
// update widget bounds.
void pqBoxWidget::select()
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  double input_bounds[6];
  if (widget  && this->getReferenceInputBounds(input_bounds))
    {
    vtkSMPropertyHelper(widget, "PlaceWidget").Set(input_bounds, 6);
    widget->UpdateVTKObjects();
    }

  this->Superclass::select();
}

//-----------------------------------------------------------------------------
void pqBoxWidget::resetBounds(double input_bounds[6])
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  vtkSMPropertyHelper(widget, "PlaceWidget").Set(input_bounds, 6);
  widget->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqBoxWidget::cleanupWidget()
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  if(widget)
    {
    pqApplicationCore::instance()->get3DWidgetFactory()->
      free3DWidget(widget);
    }
  this->setWidgetProxy(0);
}

//-----------------------------------------------------------------------------
void pqBoxWidget::onWidgetVisibilityChanged(bool visible)
{
  this->Implementation->show3DWidget->blockSignals(true);
  this->Implementation->show3DWidget->setChecked(visible);
  this->Implementation->show3DWidget->blockSignals(false);
}

//-----------------------------------------------------------------------------
void pqBoxWidget::accept()
{
  this->Superclass::accept();
  this->hideHandles();
}

//-----------------------------------------------------------------------------
void pqBoxWidget::reset()
{
  this->Superclass::reset();
  this->hideHandles();
}

//-----------------------------------------------------------------------------
void pqBoxWidget::showHandles()
{
  /*
  vtkSMProxy* proxy = this->getWidgetProxy();
  if (proxy)
    {
    pqSMAdaptor::setElementProperty(proxy->GetProperty("HandleVisibility"), 1);
    proxy->UpdateVTKObjects();
    }
    */
}

//-----------------------------------------------------------------------------
void pqBoxWidget::hideHandles()
{
  /*
  vtkSMProxy* proxy = this->getWidgetProxy();
  if (proxy)
    {
    pqSMAdaptor::setElementProperty(proxy->GetProperty("HandleVisibility"), 1);
    proxy->UpdateVTKObjects();
    }
  */
}
