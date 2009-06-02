/*=========================================================================

   Program: ParaView
   Module:    pqMultiInputView.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

#include "pqMultiInputView.h"

#include <pqApplicationCore.h>
#include <pqRepresentation.h>

#include <assert.h>
#include <vector>

////////////////////////////////////////////////////////////////////////////////////
// pqMultiInputView::implementation

class pqMultiInputView::implementation
{
public:
  implementation()
  {
  }
};

////////////////////////////////////////////////////////////////////////////////////
// pqMultiInputView

pqMultiInputView::pqMultiInputView(
  const QString& viewmoduletype, 
  const QString& group, 
  const QString& name, 
  vtkSMViewProxy* viewmodule, 
  pqServer* server, 
  QObject* p) :
  pqView(viewmoduletype, group, name, viewmodule, server, p),
  Implementation(new implementation())
{
  this->connect(this, SIGNAL(representationAdded(pqRepresentation*)),
    SLOT(onRepresentationAdded(pqRepresentation*)));
  this->connect(this, SIGNAL(representationVisibilityChanged(pqRepresentation*, bool)),
    SLOT(onRepresentationVisibilityChanged(pqRepresentation*, bool)));
  this->connect(this, SIGNAL(representationRemoved(pqRepresentation*)),
    SLOT(onRepresentationRemoved(pqRepresentation*)));
  this->connect(this, SIGNAL(endRender()), this, SLOT(renderInternal()));
  this->connect(pqApplicationCore::instance(), SIGNAL(stateLoaded()),
    SLOT(onStateLoaded()));
}

pqMultiInputView::~pqMultiInputView()
{
  delete this->Implementation;
}

bool pqMultiInputView::saveImage(int, int, const QString& )
{
  return false;
}

vtkImageData* pqMultiInputView::captureImage(int)
{
  return 0;
}

void pqMultiInputView::onRepresentationAdded(pqRepresentation* representation)
{
  //QObject::connect(representation, SIGNAL(visibilityChanged(bool)), this, SLOT(onRepresentationVisibilityChanged(bool)));
  QObject::connect(representation, SIGNAL(updated()), this, SLOT(onRepresentationUpdated()));

  this->showRepresentation(representation);
}

void pqMultiInputView::onRepresentationVisibilityChanged(pqRepresentation *rep, bool visible)
{
  if(visible)
    this->showRepresentation(rep);
  else
    this->hideRepresentation(rep);
}

void pqMultiInputView::onRepresentationUpdated()
{
  pqRepresentation* const sender = qobject_cast<pqRepresentation*>(QObject::sender());
  assert(sender);

  this->updateRepresentation(sender);
}

void pqMultiInputView::onRepresentationRemoved(pqRepresentation* representation)
{
  QObject::disconnect(representation, 0, this, 0);

  this->hideRepresentation(representation);
}

void pqMultiInputView::onStateLoaded()
{
  QList<pqRepresentation*> representations = this->getRepresentations();

  for(int i = 0; i != representations.size(); ++i)
    this->onRepresentationAdded(representations[i]);

  this->render();
}

