/*=========================================================================

   Program: ParaView
   Module:    pqSingleInputView.cxx

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

#include "pqSingleInputView.h"

#include <pqApplicationCore.h>
#include <pqRepresentation.h>

#include <vtkstd/algorithm>
#include <vtkstd/vector>

////////////////////////////////////////////////////////////////////////////////////
// pqSingleInputView::implementation

class pqSingleInputView::implementation
{
public:
  implementation() :
    VisibleRepresentation(0)
  {
  }

  vtkstd::vector<pqRepresentation*> Representations;
  pqRepresentation* VisibleRepresentation;
};

////////////////////////////////////////////////////////////////////////////////////
// pqSingleInputView

pqSingleInputView::pqSingleInputView(
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
}

pqSingleInputView::~pqSingleInputView()
{
  delete this->Implementation;
}

bool pqSingleInputView::saveImage(int, int, const QString& )
{
  return false;
}

vtkImageData* pqSingleInputView::captureImage(int)
{
  return 0;
}

pqRepresentation* pqSingleInputView::visibleRepresentation()
{
  return this->Implementation->VisibleRepresentation;
}

void pqSingleInputView::onRepresentationAdded(pqRepresentation* representation)
{
  for(size_t i = 0; i != this->Implementation->Representations.size(); ++i)
    this->Implementation->Representations[i]->setVisible(false);

  this->Implementation->Representations.push_back(representation);
  this->Implementation->VisibleRepresentation = representation;

  QObject::connect(representation, SIGNAL(updated()), this, SLOT(onRepresentationUpdated()));

  this->showRepresentation(representation);
}

void pqSingleInputView::onRepresentationVisibilityChanged(pqRepresentation* representation, bool visible)
{
  if(visible && representation != this->Implementation->VisibleRepresentation)
    {
    if(this->Implementation->VisibleRepresentation)
      {
      // No need to explicitly call this since setting visibility to false will
      // result in a call to onRepresentationVisibilityChanged() which would
      // then fall into the else condition and call this function.
      // ** this->hideRepresentation(this->Implementation->VisibleRepresentation);
      this->Implementation->VisibleRepresentation->setVisible(false);
      }

    this->Implementation->VisibleRepresentation = representation;
    this->showRepresentation(this->Implementation->VisibleRepresentation);
    }
  else if(!visible && representation == this->Implementation->VisibleRepresentation)
    {
    if(this->Implementation->VisibleRepresentation)
      {
      this->hideRepresentation(this->Implementation->VisibleRepresentation);
      this->Implementation->VisibleRepresentation = 0;
      }
    }
}

/** \deprecated */
void pqSingleInputView::onRepresentationUpdated()
{
  pqRepresentation* const representation = qobject_cast<pqRepresentation*>(QObject::sender());
  this->updateRepresentation(representation);
}

void pqSingleInputView::onRepresentationRemoved(pqRepresentation* representation)
{
  QObject::disconnect(representation, 0, this, 0);

  this->Implementation->Representations.erase(
    vtkstd::remove(
      this->Implementation->Representations.begin(),
      this->Implementation->Representations.end(),
      representation),
    this->Implementation->Representations.end());
  
  if(this->Implementation->VisibleRepresentation == representation)
    this->hideRepresentation(representation);
}

void pqSingleInputView::updateRepresentation(pqRepresentation*)
{
  /** \deprecated */
}

