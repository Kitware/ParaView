/*=========================================================================

   Program: ParaView
   Module:    MantaDisplay.cxx

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
#include "MantaDisplay.h"
#include "ui_MantaDisplay.h"

// Qt Includes.
#include <QVBoxLayout>

// ParaView Includes.
#include "pqDisplayPanel.h"
#include "pqPropertyLinks.h"
#include "vtkSMPVRepresentationProxy.h"
#include "pqSignalAdaptors.h"
#include "pqActiveObjects.h"
#include "MantaView.h"

#include <iostream>

class MantaDisplay::pqInternal
{
public:
  Ui::MantaDisplay ui;
  pqPropertyLinks links;
  pqSignalAdaptorComboBox *strAdapt;
};

//-----------------------------------------------------------------------------
MantaDisplay::MantaDisplay(pqDisplayPanel* panel)
  : Superclass(panel)
{
  QWidget* frame = new QWidget(panel);
  this->Internal = new pqInternal;
  this->Internal->ui.setupUi(frame);
  QVBoxLayout* l = qobject_cast<QVBoxLayout*>(panel->layout());
  l->addWidget(frame);

  this->Internal->strAdapt =
    new pqSignalAdaptorComboBox(this->Internal->ui.material);

  MantaView* mView = qobject_cast<MantaView*>
    (pqActiveObjects::instance().activeView());
  if (!mView)
    {
    frame->setEnabled(false);
    return;
    }

  pqRepresentation *rep = panel->getRepresentation();
  vtkSMPVRepresentationProxy *mrep = vtkSMPVRepresentationProxy::SafeDownCast
    (rep->getProxy());
  if (!mrep)
    {
    frame->setEnabled(false);
    return;
    }

  vtkSMProperty *prop = mrep->GetProperty("MaterialType");
  //have to use a helper class because pqPropertyLinks won't map directly
  this->Internal->links.addPropertyLink(
    this->Internal->strAdapt,
    "currentText",
    SIGNAL(currentTextChanged(const QString&)),
    mrep,
    prop);

  prop = mrep->GetProperty("Reflectance");
  this->Internal->links.addPropertyLink(
    this->Internal->ui.reflectance,
    "value",
    SIGNAL(valueChanged(double)),
    mrep,
    prop);

  prop = mrep->GetProperty("Thickness");
  this->Internal->links.addPropertyLink(
    this->Internal->ui.thickness,
    "value",
    SIGNAL(valueChanged(double)),
    mrep,
    prop);

  prop = mrep->GetProperty("Eta");
  this->Internal->links.addPropertyLink(
    this->Internal->ui.eta,
    "value",
    SIGNAL(valueChanged(double)),
    mrep,
    prop);

  prop = mrep->GetProperty("N");
  this->Internal->links.addPropertyLink(
    this->Internal->ui.n,
    "value",
    SIGNAL(valueChanged(double)),
    mrep,
    prop);

  prop = mrep->GetProperty("Nt");
  this->Internal->links.addPropertyLink(
    this->Internal->ui.nt,
    "value",
    SIGNAL(valueChanged(double)),
    mrep,
    prop);
}

//-----------------------------------------------------------------------------
MantaDisplay::~MantaDisplay()
{
  delete this->Internal->strAdapt;
  delete this->Internal;
}
