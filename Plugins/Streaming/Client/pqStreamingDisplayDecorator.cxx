/*=========================================================================

   Program: ParaView
   Module:    pqStreamingDisplayDecorator.cxx

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
#include "pqStreamingDisplayDecorator.h"
#include "ui_pqStreamingDisplayDecorator.h"
// Server Manager Includes.
#include <vtkSMProperty.h>
#include <vtkSMProxy.h>

// Qt Includes.
#include <QVBoxLayout>

// ParaView Includes.
#include "pqDisplayPanel.h"
#include "pqPropertyLinks.h"

class pqStreamingDisplayDecorator::pqImplementation
{
public:
  pqPropertyLinks Links;
};

//-----------------------------------------------------------------------------
pqStreamingDisplayDecorator::pqStreamingDisplayDecorator(pqDisplayPanel* panel)
  : Superclass(panel),
  Implementation(new pqImplementation())
{
  QWidget* frame = new QWidget(panel);
  Ui::pqStreamingDisplayDecorator ui;
  ui.setupUi(frame);

  //find the annotations section of the page, and add the checkbox there
  QWidget* annotations = panel->findChild<QWidget*>("AnnotationGroup");
  QGridLayout *l = qobject_cast<QGridLayout*>(annotations->layout());
  l->addWidget(frame);

  //link the Qt event to the Property that it controls
  pqRepresentation *rep = panel->getRepresentation();
  vtkSMProxy *displayProxy = rep->getProxy();
  vtkSMProperty *prop = displayProxy->GetProperty("PieceBoundsVisibility");
  QCheckBox *cb = frame->findChild<QCheckBox*>("PieceBoundsVisibility");  
  this->Implementation->Links.addPropertyLink(
    cb, "checked",
    SIGNAL(stateChanged(int)),
    displayProxy, prop);
}

//-----------------------------------------------------------------------------
pqStreamingDisplayDecorator::~pqStreamingDisplayDecorator()
{
//  this->Implementation->Links.removeAllPropertyLinks();
  delete this->Implementation;
}


