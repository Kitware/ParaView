/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
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

#include "vtkKWScaleSet.h"

#include "vtkKWApplication.h"
#include "vtkKWScale.h"
#include "vtkLinkedList.txx"
#include "vtkLinkedListIterator.txx"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkKWScaleSet);
vtkCxxRevisionMacro(vtkKWScaleSet, "1.1");

int vtkvtkKWScaleSetCommand(ClientData cd, Tcl_Interp *interp,
                                  int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWScaleSet::vtkKWScaleSet()
{
  this->PackHorizontally = 0;
  this->MaximumNumberOfWidgetInPackingDirection = 0;
  this->PadX = 0;
  this->PadY = 0;
  this->Scales = vtkKWScaleSet::ScalesContainer::New();
}

//----------------------------------------------------------------------------
vtkKWScaleSet::~vtkKWScaleSet()
{
  this->DeleteAllScales();

  // Delete the container

  this->Scales->Delete();
}

//----------------------------------------------------------------------------
void vtkKWScaleSet::DeleteAllScales()
{
  // Delete all scales

  vtkKWScaleSet::ScaleSlot *scale_slot = NULL;
  vtkKWScaleSet::ScalesContainerIterator *it = 
    this->Scales->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(scale_slot) == VTK_OK)
      {
      if (scale_slot->Scale)
        {
        scale_slot->Scale->Delete();
        scale_slot->Scale = NULL;
        }
      delete scale_slot;
      }
    it->GoToNextItem();
    }
  it->Delete();

  this->Scales->RemoveAllItems();
}

//----------------------------------------------------------------------------
vtkKWScaleSet::ScaleSlot* 
vtkKWScaleSet::GetScaleSlot(int id)
{
  vtkKWScaleSet::ScaleSlot *scale_slot = NULL;
  vtkKWScaleSet::ScaleSlot *found = NULL;
  vtkKWScaleSet::ScalesContainerIterator *it = 
    this->Scales->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(scale_slot) == VTK_OK && scale_slot->Id == id)
      {
      found = scale_slot;
      break;
      }
    it->GoToNextItem();
    }
  it->Delete();

  return found;
}

//----------------------------------------------------------------------------
vtkKWScale* vtkKWScaleSet::GetScale(int id)
{
  vtkKWScaleSet::ScaleSlot *scale_slot = 
    this->GetScaleSlot(id);

  if (!scale_slot)
    {
    return NULL;
    }

  return scale_slot->Scale;
}

//----------------------------------------------------------------------------
int vtkKWScaleSet::HasScale(int id)
{
  return this->GetScaleSlot(id) ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkKWScaleSet::Create(vtkKWApplication *app, const char *args)
{
  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("The scale set is already created");
    return;
    }

  this->SetApplication(app);

  // Create the container frame

  this->Script("frame %s %s", this->GetWidgetName(), args ? args : "");

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWScaleSet::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  vtkKWScaleSet::ScaleSlot *scale_slot = NULL;
  vtkKWScaleSet::ScalesContainerIterator *it = 
    this->Scales->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(scale_slot) == VTK_OK)
      {
      scale_slot->Scale->SetEnabled(this->Enabled);
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
int vtkKWScaleSet::AddScale(int id, 
                            vtkKWObject *object, 
                            const char *method_and_arg_string,
                            const char *balloonhelp_string)
{
  // Widget must have been created

  if (!this->IsCreated())
    {
    vtkErrorMacro("The scale set must be created before any scale "
                  "is added.");
    return 0;
    }

  // Check if the new scale has a unique id

  if (this->HasScale(id))
    {
    vtkErrorMacro("A scale with that id (" << id << ") already exists "
                  "in the scale set.");
    return 0;
    }

  // Add the scale slot to the manager

  vtkKWScaleSet::ScaleSlot *scale_slot = 
    new vtkKWScaleSet::ScaleSlot;

  if (this->Scales->AppendItem(scale_slot) != VTK_OK)
    {
    vtkErrorMacro("Error while adding a scale to the set.");
    delete scale_slot;
    return 0;
    }
  
  // Create the scale

  scale_slot->Scale = vtkKWScale::New();
  scale_slot->Id = id;

  scale_slot->Scale->SetParent(this);
  scale_slot->Scale->Create(this->Application, 0);
  scale_slot->Scale->SetEnabled(this->Enabled);

  // Set command and balloon help, if any

  if (object && method_and_arg_string)
    {
    scale_slot->Scale->SetCommand(object, method_and_arg_string);
    }

  if (balloonhelp_string)
    {
    scale_slot->Scale->SetBalloonHelpString(balloonhelp_string);
    }

  // Pack the scale

  this->Pack();

  return 1;
}

// ----------------------------------------------------------------------------
void vtkKWScaleSet::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  ostrstream tk_cmd;

  tk_cmd << "catch {eval grid forget [grid slaves " << this->GetWidgetName() 
         << "]}" << endl;

  vtkKWScaleSet::ScaleSlot *scale_slot = NULL;
  vtkKWScaleSet::ScalesContainerIterator *it = 
    this->Scales->NewIterator();

  int col = 0;
  int row = 0;

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(scale_slot) == VTK_OK)
      {
      tk_cmd << "grid " << scale_slot->Scale->GetWidgetName() 
             << " -sticky news"
             << " -column " << (this->PackHorizontally ? col : row)
             << " -row " << (this->PackHorizontally ? row : col)
             << " -padx " << this->PadX
             << " -pady " << this->PadY
             << endl;
      col++;
      if (this->MaximumNumberOfWidgetInPackingDirection &&
          col >= this->MaximumNumberOfWidgetInPackingDirection)
        {
        col = 0;
        row++;
        }
      }
    it->GoToNextItem();
    }
  it->Delete();

  // Weights
  
  int i;
  int maxcol = (row > 0) ? this->MaximumNumberOfWidgetInPackingDirection : col;
  for (i = 0; i < maxcol; i++)
    {
    tk_cmd << "grid " << (this->PackHorizontally ? "column" : "row") 
           << "configure " << this->GetWidgetName() << " " 
           << i << " -weight 1" << endl;
    }

  for (i = 0; i <= row; i++)
    {
    tk_cmd << "grid " << (this->PackHorizontally ? "row" : "column") 
           << "configure " << this->GetWidgetName() << " " 
           << i << " -weight 1" << endl;
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

// ----------------------------------------------------------------------------
void vtkKWScaleSet::SetPackHorizontally(int _arg)
{
  if (this->PackHorizontally == _arg)
    {
    return;
    }
  this->PackHorizontally = _arg;
  this->Modified();

  this->Pack();
}

// ----------------------------------------------------------------------------
void vtkKWScaleSet::SetMaximumNumberOfWidgetInPackingDirection(int _arg)
{
  if (this->MaximumNumberOfWidgetInPackingDirection == _arg)
    {
    return;
    }
  this->MaximumNumberOfWidgetInPackingDirection = _arg;
  this->Modified();

  this->Pack();
}

// ----------------------------------------------------------------------------
void vtkKWScaleSet::SetPadding(int x, int y)
{
  if (this->PadX == x && this->PadY == y)
    {
    return;
    }

  this->PadX = x;
  this->PadY = y;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWScaleSet::HideScale(int id)
{
  this->SetScaleVisibility(id, 0);
}

//----------------------------------------------------------------------------
void vtkKWScaleSet::ShowScale(int id)
{
  this->SetScaleVisibility(id, 1);
}

//----------------------------------------------------------------------------
void vtkKWScaleSet::SetScaleVisibility(int id, int flag)
{
  vtkKWScaleSet::ScaleSlot *scale_slot = 
    this->GetScaleSlot(id);

  if (scale_slot && scale_slot->Scale)
    {
    this->Script("grid %s %s", 
                 (flag ? "" : "remove"),
                 scale_slot->Scale->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
int vtkKWScaleSet::GetNumberOfVisibleScales()
{
  if (!this->IsCreated())
    {
    return 0;
    }
  return atoi(this->Script("llength [grid slaves %s]", this->GetWidgetName()));
}

//----------------------------------------------------------------------------
void vtkKWScaleSet::SetBorderWidth(int bd)
{
  ostrstream tk_cmd;

  vtkKWScaleSet::ScaleSlot *scale_slot = NULL;
  vtkKWScaleSet::ScalesContainerIterator *it = 
    this->Scales->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(scale_slot) == VTK_OK)
      {
      tk_cmd << scale_slot->Scale->GetWidgetName() 
         << " config -bd " << bd << endl;
      }
    it->GoToNextItem();
    }
  it->Delete();

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWScaleSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PackHorizontally: " 
     << (this->PackHorizontally ? "On" : "Off") << endl;

  os << indent << "MaximumNumberOfWidgetInPackingDirection: " 
     << (this->MaximumNumberOfWidgetInPackingDirection ? "On" : "Off") << endl;
}

