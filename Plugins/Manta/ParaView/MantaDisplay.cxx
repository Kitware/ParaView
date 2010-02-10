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
#include "pqActiveView.h"
#include "pqApplicationCore.h"
#include "pqDisplayPanel.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServerManagerSelectionModel.h"
#include "vtkSMMantaRepresentation.h"
#include "pqDataRepresentation.h"

#include "pqMantaView.h"

#include <iostream>

using namespace std;

class MantaDisplay::pqInternal
{
public:
  Ui::MantaDisplay ui;
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

  QObject::connect(this->Internal->ui.material,
                   SIGNAL(currentIndexChanged(int)),
                   this, SLOT(applyChanges()));
  QObject::connect(this->Internal->ui.reflectance,
                   SIGNAL(valueChanged(double)),
                   this, SLOT(applyChanges()));
  QObject::connect(this->Internal->ui.thickness,
                   SIGNAL(valueChanged(double)),
                   this, SLOT(applyChanges()));
  QObject::connect(this->Internal->ui.eta,
                   SIGNAL(valueChanged(double)),
                   this, SLOT(applyChanges()));
  QObject::connect(this->Internal->ui.n,
                   SIGNAL(valueChanged(double)),
                   this, SLOT(applyChanges()));
  QObject::connect(this->Internal->ui.nt,
                   SIGNAL(valueChanged(double)),
                   this, SLOT(applyChanges()));
}

//-----------------------------------------------------------------------------
MantaDisplay::~MantaDisplay()
{
}


//-----------------------------------------------------------------------------
void MantaDisplay::applyChanges()
{
  //find out what view and representation with that view we are modifying
  pqMantaView* view = qobject_cast<pqMantaView*>
    (pqActiveView::instance().current());
  if(!view)
    {
    return;
    }

  pqServerManagerModelItem *item =
    pqApplicationCore::instance()->getSelectionModel()->currentItem();
  if (!item)
    {
    return;
    }
  pqOutputPort* opPort = qobject_cast<pqOutputPort*>(item);
  pqPipelineSource *source = opPort? opPort->getSource() : 
    qobject_cast<pqPipelineSource*>(item);
  if (!source)
    {
    return;
    }
  pqDataRepresentation* representation = 
    source->getRepresentation(0, view);
  if (!representation)
    {
    return;
    }
  vtkSMMantaRepresentation *mrep = vtkSMMantaRepresentation::SafeDownCast
    (representation->getProxy());
  if (!mrep)
    {
    return;
    }

  QString s = this->Internal->ui.material->currentText();
  mrep->SetMaterialType((char*)s.toStdString().c_str());
  mrep->SetReflectance(this->Internal->ui.reflectance->value());
  mrep->SetThickness(this->Internal->ui.thickness->value());
  mrep->SetEta(this->Internal->ui.eta->value());
  mrep->SetN(this->Internal->ui.n->value());
  mrep->SetNt(this->Internal->ui.nt->value());
}
