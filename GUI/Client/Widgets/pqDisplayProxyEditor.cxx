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

// VTK includes
#include "QVTKWidget.h"

// paraview includes
#include "vtkSMPropertyIterator.h"
#include "vtkSMProperty.h"
#include "vtkSMDisplayProxy.h"

// paraq widget includes
#include "pqSignalAdaptors.h"

// paraq client includes
#include "pqSMAdaptor.h"
#include "pqPropertyLinks.h"
#include "pqSMProxy.h"
#include "pqPipelineData.h"

class pqDisplayProxyEditorInternal : public Ui::pqDisplayProxyEditor
{
public:
  pqDisplayProxyEditorInternal()
    {
    this->Links = new pqPropertyLinks;
    }
  ~pqDisplayProxyEditorInternal()
    {
    delete this->Links;
    }

  pqPropertyLinks* Links;
};

/// constructor
pqDisplayProxyEditor::pqDisplayProxyEditor(QWidget* p)
  : QWidget(p), DisplayProxy(NULL)
{
  this->Internal = new pqDisplayProxyEditorInternal;
  this->Internal->setupUi(this);
}
/// destructor
pqDisplayProxyEditor::~pqDisplayProxyEditor()
{
  delete this->Internal;
}

/// set the proxy to display properties for
void pqDisplayProxyEditor::setDisplayProxy(pqSMProxy display, pqSMProxy sourceProxy)
{
  if(this->DisplayProxy == display)
    {
    return;
    }

  if(this->DisplayProxy)
    {

    // clean up visibility
    this->Internal->Links->removePropertyLink(this->Internal->ViewData,
                                             "checked",
                                             SIGNAL(stateChanged(int)),
                                             this->DisplayProxy,
                                             this->DisplayProxy->GetProperty("Visibility"));
    QObject::disconnect(this->Internal->ViewData, SIGNAL(stateChanged(int)),
                        this, SLOT(updateView()));
    
    // clean up representation
    QObject* signalAdaptor = this->Internal->StyleRepresentation->findChild<QObject*>("StyleRepresentationAdapator");
    if(signalAdaptor)
      {
      this->Internal->Links->removePropertyLink(signalAdaptor,
                                             "currentText",
                                             SIGNAL(currentTextChanged(const QString&)),
                                             this->DisplayProxy,
                                             this->DisplayProxy->GetProperty("Representation"));
      delete signalAdaptor;
      }
    this->Internal->StyleRepresentation->clear();
    
    // clean up interpolation
    signalAdaptor = this->Internal->StyleInterpolation->findChild<QObject*>("StyleInterpolationAdapator");
    if(signalAdaptor)
      {
      this->Internal->Links->removePropertyLink(signalAdaptor,
                                             "currentText",
                                             SIGNAL(currentTextChanged(const QString&)),
                                             this->DisplayProxy,
                                             this->DisplayProxy->GetProperty("Interpolation"));
      delete signalAdaptor;
      }
    this->Internal->StyleInterpolation->clear();
    
    // clean up point size
    this->Internal->Links->removePropertyLink(this->Internal->StylePointSize,
                                             "value",
                                             SIGNAL(valueChanged(double)),
                                             this->DisplayProxy,
                                             this->DisplayProxy->GetProperty("PointSize"));
    QObject::disconnect(this->Internal->StylePointSize, SIGNAL(valueChanged(double)),
                        this, SLOT(updateView()));
    
    // clean up line width
    this->Internal->Links->removePropertyLink(this->Internal->StyleLineWidth,
                                             "value",
                                             SIGNAL(valueChanged(double)),
                                             this->DisplayProxy,
                                             this->DisplayProxy->GetProperty("LineWidth"));
    QObject::disconnect(this->Internal->StyleLineWidth, SIGNAL(valueChanged(double)),
                        this, SLOT(updateView()));

    // clean up for translate
    this->Internal->Links->removePropertyLink(this->Internal->TranslateX,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Position"),
                                           0
                                           );
    QObject::disconnect(this->Internal->TranslateX, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    this->Internal->Links->removePropertyLink(this->Internal->TranslateY,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Position"),
                                           1
                                           );
    QObject::disconnect(this->Internal->TranslateY, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    this->Internal->Links->removePropertyLink(this->Internal->TranslateZ,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Position"),
                                           2
                                           );
    QObject::disconnect(this->Internal->TranslateZ, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    
    // clean up for scale
    this->Internal->Links->removePropertyLink(this->Internal->ScaleX,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Scale"),
                                           0
                                           );
    QObject::disconnect(this->Internal->ScaleX, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    this->Internal->Links->removePropertyLink(this->Internal->ScaleY,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Scale"),
                                           1
                                           );
    QObject::disconnect(this->Internal->ScaleY, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    this->Internal->Links->removePropertyLink(this->Internal->ScaleZ,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Scale"),
                                           2
                                           );
    QObject::disconnect(this->Internal->ScaleZ, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    
    // clean up for orientation
    this->Internal->Links->removePropertyLink(this->Internal->OrientationX,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Orientation"),
                                           0
                                           );
    QObject::disconnect(this->Internal->OrientationX, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    this->Internal->Links->removePropertyLink(this->Internal->OrientationY,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Orientation"),
                                           1
                                           );
    QObject::disconnect(this->Internal->OrientationY, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    this->Internal->Links->removePropertyLink(this->Internal->OrientationZ,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Orientation"),
                                           2
                                           );
    QObject::disconnect(this->Internal->OrientationZ, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    
    // clean up for origin
    this->Internal->Links->removePropertyLink(this->Internal->OriginX,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Origin"),
                                           0
                                           );
    QObject::disconnect(this->Internal->OriginX, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    this->Internal->Links->removePropertyLink(this->Internal->OriginY,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Origin"),
                                           1
                                           );
    QObject::disconnect(this->Internal->OriginY, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    this->Internal->Links->removePropertyLink(this->Internal->OriginZ,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Origin"),
                                           2
                                           );
    QObject::disconnect(this->Internal->OriginZ, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    
    // clean up for opacity
    this->Internal->Links->removePropertyLink(this->Internal->Opacity,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Opacity"));
    QObject::disconnect(this->Internal->Opacity, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    }
  
  if(vtkSMDisplayProxy::SafeDownCast(display))
    {
    this->DisplayProxy = display;
    this->SourceProxy = sourceProxy;
    }

  if(this->DisplayProxy)
    {
    // set up

    // setup for visibility
    this->Internal->Links->addPropertyLink(this->Internal->ViewData,
                                           "checked",
                                           SIGNAL(stateChanged(int)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Visibility"));
    QObject::connect(this->Internal->ViewData, SIGNAL(stateChanged(int)),
                     this, SLOT(updateView()));
    

    // setup for representation
    QList<QVariant> items = pqSMAdaptor::getEnumerationPropertyDomain(
                                           this->DisplayProxy->GetProperty("Representation"));
    foreach(QVariant item, items)
      {
      this->Internal->StyleRepresentation->addItem(item.toString());
      }
    QObject* signalAdaptor = new pqSignalAdaptorComboBox(this->Internal->StyleRepresentation);
    signalAdaptor->setObjectName("StyleRepresentationAdapator");
    this->Internal->Links->addPropertyLink(signalAdaptor,
                                           "currentText",
                                           SIGNAL(currentTextChanged(const QString&)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Representation"));
    QObject::connect(signalAdaptor, SIGNAL(currentTextChanged(const QString&)),
                     this, SLOT(updateView()));
    
    
    // setup for interpolation
    items = pqSMAdaptor::getEnumerationPropertyDomain(
                                           this->DisplayProxy->GetProperty("Interpolation"));
    foreach(QVariant item, items)
      {
      this->Internal->StyleInterpolation->addItem(item.toString());
      }
    signalAdaptor = new pqSignalAdaptorComboBox(this->Internal->StyleInterpolation);
    signalAdaptor->setObjectName("StyleInterpolationAdapator");
    this->Internal->Links->addPropertyLink(signalAdaptor,
                                           "currentText",
                                           SIGNAL(currentTextChanged(const QString&)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Interpolation"));
    QObject::connect(signalAdaptor, SIGNAL(currentTextChanged(const QString&)),
                     this, SLOT(updateView()));
    
    // setup for point size
    this->Internal->Links->addPropertyLink(this->Internal->StylePointSize,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("PointSize"));
    QObject::connect(this->Internal->StylePointSize, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    
    // setup for line width
    this->Internal->Links->addPropertyLink(this->Internal->StyleLineWidth,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("LineWidth"));
    QObject::connect(this->Internal->StyleLineWidth, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    
    // setup for translate
    this->Internal->Links->addPropertyLink(this->Internal->TranslateX,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Position"),
                                           0
                                           );
    QObject::connect(this->Internal->TranslateX, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    this->Internal->Links->addPropertyLink(this->Internal->TranslateY,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Position"),
                                           1
                                           );
    QObject::connect(this->Internal->TranslateY, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    this->Internal->Links->addPropertyLink(this->Internal->TranslateZ,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Position"),
                                           2
                                           );
    QObject::connect(this->Internal->TranslateZ, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    
    // setup for scale
    this->Internal->Links->addPropertyLink(this->Internal->ScaleX,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Scale"),
                                           0
                                           );
    QObject::connect(this->Internal->ScaleX, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    this->Internal->Links->addPropertyLink(this->Internal->ScaleY,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Scale"),
                                           1
                                           );
    QObject::connect(this->Internal->ScaleY, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    this->Internal->Links->addPropertyLink(this->Internal->ScaleZ,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Scale"),
                                           2
                                           );
    QObject::connect(this->Internal->ScaleZ, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    
    // setup for orientation
    this->Internal->Links->addPropertyLink(this->Internal->OrientationX,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Orientation"),
                                           0
                                           );
    QObject::connect(this->Internal->OrientationX, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    this->Internal->Links->addPropertyLink(this->Internal->OrientationY,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Orientation"),
                                           1
                                           );
    QObject::connect(this->Internal->OrientationY, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    this->Internal->Links->addPropertyLink(this->Internal->OrientationZ,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Orientation"),
                                           2
                                           );
    QObject::connect(this->Internal->OrientationZ, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    
    // setup for origin
    this->Internal->Links->addPropertyLink(this->Internal->OriginX,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Origin"),
                                           0
                                           );
    QObject::connect(this->Internal->OriginX, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    this->Internal->Links->addPropertyLink(this->Internal->OriginY,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Origin"),
                                           1
                                           );
    QObject::connect(this->Internal->OriginY, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    this->Internal->Links->addPropertyLink(this->Internal->OriginZ,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Origin"),
                                           2
                                           );
    QObject::connect(this->Internal->OriginZ, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    
    // setup for opacity
    this->Internal->Links->addPropertyLink(this->Internal->Opacity,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->DisplayProxy,
                                           this->DisplayProxy->GetProperty("Opacity"));
    QObject::connect(this->Internal->Opacity, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    }
}

/// get the proxy for which properties are displayed
pqSMProxy pqDisplayProxyEditor::displayProxy()
{
  return this->DisplayProxy;
}

void pqDisplayProxyEditor::updateView()
{
  QVTKWidget* widget = pqPipelineData::instance()->getWindowFor(this->SourceProxy);
  if(widget)
    {
    widget->update();
    }
}

