/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWExtent.cxx
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
#include "vtkKWExtent.h"

#include "vtkKWApplication.h"
#include "vtkKWImageLabel.h"
#include "vtkKWRange.h"
#include "vtkKWTkUtilities.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWExtent );
vtkCxxRevisionMacro(vtkKWExtent, "1.21");

//----------------------------------------------------------------------------
int vtkKWExtentCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWExtent::vtkKWExtent()
{
  this->CommandFunction = vtkKWExtentCommand;
  this->Command = NULL;

  this->XRange = vtkKWRange::New();
  this->YRange = vtkKWRange::New();
  this->ZRange = vtkKWRange::New();

  this->Extent[0] = 0;
  this->Extent[1] = 0;
  this->Extent[2] = 0;
  this->Extent[3] = 0;
  this->Extent[4] = 0;
  this->Extent[5] = 0;
}

//----------------------------------------------------------------------------
vtkKWExtent::~vtkKWExtent()
{
  if (this->Command)
    {
    delete [] this->Command;
    }

  this->XRange->Delete();  
  this->YRange->Delete();  
  this->ZRange->Delete();  
}

//----------------------------------------------------------------------------
void vtkKWExtent::Create(vtkKWApplication *app, const char *args)
{
  const char *wname;

  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("Extent already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -bd 0 %s", wname, (args ? args : ""));

  this->XRange->SetParent(this);
  this->XRange->Create(this->Application, "");
  this->XRange->SetCommand(this, "ExtentSelected");
  this->XRange->AdjustResolutionOn();
  this->XRange->ShowLabelOn();
  this->XRange->SetLabel("X (Units)");
  this->XRange->ShowEntriesOn();
  
  this->YRange->SetParent(this);
  this->YRange->Create(this->Application, "");
  this->YRange->AdjustResolutionOn();
  this->YRange->SetCommand(this, "ExtentSelected");
  this->YRange->ShowLabelOn();
  this->YRange->SetLabel("Y (Units)");
  this->YRange->ShowEntriesOn();

  this->ZRange->SetParent(this);
  this->ZRange->Create(this->Application, "");
  this->ZRange->AdjustResolutionOn();
  this->ZRange->SetCommand(this, "ExtentSelected");
  this->ZRange->ShowLabelOn();
  this->ZRange->SetLabel("Z (Units)");
  this->ZRange->ShowEntriesOn();
  
  // Pack the label and the option menu

  this->Pack();

  // Update enable state

  this->UpdateEnableState();
}

// ----------------------------------------------------------------------------
void vtkKWExtent::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  this->XRange->UnpackSiblings();

  int is_horiz = 
    (this->XRange->GetOrientation() == vtkKWRange::ORIENTATION_HORIZONTAL);

  // Repack everything

  ostrstream tk_cmd;

  tk_cmd << "pack "
         << this->XRange->GetWidgetName() << " "
         << this->YRange->GetWidgetName() << " "
         << this->ZRange->GetWidgetName() << " "
         << "-padx 2 -pady 2 -fill both -expand yes -anchor w "
         << "-side " << (is_horiz ? "top" : "left") << endl;
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetExtentRange(float *er)
{
  this->SetExtentRange(er[0],er[1],er[2],er[3],er[4],er[5]);
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetExtentRange(float x1, float x2, float y1, float y2, 
                                 float z1, float z2)
{
  float res = 512.0;
  this->XRange->SetResolution((x2<x1)?((x1-x2)/res):((x2-x1)/res));
  this->YRange->SetResolution((y2<y1)?((y1-y2)/res):((y2-y1)/res));
  this->ZRange->SetResolution((y2<y1)?((y1-y2)/res):((y2-y1)/res));
  
  this->XRange->SetWholeRange(x1, x2);
  this->YRange->SetWholeRange(y1, y2);
  this->ZRange->SetWholeRange(z1, z2);

  float ex1, ex2, ey1, ey2, ez1, ez2;

  ex1 = (this->Extent[0] < x1 || this->Extent[0] > x2) ? x1 : this->Extent[0];
  ex2 = (this->Extent[1] < x1 || this->Extent[1] > x2) ? x2 : this->Extent[1];
  ey1 = (this->Extent[2] < y1 || this->Extent[2] > y2) ? y1 : this->Extent[2];
  ey2 = (this->Extent[3] < y1 || this->Extent[3] > y2) ? y2 : this->Extent[3];
  ez1 = (this->Extent[4] < z1 || this->Extent[4] > z2) ? z1 : this->Extent[4];
  ez2 = (this->Extent[5] < z1 || this->Extent[5] > z2) ? z2 : this->Extent[5];

  this->SetExtent(ex1, ex2, ey1, ey2, ez1, ez2);
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetExtent(float x1, float x2, float y1, float y2, float z1, float z2)
{
  if (this->Extent[0] == x1 &&
      this->Extent[1] == x2 &&
      this->Extent[2] == y1 &&
      this->Extent[3] == y2 &&
      this->Extent[4] == z1 &&
      this->Extent[5] == z2)
    {
    return;
    }

  this->XRange->SetRange(x1, x2);
  this->YRange->SetRange(y1, y2);
  this->ZRange->SetRange(z1, z2);
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetExtent(float *er)
{
  this->SetExtent(er[0],er[1],er[2],er[3],er[4],er[5]);
}

//----------------------------------------------------------------------------
void vtkKWExtent::ExtentSelected()
{
  // first check to see if anything changed.
  // Normally something should have changed, but 
  // on initialization this isn;t the case.
  if (this->Extent[0] == this->XRange->GetRange()[0] &&
      this->Extent[1] == this->XRange->GetRange()[1] &&
      this->Extent[2] == this->YRange->GetRange()[0] &&
      this->Extent[3] == this->YRange->GetRange()[1] &&
      this->Extent[4] == this->ZRange->GetRange()[0] &&
      this->Extent[5] == this->ZRange->GetRange()[1])
    {
    return;
    }
  
  this->Extent[0] = this->XRange->GetRange()[0];
  this->Extent[1] = this->XRange->GetRange()[1];
  this->Extent[2] = this->YRange->GetRange()[0];
  this->Extent[3] = this->YRange->GetRange()[1];
  this->Extent[4] = this->ZRange->GetRange()[0];
  this->Extent[5] = this->ZRange->GetRange()[1];
 
  if ( this->Command )
    {
    this->Script("eval %s",this->Command);
    }
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetCommand(vtkKWObject* CalledObject, const char *CommandString)
{ 
  if (this->Command)
    {
    delete [] this->Command;
    }
  ostrstream command;
  command << CalledObject->GetTclName() << " " << CommandString << ends;

  this->Command = command.str();
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetStartCommand(vtkKWObject* Object, 
                                  const char *MethodAndArgString)
{ 
  if (this->XRange)
    {
    this->XRange->SetStartCommand(Object, MethodAndArgString);
    }

  if (this->YRange)
    {
    this->YRange->SetStartCommand(Object, MethodAndArgString);
    }

  if (this->ZRange)
    {
    this->ZRange->SetStartCommand(Object, MethodAndArgString);
    }
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetEndCommand(vtkKWObject* Object, 
                                const char *MethodAndArgString)
{ 
  if (this->XRange)
    {
    this->XRange->SetEndCommand(Object, MethodAndArgString);
    }

  if (this->YRange)
    {
    this->YRange->SetEndCommand(Object, MethodAndArgString);
    }

  if (this->ZRange)
    {
    this->ZRange->SetEndCommand(Object, MethodAndArgString);
    }
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetDisableCommands(int v)
{ 
  if (this->XRange)
    {
    this->XRange->SetDisableCommands(v);
    }

  if (this->YRange)
    {
    this->YRange->SetDisableCommands(v);
    }

  if (this->ZRange)
    {
    this->ZRange->SetDisableCommands(v);
    }
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetOrientation(int v)
{ 
  if (this->XRange)
    {
    this->XRange->SetOrientation(v);
    }

  if (this->YRange)
    {
    this->YRange->SetOrientation(v);
    }

  if (this->ZRange)
    {
    this->ZRange->SetOrientation(v);
    }

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetLabelPosition(int v)
{ 
  if (this->XRange)
    {
    this->XRange->SetLabelPosition(v);
    }

  if (this->YRange)
    {
    this->YRange->SetLabelPosition(v);
    }

  if (this->ZRange)
    {
    this->ZRange->SetLabelPosition(v);
    }
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetEntriesPosition(int v)
{ 
  if (this->XRange)
    {
    this->XRange->SetEntriesPosition(v);
    }

  if (this->YRange)
    {
    this->YRange->SetEntriesPosition(v);
    }

  if (this->ZRange)
    {
    this->ZRange->SetEntriesPosition(v);
    }
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetSliderCanPush(int v)
{ 
  if (this->XRange)
    {
    this->XRange->SetSliderCanPush(v);
    }

  if (this->YRange)
    {
    this->YRange->SetSliderCanPush(v);
    }

  if (this->ZRange)
    {
    this->ZRange->SetSliderCanPush(v);
    }
}

// ---------------------------------------------------------------------------
void vtkKWExtent::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->XRange)
    {
    this->XRange->SetEnabled(this->Enabled);
    }

  if (this->YRange)
    {
    this->YRange->SetEnabled(this->Enabled);
    }

  if (this->ZRange)
    {
    this->ZRange->SetEnabled(this->Enabled);
    }
}

//----------------------------------------------------------------------------
void vtkKWExtent::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Extent: " << this->GetExtent() << endl;
}
