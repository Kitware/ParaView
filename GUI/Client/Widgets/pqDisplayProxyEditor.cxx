/*=========================================================================

   Program:   ParaQ
   Module:    pqDisplayProxyEditor.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

// this include
#include "pqDisplayProxyEditor.h"
#include "ui_pqDisplayProxyEditor.h"

// Qt includes
#include <QPointer>
#include <QtDebug>

// VTK includes
#include "QVTKWidget.h"

// paraview includes
#include "vtkPVDataInformation.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMSourceProxy.h"

// paraq widget includes
#include "pqSignalAdaptors.h"

// paraq client includes
#include "pqApplicationCore.h"
#include "pqSMAdaptor.h"
#include "pqPropertyLinks.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineSource.h"
#include "pqParts.h"
#include "pqRenderModule.h"

class pqDisplayProxyEditorInternal : public Ui::pqDisplayProxyEditor
{
public:
  pqDisplayProxyEditorInternal()
    {
    this->Links = new pqPropertyLinks;
    this->ColorAdaptor = 0;
    this->RepresentationAdaptor = 0;
    this->InterpolationAdaptor = 0;
    }

  ~pqDisplayProxyEditorInternal()
    {
    delete this->Links;
    delete this->ColorAdaptor;
    delete this->RepresentationAdaptor;
    delete this->InterpolationAdaptor;
    }

  pqPropertyLinks* Links;

  // The display whose properties are being edited.
  QPointer<pqPipelineDisplay> Display;
  pqSignalAdaptorColor* ColorAdaptor;
  pqSignalAdaptorComboBox* RepresentationAdaptor;
  pqSignalAdaptorComboBox* InterpolationAdaptor;
  
};

//-----------------------------------------------------------------------------
/// constructor
pqDisplayProxyEditor::pqDisplayProxyEditor(QWidget* p)
  : QWidget(p)
{
  this->Internal = new pqDisplayProxyEditorInternal;
  this->Internal->setupUi(this);
  this->setupGUIConnections();
}

//-----------------------------------------------------------------------------
/// destructor
pqDisplayProxyEditor::~pqDisplayProxyEditor()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
/// set the proxy to display properties for
void pqDisplayProxyEditor::setDisplay(pqPipelineDisplay* display) 
{
  if(this->Internal->Display == display)
    {
    return;
    }

  vtkSMDataObjectDisplayProxy* displayProxy = 
    (display)? display->getProxy() : NULL;
  if(this->Internal->Display)
    {
    // break all old links.
    this->Internal->Links->removeAllPropertyLinks();
    }

  this->Internal->Display = 0;
  if (!display )
    {
    return;
    }
  
  // setup for visibility
  this->Internal->Links->addPropertyLink(this->Internal->ViewData,
    "checked", SIGNAL(stateChanged(int)),
    displayProxy, displayProxy->GetProperty("Visibility"));

  // set color by array
  this->Internal->ColorBy->clear();
  this->Internal->ColorBy->addItems(pqPart::GetColorFields(displayProxy));
  QString currentArray = pqPart::GetColorField(displayProxy);
  this->Internal->ColorBy->setCurrentIndex(
    this->Internal->ColorBy->findText(currentArray));
  if(currentArray == "Property")
    {
    this->Internal->ColorActorColor->setEnabled(true);
    }
  else
    {
    this->Internal->ColorActorColor->setEnabled(false);
    }

  // set up actor color
  this->Internal->Links->addPropertyLink(
    this->Internal->ColorAdaptor, "color",
    SIGNAL(colorChanged(const QVariant&)),
    displayProxy, displayProxy->GetProperty("Color"));

  // setup for representation
  this->Internal->StyleRepresentation->clear();
  QList<QVariant> items = pqSMAdaptor::getEnumerationPropertyDomain(
    displayProxy->GetProperty("Representation"));
  foreach(QVariant item, items)
    {
    if (item == "Volume" && !displayProxy->GetHasVolumePipeline())
      {
      continue; // add volume only if volume representation is supported.
      }
    this->Internal->StyleRepresentation->addItem(item.toString());
    }
  this->Internal->Links->addPropertyLink(
    this->Internal->RepresentationAdaptor, "currentText",
    SIGNAL(currentTextChanged(const QString&)),
    displayProxy, displayProxy->GetProperty("Representation"));

  // setup for interpolation
  this->Internal->StyleInterpolation->clear();
  items = pqSMAdaptor::getEnumerationPropertyDomain(
    displayProxy->GetProperty("Interpolation"));
  foreach(QVariant item, items)
    {
    this->Internal->StyleInterpolation->addItem(item.toString());
    }
  this->Internal->Links->addPropertyLink(this->Internal->InterpolationAdaptor,
    "currentText", SIGNAL(currentTextChanged(const QString&)),
    displayProxy, displayProxy->GetProperty("Interpolation"));

  // setup for point size
  this->Internal->Links->addPropertyLink(this->Internal->StylePointSize,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("PointSize"));

  // setup for line width
  this->Internal->Links->addPropertyLink(this->Internal->StyleLineWidth,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("LineWidth"));

  // setup for translate
  this->Internal->Links->addPropertyLink(this->Internal->TranslateX,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Position"), 0);

  this->Internal->Links->addPropertyLink(this->Internal->TranslateY,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Position"), 1);
  
  this->Internal->Links->addPropertyLink(this->Internal->TranslateZ,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Position"), 2);

  // setup for scale
  this->Internal->Links->addPropertyLink(this->Internal->ScaleX,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Scale"), 0);

  this->Internal->Links->addPropertyLink(this->Internal->ScaleY,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Scale"), 1);

  this->Internal->Links->addPropertyLink(this->Internal->ScaleZ,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Scale"), 2);

  // setup for orientation
  this->Internal->Links->addPropertyLink(this->Internal->OrientationX,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Orientation"), 0);

  this->Internal->Links->addPropertyLink(this->Internal->OrientationY,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Orientation"), 1);

  this->Internal->Links->addPropertyLink(this->Internal->OrientationZ,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Orientation"), 2);

  // setup for origin
  this->Internal->Links->addPropertyLink(this->Internal->OriginX,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Origin"), 0);

  this->Internal->Links->addPropertyLink(this->Internal->OriginY,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Origin"), 1);

  this->Internal->Links->addPropertyLink(this->Internal->OriginZ,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Origin"), 2);

  // setup for opacity
  this->Internal->Links->addPropertyLink(this->Internal->Opacity,
    "value", SIGNAL(valueChanged(double)),
    displayProxy, displayProxy->GetProperty("Opacity"));

  this->Internal->Display = display;
}

//-----------------------------------------------------------------------------
/// get the proxy for which properties are displayed
pqPipelineDisplay* pqDisplayProxyEditor::getDisplay()
{
  return this->Internal->Display;
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::updateView()
{
  if (this->Internal->Display)
    {
    pqApplicationCore::instance()->getActiveRenderModule()->render();
    }
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::setupGUIConnections()
{
  // We are usinging Queues slot execution where ever possible,
  // This ensures that the updateView() slot is called 
  // only after the vtkSMProperty has been changed by the pqPropertyLinks.
  QObject::connect(this->Internal->ViewData, SIGNAL(stateChanged(int)),
    this, SLOT(updateView()),Qt::QueuedConnection);
  QObject::connect(this->Internal->ColorBy, 
    SIGNAL(currentIndexChanged(const QString&)),
    this, SLOT(colorByChanged(const QString&)));
  QObject::connect(this->Internal->StylePointSize, 
    SIGNAL(valueChanged(double)), this, SLOT(updateView()));
  QObject::connect(this->Internal->StyleLineWidth, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),Qt::QueuedConnection);
  QObject::connect(this->Internal->TranslateX, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),Qt::QueuedConnection);
  QObject::connect(this->Internal->TranslateY, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),Qt::QueuedConnection);
  QObject::connect(this->Internal->TranslateZ, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),Qt::QueuedConnection);
  QObject::connect(this->Internal->ScaleX, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),Qt::QueuedConnection);
  QObject::connect(this->Internal->ScaleY, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),Qt::QueuedConnection);
  QObject::connect(this->Internal->ScaleZ, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),Qt::QueuedConnection);
  QObject::connect(this->Internal->OrientationX, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),Qt::QueuedConnection);
  QObject::connect(this->Internal->OrientationY, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),Qt::QueuedConnection);
  QObject::connect(this->Internal->OrientationZ, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),Qt::QueuedConnection);
  QObject::connect(this->Internal->OriginX, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),Qt::QueuedConnection);
  QObject::connect(this->Internal->OriginY, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),Qt::QueuedConnection);
  QObject::connect(this->Internal->OriginZ, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),Qt::QueuedConnection);
  QObject::connect(this->Internal->Opacity, SIGNAL(valueChanged(double)),
    this, SLOT(updateView()),Qt::QueuedConnection);
  QObject::connect(this->Internal->ViewZoomToData, SIGNAL(pressed()), 
    this, SLOT(zoomToData()));
  QObject::connect(this->Internal->DismissButton, SIGNAL(pressed()),
    this, SIGNAL(dismiss()));
  
  // Create an connect signal adaptors.
  this->Internal->ColorAdaptor = new pqSignalAdaptorColor(
    this->Internal->ColorActorColor, "chosenColor",
    SIGNAL(chosenColorChanged(const QColor&)));
  this->Internal->ColorAdaptor->setObjectName("ActorColorAdaptor");
  QObject::connect(this->Internal->ColorAdaptor, 
    SIGNAL(colorChanged(const QVariant&)), this, SLOT(updateView()));

  this->Internal->RepresentationAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->StyleRepresentation);
  this->Internal->RepresentationAdaptor->setObjectName(
    "StyleRepresentationAdapator");
  QObject::connect(this->Internal->RepresentationAdaptor,
    SIGNAL(currentTextChanged(const QString&)), this, SLOT(updateView()),
    Qt::QueuedConnection);
    
    
  this->Internal->InterpolationAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->StyleInterpolation);
  this->Internal->InterpolationAdaptor->setObjectName(
    "StyleInterpolationAdapator");
  QObject::connect(this->Internal->InterpolationAdaptor, 
    SIGNAL(currentTextChanged(const QString&)), this, SLOT(updateView()),
    Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::colorByChanged(const QString& val)
{
  if (!this->Internal->Display)
    {
    return;
    }
  if(val == "Property")
    {
    this->Internal->ColorActorColor->setEnabled(true);
    pqPart::Color(this->Internal->Display->getProxy(), NULL, 0);
    }
  else
    {
    this->Internal->ColorActorColor->setEnabled(false);
    pqPart::SetColorField(this->Internal->Display->getProxy(), val);
    }
  this->updateView();
}

//-----------------------------------------------------------------------------
void pqDisplayProxyEditor::zoomToData()
{
  if (!this->Internal->Display)
    {
    return;
    }

  pqPipelineSource* input = this->Internal->Display->getInput();
  vtkSMSourceProxy* input_proxy = (input)?
    vtkSMSourceProxy::SafeDownCast(input->getProxy()) : NULL;

  if(!input_proxy)
    {
    qDebug() << "Cannot zoom to data, failed to locate input proxy.";
    return;
    }
  double bounds[6];
  input_proxy->GetDataInformation()->GetBounds(bounds);
  if (bounds[0]<=bounds[1] && bounds[2]<=bounds[3] && bounds[4]<=bounds[5])
    {
    pqRenderModule* renModule = 
      pqApplicationCore::instance()->getActiveRenderModule();
    vtkSMRenderModuleProxy* rm = renModule->getProxy();
    rm->ResetCamera(bounds);
    rm->ResetCameraClippingRange();
    renModule->render();
    }
}

