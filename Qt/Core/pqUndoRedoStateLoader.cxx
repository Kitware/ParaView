/*=========================================================================

   Program:   ParaView
   Module:    pqUndoRedoStateLoader.cxx

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

#include "pqUndoRedoStateLoader.h"

#include "vtkObjectFactory.h"
#include "vtkUndoSet.h"

#include "pqPendingDisplayUndoElement.h"


vtkStandardNewMacro(pqUndoRedoStateLoader);
vtkCxxRevisionMacro(pqUndoRedoStateLoader, "1.1");
//-----------------------------------------------------------------------------
pqUndoRedoStateLoader::pqUndoRedoStateLoader()
{
}

//-----------------------------------------------------------------------------
pqUndoRedoStateLoader::~pqUndoRedoStateLoader()
{
}
//-----------------------------------------------------------------------------
void pqUndoRedoStateLoader::HandleTag(const char* tagName, 
  vtkPVXMLElement* root)
{
  if (!this->UndoSet)
    {
    return; //sanity check.
    }
  if (strcmp(tagName, "PendingDisplay") == 0)
    {
    this->HandlePendingDisplay(root);
    }
  else
    {
    this->Superclass::HandleTag(tagName, root);
    }
}

//-----------------------------------------------------------------------------
void pqUndoRedoStateLoader::HandlePendingDisplay(vtkPVXMLElement* root)
{
  pqPendingDisplayUndoElement* elem = pqPendingDisplayUndoElement::New();
  elem->LoadState(root);
  elem->SetConnectionID(this->ConnectionID);
  this->UndoSet->AddElement(elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void pqUndoRedoStateLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
