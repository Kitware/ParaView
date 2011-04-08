/*=========================================================================

   Program:   ParaQ
   Module:    pqHandleWidget.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.2. 

   See License_v1.2.txt for the full ParaQ license.
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
#include "pqHandleWidget.h"
#include "ui_pqHandleWidget.h"

#include "pq3DWidgetFactory.h"
#include "pqApplicationCore.h"
#include "pqPropertyLinks.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"

#include <QDoubleValidator>
#include <QShortcut>

#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMNewWidgetRepresentationProxy.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxyManager.h>
#include <vtkSmartPointer.h>

/////////////////////////////////////////////////////////////////////////
// pqHandleWidget::pqImplementation

class pqHandleWidget::pqImplementation
{
public:
  pqImplementation() :
    UI(new Ui::pqHandleWidget())
  {
  }
  
  ~pqImplementation()
  {
    delete this->UI;
  }
  
  /// Stores the Qt widgets
  Ui::pqHandleWidget* const UI;
  pqPropertyLinks Links;
};

/////////////////////////////////////////////////////////////////////////
// pqHandleWidget

pqHandleWidget::pqHandleWidget(vtkSMProxy* _smproxy, vtkSMProxy* pxy, QWidget* p) :
  Superclass(_smproxy, pxy, p),
  Implementation(new pqImplementation())
{
  // enable picking.
  this->pickingSupported(QKeySequence(tr("P")));

  this->Implementation->UI->setupUi(this);
  this->Implementation->UI->show3DWidget->setChecked(this->widgetVisible());

  // Setup validators for all line edits.
  QDoubleValidator* validator = new QDoubleValidator(this);
  this->Implementation->UI->worldPositionX->setValidator(validator);
  this->Implementation->UI->worldPositionY->setValidator(validator);
  this->Implementation->UI->worldPositionZ->setValidator(validator);

  QObject::connect(this->Implementation->UI->show3DWidget,
    SIGNAL(toggled(bool)), this, SLOT(setWidgetVisible(bool)));

  QObject::connect(this, SIGNAL(widgetVisibilityChanged(bool)),
    this, SLOT(onWidgetVisibilityChanged(bool)));

  QObject::connect(this->Implementation->UI->useCenterBounds,
    SIGNAL(clicked()), this, SLOT(resetBounds()));

  QObject::connect(&this->Implementation->Links, SIGNAL(qtWidgetChanged()),
    this, SLOT(setModified()));

  // Trigger a render when use explicitly edits the positions.
  QObject::connect(this->Implementation->UI->worldPositionX, 
    SIGNAL(editingFinished()), 
    this, SLOT(render()), Qt::QueuedConnection);
  QObject::connect(this->Implementation->UI->worldPositionY, 
    SIGNAL(editingFinished()), 
    this, SLOT(render()), Qt::QueuedConnection);
  QObject::connect(this->Implementation->UI->worldPositionZ,
    SIGNAL(editingFinished()), 
    this, SLOT(render()), Qt::QueuedConnection);
  
  pqServerManagerModel* smmodel =
    pqApplicationCore::instance()->getServerManagerModel();
  this->createWidget(smmodel->findServer(_smproxy->GetSession()));
}

//-----------------------------------------------------------------------------
pqHandleWidget::~pqHandleWidget()
{
  this->cleanupWidget();
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void pqHandleWidget::pick(double dx, double dy, double dz)
{
  vtkSMProxy* widget = this->getWidgetProxy();
  vtkSMPropertyHelper(widget, "WorldPosition").Set(0, dx);
  vtkSMPropertyHelper(widget, "WorldPosition").Set(1, dy);
  vtkSMPropertyHelper(widget, "WorldPosition").Set(2, dz);
  widget->UpdateVTKObjects();
  this->setModified();
  this->render();
}

//-----------------------------------------------------------------------------
void pqHandleWidget::createWidget(pqServer* server)
{
  vtkSMNewWidgetRepresentationProxy* widget =
    pqApplicationCore::instance()->get3DWidgetFactory()->
    get3DWidget("PointSourceWidgetRepresentation", server);
  this->setWidgetProxy(widget);
  
  widget->UpdateVTKObjects();
  widget->UpdatePropertyInformation();

  this->Implementation->Links.addPropertyLink(
    this->Implementation->UI->worldPositionX, "text2",
    SIGNAL(textChanged(const QString&)),
    widget, widget->GetProperty("WorldPosition"), 0);

  this->Implementation->Links.addPropertyLink(
    this->Implementation->UI->worldPositionY, "text2",
    SIGNAL(textChanged(const QString&)),
    widget, widget->GetProperty("WorldPosition"), 1);

  this->Implementation->Links.addPropertyLink(
    this->Implementation->UI->worldPositionZ, "text2",
    SIGNAL(textChanged(const QString&)),
    widget, widget->GetProperty("WorldPosition"), 2);
}

//-----------------------------------------------------------------------------
void pqHandleWidget::cleanupWidget()
{
  this->Implementation->Links.removeAllPropertyLinks();
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  if (widget)
    {
    pqApplicationCore::instance()->get3DWidgetFactory()->
      free3DWidget(widget);
    }
  this->setWidgetProxy(0);
}

//-----------------------------------------------------------------------------
void pqHandleWidget::onWidgetVisibilityChanged(bool visible)
{
  this->Implementation->UI->show3DWidget->blockSignals(true);
  this->Implementation->UI->show3DWidget->setChecked(visible);
  this->Implementation->UI->show3DWidget->blockSignals(false);
}

//-----------------------------------------------------------------------------
void pqHandleWidget::resetBounds(double input_bounds[6])
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  double input_origin[3];
  input_origin[0] = (input_bounds[0] + input_bounds[1]) / 2.0;
  input_origin[1] = (input_bounds[2] + input_bounds[3]) / 2.0;
  input_origin[2] = (input_bounds[4] + input_bounds[5]) / 2.0;

  if(vtkSMDoubleVectorProperty* const widget_position =
    vtkSMDoubleVectorProperty::SafeDownCast(
      widget->GetProperty("WorldPosition")))
    {
    widget_position->SetElements(input_origin);
    widget->UpdateVTKObjects();
    }
}


