/*=========================================================================

   Program: ParaView
   Module:    pqCloseViewUndoElement.cxx

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

========================================================================*/
#include "pqCloseViewUndoElement.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"

#include "pqApplicationCore.h"
#include "pqViewManager.h"

vtkStandardNewMacro(pqCloseViewUndoElement);
//----------------------------------------------------------------------------
pqCloseViewUndoElement::pqCloseViewUndoElement()
{
}

//----------------------------------------------------------------------------
pqCloseViewUndoElement::~pqCloseViewUndoElement()
{
}

//----------------------------------------------------------------------------
bool pqCloseViewUndoElement::CanLoadState(vtkPVXMLElement* elem)
{
  return (elem && elem->GetName() && 
    strcmp(elem->GetName(), "CloseView") == 0);
}

//----------------------------------------------------------------------------
void pqCloseViewUndoElement::CloseView(
  pqMultiView::Index frameIndex, vtkPVXMLElement* state)
{
  vtkPVXMLElement* elem = vtkPVXMLElement::New();
  elem->SetName("CloseView");
  elem->AddAttribute("index", frameIndex.getString().toAscii().data());
  elem->AddNestedElement(state);
  this->SetXMLElement(elem);
  elem->Delete();
}

//----------------------------------------------------------------------------
int pqCloseViewUndoElement::Undo()
{
  vtkPVXMLElement* state = this->XMLElement->GetNestedElement(0);
  pqViewManager* manager = qobject_cast<pqViewManager*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_MANAGER"));
  if (!manager)
    {
    vtkErrorMacro("Failed to locate the multi view manager. "
      << "MULTIVIEW_MANAGER must be registered with application core.");
    return 0;
    }
  manager->loadState(state, this->GetProxyLocator());
  return 1;
}

//----------------------------------------------------------------------------
int pqCloseViewUndoElement::Redo()
{
  pqMultiView::Index index;
  index.setFromString(this->XMLElement->GetAttribute("index"));

  pqMultiView* manager = qobject_cast<pqMultiView*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_MANAGER"));
  if (!manager)
    {
    vtkErrorMacro("Failed to locate the multi view manager. "
      << "MULTIVIEW_MANAGER must be registered with application core.");
    return 0;
    }
  manager->removeWidget(manager->widgetOfIndex(index));
  return 1;
}

//----------------------------------------------------------------------------
void pqCloseViewUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


