/*=========================================================================

   Program: ParaView
   Module:    pqPendingDisplayUndoElement.cxx

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
#include "pqPendingDisplayUndoElement.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxy.h"
#include "vtkSMDefaultStateLoader.h"

#include "pqPipelineSource.h"
#include "pqApplicationCore.h"
#include "pqServerManagerModel.h"

vtkStandardNewMacro(pqPendingDisplayUndoElement);
vtkCxxRevisionMacro(pqPendingDisplayUndoElement, "1.1");
vtkCxxSetObjectMacro(pqPendingDisplayUndoElement, XMLElement, vtkPVXMLElement);
//-----------------------------------------------------------------------------
pqPendingDisplayUndoElement::pqPendingDisplayUndoElement()
{
  this->XMLElement = 0;
}

//-----------------------------------------------------------------------------
pqPendingDisplayUndoElement::~pqPendingDisplayUndoElement()
{
  this->SetXMLElement(0);
}

//-----------------------------------------------------------------------------
void pqPendingDisplayUndoElement::PendingDisplay(pqPipelineSource* source, 
  int state)
{
  vtkPVXMLElement* elem = vtkPVXMLElement::New();
  elem->SetName("PendingDisplay");
  elem->AddAttribute("id", source->getProxy()->GetSelfIDAsString());
  elem->AddAttribute("state", (state? "1": "0"));
  this->SetXMLElement(elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
void pqPendingDisplayUndoElement::SaveStateInternal(vtkPVXMLElement* root)
{
   if (!this->XMLElement)
    {
    vtkErrorMacro("No state present to save.");
    }
  root->AddNestedElement(this->XMLElement);
}
//-----------------------------------------------------------------------------
void pqPendingDisplayUndoElement::LoadStateInternal(vtkPVXMLElement* element)
{
  if (strcmp(element->GetName(), "PendingDisplay") != 0)
    {
    vtkErrorMacro("Invalid element name: " 
      << element->GetName() << ". Must be PendingDisplay.");
    return;
    }

  this->SetXMLElement(element);
}

//-----------------------------------------------------------------------------
int pqPendingDisplayUndoElement::InternalUndoRedo(bool undo)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smModel = core->getServerManagerModel();

  vtkPVXMLElement* element = this->XMLElement;
  int state = 0;
  element->GetScalarAttribute("state", &state);

  int id = 0;
  element->GetScalarAttribute("id",&id);
  if (!id)
    {
    vtkErrorMacro("Failed to locate proxy id.");
    return 0;
    }

  vtkSMStateLoader* loader = vtkSMDefaultStateLoader::New();
  loader->SetConnectionID(this->GetConnectionID());
  vtkSMProxy* proxy = loader->NewProxy(id);
  loader->Delete();

  if (!proxy)
    {
    vtkErrorMacro("Failed to locate the proxy to register.");
    return 0;
    }

  if ((state && undo) || (!state && !undo))
    {
    core->removeSourcePendingDisplay(smModel->getPQSource(proxy));
    }
  else
    {
    core->addSourcePendingDisplay(smModel->getPQSource(proxy));
    }
  proxy->Delete();
  return 1;
}

//-----------------------------------------------------------------------------
int pqPendingDisplayUndoElement::Undo()
{
  return this->InternalUndoRedo(true);
}

//-----------------------------------------------------------------------------
int pqPendingDisplayUndoElement::Redo()
{
  return this->InternalUndoRedo(false);
}

//-----------------------------------------------------------------------------
void pqPendingDisplayUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

