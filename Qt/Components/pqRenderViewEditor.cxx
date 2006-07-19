/*=========================================================================

   Program: ParaView
   Module:    pqRenderViewEditor.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#include "pqRenderViewEditor.h"
#include "ui_pqRenderViewEditor.h"

// Qt includes

// VTK includes
#include "QVTKWidget.h"

// ParaView Server Manager includes
#include "vtkSMRenderModuleProxy.h"

// ParaView widget includes
#include "pqSignalAdaptors.h"

// ParaView client includes
#include "pqSMAdaptor.h"
#include "pqPropertyLinks.h"

class pqRenderViewEditorInternal : public Ui::pqRenderViewEditor
{
public:
  pqRenderViewEditorInternal()
    {
    this->Links = new pqPropertyLinks;
    }
  ~pqRenderViewEditorInternal()
    {
    delete this->Links;
    }
  pqPropertyLinks* Links;
};

/// constructor
pqRenderViewEditor::pqRenderViewEditor(QWidget* p)
  : QWidget(p), RenderView(NULL)
{
  this->Internal = new pqRenderViewEditorInternal;
  this->Internal->setupUi(this);
}
/// destructor
pqRenderViewEditor::~pqRenderViewEditor()
{
  delete this->Internal;
}

/// set the proxy to display properties for
void pqRenderViewEditor::setRenderView(vtkSMRenderModuleProxy* view)
{
  if(view == &*this->RenderView)
    {
    return;
    }

  if(this->RenderView)
    {
    // clean up
    QObject* signalAdaptor = this->Internal->BackgroundColor->findChild<QObject*>("BackgroundColorAdaptor");
    if(signalAdaptor)
      {
      this->Internal->Links->removePropertyLink(signalAdaptor,
                                             "color",
                                             SIGNAL(colorChanged(const QVariant&)),
                                             this->RenderView,
                                             this->RenderView->GetProperty("Background"));
      delete signalAdaptor;
      }
    
    // clean up for parallel projection
    this->Internal->Links->removePropertyLink(this->Internal->ParallelProjection,
                                           "checked",
                                           SIGNAL(stateChanged(int)),
                                           this->RenderView,
                                           this->RenderView->GetProperty("CameraParallelProjection"));
    QObject::disconnect(this->Internal->ParallelProjection, SIGNAL(stateChanged(int)),
                     this, SLOT(updateView()));
    
    // clean up for triangle strips
    this->Internal->Links->removePropertyLink(this->Internal->TriangleStrips,
                                           "checked",
                                           SIGNAL(stateChanged(int)),
                                           this->RenderView,
                                           this->RenderView->GetProperty("UseTriangleStrips"));
    QObject::disconnect(this->Internal->TriangleStrips, SIGNAL(stateChanged(int)),
                     this, SLOT(updateView()));
    
    // clean up for immediate mode
    this->Internal->Links->removePropertyLink(this->Internal->ImmediateModeRendering,
                                           "checked",
                                           SIGNAL(stateChanged(int)),
                                           this->RenderView,
                                           this->RenderView->GetProperty("UseImmediateMode"));
    QObject::disconnect(this->Internal->ImmediateModeRendering, SIGNAL(stateChanged(int)),
                     this, SLOT(updateView()));
    
    
    // clean up for light color
    signalAdaptor = this->Internal->SetLightColor->findChild<QObject*>("LightColorAdaptor");
    if(signalAdaptor)
      {
      this->Internal->Links->removePropertyLink(signalAdaptor,
                                             "color",
                                             SIGNAL(colorChanged(const QVariant&)),
                                             this->RenderView,
                                             this->RenderView->GetProperty("LightAmbientColor"));
      this->Internal->Links->removePropertyLink(signalAdaptor,
                                             "color",
                                             SIGNAL(colorChanged(const QVariant&)),
                                             this->RenderView,
                                             this->RenderView->GetProperty("LightDiffuseColor"));
      this->Internal->Links->removePropertyLink(signalAdaptor,
                                             "color",
                                             SIGNAL(colorChanged(const QVariant&)),
                                             this->RenderView,
                                             this->RenderView->GetProperty("LightSpecularColor"));
      delete signalAdaptor;
      }

    // clean up for light intensity
    signalAdaptor = this->Internal->LightIntensity->findChild<QObject*>("LightIntensityAdaptor");
    if(signalAdaptor)
      {
      this->Internal->Links->removePropertyLink(signalAdaptor,
                                             "value",
                                             SIGNAL(valueChanged(double)),
                                             this->RenderView,
                                             this->RenderView->GetProperty("LightIntensity"));
      delete signalAdaptor;
      }
    this->Internal->Links->removePropertyLink(this->Internal->LightIntensityEdit,
                                           "text",
                                           SIGNAL(textChanged(const QString&)),
                                           this->RenderView,
                                           this->RenderView->GetProperty("LightIntensity"));
    
    }
  
  this->RenderView = view;

  if(this->RenderView)
    {
    // setup

    // setup for background color
    QObject* signalAdaptor = new pqSignalAdaptorColor(this->Internal->BackgroundColor, 
                                                      "chosenColor",
                                                      SIGNAL(chosenColorChanged(const QColor&)));
    signalAdaptor->setObjectName("BackgroundColorAdaptor");
    this->Internal->Links->addPropertyLink(signalAdaptor,
                                           "color",
                                           SIGNAL(colorChanged(const QVariant&)),
                                           this->RenderView,
                                           this->RenderView->GetProperty("Background"));
    QObject::connect(signalAdaptor, SIGNAL(colorChanged(const QVariant&)),
                     this, SLOT(updateView()));

    // setup for parallel projection
    this->Internal->Links->addPropertyLink(this->Internal->ParallelProjection,
                                           "checked",
                                           SIGNAL(stateChanged(int)),
                                           this->RenderView,
                                           this->RenderView->GetProperty("CameraParallelProjection"));
    QObject::connect(this->Internal->ParallelProjection, SIGNAL(stateChanged(int)),
                     this, SLOT(updateView()));
    
    // setup for triangle strips
    this->Internal->Links->addPropertyLink(this->Internal->TriangleStrips,
                                           "checked",
                                           SIGNAL(stateChanged(int)),
                                           this->RenderView,
                                           this->RenderView->GetProperty("UseTriangleStrips"));
    QObject::connect(this->Internal->TriangleStrips, SIGNAL(stateChanged(int)),
                     this, SLOT(updateView()));
    
    // setup for immediate mode
    this->Internal->Links->addPropertyLink(this->Internal->ImmediateModeRendering,
                                           "checked",
                                           SIGNAL(stateChanged(int)),
                                           this->RenderView,
                                           this->RenderView->GetProperty("UseImmediateMode"));
    QObject::connect(this->Internal->ImmediateModeRendering, SIGNAL(stateChanged(int)),
                     this, SLOT(updateView()));
    
    
    
    // setup for light color
    signalAdaptor = new pqSignalAdaptorColor(this->Internal->SetLightColor, 
                                                      "chosenColor",
                                                      SIGNAL(chosenColorChanged(const QColor&)));
    signalAdaptor->setObjectName("LightColorAdaptor");
    this->Internal->Links->addPropertyLink(signalAdaptor,
                                           "color",
                                           SIGNAL(colorChanged(const QVariant&)),
                                           this->RenderView,
                                           this->RenderView->GetProperty("LightAmbientColor"));
    this->Internal->Links->addPropertyLink(signalAdaptor,
                                           "color",
                                           SIGNAL(colorChanged(const QVariant&)),
                                           this->RenderView,
                                           this->RenderView->GetProperty("LightSpecularColor"));
    this->Internal->Links->addPropertyLink(signalAdaptor,
                                           "color",
                                           SIGNAL(colorChanged(const QVariant&)),
                                           this->RenderView,
                                           this->RenderView->GetProperty("LightDiffuseColor"));
    QObject::connect(signalAdaptor, SIGNAL(colorChanged(const QVariant&)),
                     this, SLOT(updateView()));
    
    // setup for light intensity
    signalAdaptor = new pqSignalAdaptorSliderRange(this->Internal->LightIntensity);
    signalAdaptor->setObjectName("LightIntensityAdaptor");
    this->Internal->Links->addPropertyLink(signalAdaptor,
                                           "value",
                                           SIGNAL(valueChanged(double)),
                                           this->RenderView,
                                           this->RenderView->GetProperty("LightIntensity"));
    this->Internal->Links->addPropertyLink(this->Internal->LightIntensityEdit,
                                           "text",
                                           SIGNAL(textChanged(const QString&)),
                                           this->RenderView,
                                           this->RenderView->GetProperty("LightIntensity"));
    QObject::connect(signalAdaptor, SIGNAL(valueChanged(double)),
                     this, SLOT(updateView()));
    }
}

vtkSMRenderModuleProxy* pqRenderViewEditor::renderView() const
{
  return static_cast<vtkSMRenderModuleProxy*>(&*this->RenderView);
}

void pqRenderViewEditor::updateView()
{
  this->renderView()->StillRender();
}

