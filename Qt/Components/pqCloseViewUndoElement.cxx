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
#include "vtkSMProxyLocator.h"
#include "vtkSMStateLoader.h"
#include "vtkSMProxyManager.h"
#include "vtkSMCacheBasedProxyLocator.h"

#include "pqApplicationCore.h"
#include "pqViewManager.h"

vtkStandardNewMacro(pqCloseViewUndoElement);
//----------------------------------------------------------------------------
pqCloseViewUndoElement::pqCloseViewUndoElement()
{
  this->Index = NULL;
  this->ViewStateCache = vtkSMCacheBasedProxyLocator::New();
  this->SetSession(NULL); // Maybe keep the one use to create the Element for state loading...
}

//----------------------------------------------------------------------------
pqCloseViewUndoElement::~pqCloseViewUndoElement()
{
  this->SetIndex(NULL);
  this->ViewStateCache->Delete();
}

//----------------------------------------------------------------------------
void pqCloseViewUndoElement::CloseView(
  pqMultiView::Index frameIndex, vtkPVXMLElement* state)
{
  this->SetIndex(frameIndex.getString().toAscii().data());
  this->State = state;
}

//----------------------------------------------------------------------------
int pqCloseViewUndoElement::Undo()
{
    pqViewManager* manager = qobject_cast<pqViewManager*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_MANAGER"));
  if (!manager)
    {
    vtkErrorMacro("Failed to locate the multi view manager. "
      << "MULTIVIEW_MANAGER must be registered with application core.");
    return 0;
    }
  manager->loadState(this->State, this->ViewStateCache);
  this->ViewStateCache->GetLocatedProxies(this->UndoSetWorkingContext);
  this->ViewStateCache->Clear();
  return 1;
}

//----------------------------------------------------------------------------
int pqCloseViewUndoElement::Redo()
{
  pqMultiView::Index index;
  index.setFromString(this->Index);

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


