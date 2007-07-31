/*=========================================================================

   Program: ParaView
   Module:    pqSelectionLinksManager.cxx

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

========================================================================*/
#include "pqSelectionLinksManager.h"

// Server Manager Includes.
#include "vtkSMSelectionLink.h"
#include "vtkSmartPointer.h"

// Qt Includes.
#include <QMap>

// ParaView Includes.
#include "pqPipelineSource.h"
#include "pqOutputPort.h"
#include "pqDataRepresentation.h"
#include "pqServerManagerModel.h"
#include "pqApplicationCore.h"

class pqSelectionLinksManager::pqInternal
{
public:
  typedef QMap<pqOutputPort*, vtkSmartPointer<vtkSMSelectionLink> > LinksType;
  LinksType Links;
};

//-----------------------------------------------------------------------------
pqSelectionLinksManager::pqSelectionLinksManager()
{
  this->Internal = new pqInternal();

  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smmodel, SIGNAL(sourceAdded(pqPipelineSource*)),
    this, SLOT(sourceAdded(pqPipelineSource*)));
  QObject::connect(smmodel, SIGNAL(sourceRemoved(pqPipelineSource*)),
    this, SLOT(sourceRemoved(pqPipelineSource*)));
}

//-----------------------------------------------------------------------------
pqSelectionLinksManager::~pqSelectionLinksManager()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqSelectionLinksManager::sourceAdded(pqPipelineSource* source)
{
  for (int cc=0; cc < source->getNumberOfOutputPorts(); cc++)
    {
    pqOutputPort* opport = source->getOutputPort(cc);
    vtkSMSelectionLink* link = vtkSMSelectionLink::New();
    this->Internal->Links[opport] = link;
    link->Delete();

    QObject::connect(
      opport, SIGNAL(representationAdded(pqOutputPort*, pqDataRepresentation*)),
      this, SLOT(representationAdded(pqOutputPort*, pqDataRepresentation*)));
    QObject::connect(
      opport, SIGNAL(representationRemoved(pqOutputPort*, pqDataRepresentation*)),
      this, SLOT(representationRemoved(pqOutputPort*, pqDataRepresentation*)));

    /// Process already present representations.
    QList<pqDataRepresentation*> reprs = opport->getRepresentations(NULL);
    foreach (pqDataRepresentation* repr, reprs)
      {
      this->representationAdded(opport, repr);
      }
    }
}

//-----------------------------------------------------------------------------
void pqSelectionLinksManager::sourceRemoved(pqPipelineSource* source)
{
  for (int cc=0; cc < source->getNumberOfOutputPorts(); cc++)
    {
    pqOutputPort* opport = source->getOutputPort(cc);
    pqInternal::LinksType::iterator iter = this->Internal->Links.find(opport);
    if (iter != this->Internal->Links.end())
      {
      iter.value()->RemoveAllLinks();
      this->Internal->Links.erase(iter);
      }

    QObject::disconnect(opport, 0, this, 0);
    }
}

//-----------------------------------------------------------------------------
void pqSelectionLinksManager::representationAdded(
  pqOutputPort* opport, pqDataRepresentation* repr)
{
  pqInternal::LinksType::iterator iter = this->Internal->Links.find(opport);
  if (iter != this->Internal->Links.end())
    {
    iter.value()->AddSelectionLink(repr->getProxy(), 
      vtkSMLink::INPUT|vtkSMLink::OUTPUT);
    }
}

//-----------------------------------------------------------------------------
void pqSelectionLinksManager::representationRemoved(
  pqOutputPort* opport, pqDataRepresentation* repr)
{
  pqInternal::LinksType::iterator iter = this->Internal->Links.find(opport);
  if (iter != this->Internal->Links.end())
    {
    iter.value()->RemoveSelectionLink(repr->getProxy());
    }
}
