/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLStack.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkXMLStack.h"

#include "vtkObjectFactory.h"
#include "vtkVector.txx"
#include "vtkVectorIterator.txx"

vtkStandardNewMacro(vtkXMLStack);
vtkCxxRevisionMacro(vtkXMLStack, "1.1");

//----------------------------------------------------------------------------
vtkXMLStack::vtkXMLStack() 
{ 
  this->Elements = vtkXMLStack::ElementsContainer::New();
  this->CompleteCurrentElement = 0;
} 

//----------------------------------------------------------------------------
vtkXMLStack::~vtkXMLStack() 
{ 
  // Remove all elements
  
  this->RemoveAllElements();

  // Delete container

  this->Elements->Delete();

  this->SetCompleteCurrentElement(0);
}

//----------------------------------------------------------------------------
void vtkXMLStack::RemoveAllElements() 
{ 
  if (!this->Elements)
    {
    return;
    }

  // Remove all elements

  vtkXMLStack::ElementSlot *element_slot = NULL;
  vtkXMLStack::ElementsContainerIterator *it = 
    this->Elements->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(element_slot) == VTK_OK)
      {
      delete element_slot;
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
vtkXMLStack::ElementSlot::ElementSlot() 
{ 
  this->Name = 0;
  this->Attributes = 0;
} 

//----------------------------------------------------------------------------
vtkXMLStack::ElementSlot::~ElementSlot() 
{ 
  this->SetName(0);
  this->SetAttributes(0);
}

//----------------------------------------------------------------------------
void vtkXMLStack::ElementSlot::SetName(const char *arg)
{
  if ((!arg && !this->Name) ||
      (this->Name && arg && !strcmp(this->Name, arg)))
    {
    return;
    }

  if (this->Name)
    {
    delete [] this->Name;
    }

  if (arg)
    {
    this->Name = new char[strlen(arg) + 1];
    strcpy(this->Name, arg);
    }
  else
    {
    this->Name = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkXMLStack::ElementSlot::SetAttributes(const char *arg)
{
  if ((!arg && !this->Attributes) ||
      (this->Attributes && arg && !strcmp(this->Attributes, arg)))
    {
    return;
    }

  if (this->Attributes)
    {
    delete [] this->Attributes;
    }

  if (arg)
    {
    this->Attributes = new char[strlen(arg) + 1];
    strcpy(this->Attributes, arg);
    }
  else
    {
    this->Attributes = NULL;
    }
}

//----------------------------------------------------------------------------
int vtkXMLStack::Push(const char *name)
{
  if (!this->Elements)
    {
    return VTK_ERROR;
    }

  vtkXMLStack::ElementSlot *element_slot = new vtkXMLStack::ElementSlot;
  int res = this->Elements->AppendItem(element_slot);
  if (res != VTK_OK)
    {
    delete element_slot;
    }

  element_slot->SetName(name);

  return res;
}

//----------------------------------------------------------------------------
int vtkXMLStack::Pop()
{
  vtkXMLStack::ElementSlot *element_slot = this->GetCurrentElement();
  if (!element_slot)
    {
    return VTK_ERROR;
    }

  delete element_slot;

  return this->Elements->RemoveItem(this->Elements->GetNumberOfItems() - 1);
}

//----------------------------------------------------------------------------
vtkXMLStack::ElementSlot*
vtkXMLStack::GetElement(int id)
{
  if (!this->Elements || !(this->Elements->GetNumberOfItems() - id))
    {
    return 0;
    }

  vtkXMLStack::ElementSlot *element_slot = 0;
  if (this->Elements->GetItem(this->Elements->GetNumberOfItems() - 1 - id, 
                              element_slot) != VTK_OK)
    {
    return 0;
    }

  return element_slot;
}

//----------------------------------------------------------------------------
vtkXMLStack::ElementSlot*
vtkXMLStack::GetCurrentElement()
{
  return this->GetElement(0);
}

//----------------------------------------------------------------------------
char* vtkXMLStack::GetCurrentElementName()
{
  vtkXMLStack::ElementSlot *element_slot = this->GetCurrentElement();
  if (element_slot)
    {
    return element_slot->GetName();
    }

  return 0;
}

//----------------------------------------------------------------------------
char* vtkXMLStack::GetPreviousElementName()
{
  vtkXMLStack::ElementSlot *element_slot = this->GetElement(1);
  if (element_slot)
    {
    return element_slot->GetName();
    }

  return 0;
}

//----------------------------------------------------------------------------
char* vtkXMLStack::GetCurrentElementAttributes()
{
  vtkXMLStack::ElementSlot *element_slot = this->GetCurrentElement();
  if (element_slot)
    {
    return element_slot->GetAttributes();
    }

  return 0;
}

//----------------------------------------------------------------------------
char* vtkXMLStack::SetCurrentElementAttributes(const char **args)
{
  vtkXMLStack::ElementSlot *element_slot = this->GetCurrentElement();
  if (!element_slot)
    {
    return 0;
    }

  ostrstream attributes;
  this->CollateAttributes(args, attributes);
  attributes << ends;
  
  element_slot->SetAttributes(attributes.str());
  attributes.rdbuf()->freeze(0);

  return element_slot->GetAttributes();
}

//----------------------------------------------------------------------------
void vtkXMLStack::CollateAttributes(const char **args, ostream &os)
{
  if (args)
    {
    for (int i = 0; args[i] && args[i + 1]; i += 2)
      {
      os << args[i] << "=\"" << args[i + 1] << (args[i + 2] ? "\" " : "\"");
      }
    }
}

//----------------------------------------------------------------------------
char* vtkXMLStack::GetCompleteCurrentElement()
{
  vtkXMLStack::ElementSlot *element_slot = this->GetCurrentElement();
  if (!element_slot)
    {
    return 0;
    }

  char *attribs = element_slot->GetAttributes();
  ostrstream complete;
  complete << '<' << element_slot->GetName();
  if (attribs)
    {
    complete << ' ' << attribs;
    }
  complete << "/>" << ends;

  this->SetCompleteCurrentElement(complete.str());
  complete.rdbuf()->freeze(0);

  return this->CompleteCurrentElement;
}

//----------------------------------------------------------------------------
void vtkXMLStack::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
