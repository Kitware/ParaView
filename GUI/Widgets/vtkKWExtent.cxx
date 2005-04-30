/*=========================================================================

  Module:    vtkKWExtent.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWExtent.h"

#include "vtkKWApplication.h"
#include "vtkKWRange.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWExtent );
vtkCxxRevisionMacro(vtkKWExtent, "1.34");

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

  this->ShowXExtent = 1;
  this->ShowYExtent = 1;
  this->ShowZExtent = 1;
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
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(app, "frame", "-bd 0"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->ConfigureOptions(args);

  this->XRange->SetParent(this);
  this->XRange->ShowLabelOn();
  this->XRange->ShowEntriesOn();
  this->XRange->Create(app, "");
  this->XRange->SetCommand(this, "ExtentChangedCallback");
  this->XRange->AdjustResolutionOn();
  this->XRange->GetLabel()->SetText("X (Units)");
  
  this->YRange->SetParent(this);
  this->YRange->ShowLabelOn();
  this->YRange->ShowEntriesOn();
  this->YRange->Create(app, "");
  this->YRange->AdjustResolutionOn();
  this->YRange->SetCommand(this, "ExtentChangedCallback");
  this->YRange->GetLabel()->SetText("Y (Units)");

  this->ZRange->SetParent(this);
  this->ZRange->ShowLabelOn();
  this->ZRange->ShowEntriesOn();
  this->ZRange->Create(app, "");
  this->ZRange->AdjustResolutionOn();
  this->ZRange->SetCommand(this, "ExtentChangedCallback");
  this->ZRange->GetLabel()->SetText("Z (Units)");
  
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

  if (this->ShowXExtent)
    {
    tk_cmd << "pack "
           << this->XRange->GetWidgetName() << " "
           << "-padx 2 -pady 2 -fill both -expand yes -anchor w "
           << "-side " << (is_horiz ? "top" : "left") << endl;
    }
  
  if (this->ShowYExtent)
    {
    tk_cmd << "pack "
           << this->YRange->GetWidgetName() << " "
           << "-padx 2 -pady 2 -fill both -expand yes -anchor w "
           << "-side " << (is_horiz ? "top" : "left") << endl;
    }
  
  if (this->ShowZExtent)
    {
    tk_cmd << "pack "
           << this->ZRange->GetWidgetName() << " "
           << "-padx 2 -pady 2 -fill both -expand yes -anchor w "
           << "-side " << (is_horiz ? "top" : "left") << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetExtentRange(double er[6])
{
  this->SetExtentRange(er[0], er[1], er[2], er[3], er[4], er[5]);
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetExtentRange(double x1, double x2, 
                                 double y1, double y2, 
                                 double z1, double z2)
{
  double res = 512.0;

  this->XRange->SetResolution((x2<x1) ? ((x1-x2) / res) : ((x2-x1) / res));
  this->YRange->SetResolution((y2<y1) ? ((y1-y2) / res) : ((y2-y1) /res));
  this->ZRange->SetResolution((y2<y1) ? ((y1-y2) / res) : ((y2-y1) /res));
  
  this->XRange->SetWholeRange(x1, x2);
  this->YRange->SetWholeRange(y1, y2);
  this->ZRange->SetWholeRange(z1, z2);

  double ex1, ex2, ey1, ey2, ez1, ez2;

  ex1 = (this->Extent[0] < x1 || this->Extent[0] > x2) ? x1 : this->Extent[0];
  ex2 = (this->Extent[1] < x1 || this->Extent[1] > x2) ? x2 : this->Extent[1];
  ey1 = (this->Extent[2] < y1 || this->Extent[2] > y2) ? y1 : this->Extent[2];
  ey2 = (this->Extent[3] < y1 || this->Extent[3] > y2) ? y2 : this->Extent[3];
  ez1 = (this->Extent[4] < z1 || this->Extent[4] > z2) ? z1 : this->Extent[4];
  ez2 = (this->Extent[5] < z1 || this->Extent[5] > z2) ? z2 : this->Extent[5];

  this->SetExtent(ex1, ex2, ey1, ey2, ez1, ez2);
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetExtent(double x1, double x2, 
                            double y1, double y2, 
                            double z1, double z2)
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

  this->Extent[0] = x1;
  this->Extent[1] = x2;
  this->Extent[2] = y1;
  this->Extent[3] = y2;
  this->Extent[4] = z1;
  this->Extent[5] = z2;

  this->XRange->SetRange(x1, x2);
  this->YRange->SetRange(y1, y2);
  this->ZRange->SetRange(z1, z2);
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetExtent(double er[6])
{
  this->SetExtent(er[0], er[1], er[2], er[3], er[4], er[5]);
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetShowXExtent(int arg)
{
  if (this->ShowXExtent == arg)
    {
    return;
    }

  this->ShowXExtent = arg;
  this->Pack();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetShowYExtent(int arg)
{
  if (this->ShowYExtent == arg)
    {
    return;
    }

  this->ShowYExtent = arg;
  this->Pack();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetShowZExtent(int arg)
{
  if (this->ShowZExtent == arg)
    {
    return;
    }

  this->ShowZExtent = arg;
  this->Pack();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWExtent::ExtentChangedCallback()
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
  this->SetObjectMethodCommand(&this->Command, CalledObject, CommandString);
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
void vtkKWExtent::SetEntry1Position(int v)
{ 
  if (this->XRange)
    {
    this->XRange->SetEntry1Position(v);
    }

  if (this->YRange)
    {
    this->YRange->SetEntry1Position(v);
    }

  if (this->ZRange)
    {
    this->ZRange->SetEntry1Position(v);
    }
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetEntry2Position(int v)
{ 
  if (this->XRange)
    {
    this->XRange->SetEntry2Position(v);
    }

  if (this->YRange)
    {
    this->YRange->SetEntry2Position(v);
    }

  if (this->ZRange)
    {
    this->ZRange->SetEntry2Position(v);
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

  this->PropagateEnableState(this->XRange);
  this->PropagateEnableState(this->YRange);
  this->PropagateEnableState(this->ZRange);
}

//----------------------------------------------------------------------------
void vtkKWExtent::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Extent: " << this->GetExtent() << endl;
  os << indent << "ShowXExtent: " 
     << (this->ShowXExtent ? "On" : "Off") << endl;
  os << indent << "ShowYExtent: " 
     << (this->ShowYExtent ? "On" : "Off") << endl;
  os << indent << "ShowZExtent: " 
     << (this->ShowZExtent ? "On" : "Off") << endl;
}

