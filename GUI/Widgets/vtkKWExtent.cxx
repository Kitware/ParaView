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
vtkCxxRevisionMacro(vtkKWExtent, "1.40");

//----------------------------------------------------------------------------
vtkKWExtent::vtkKWExtent()
{
  this->Command = NULL;

  for (int i = 0; i < 3; i++)
    {
    this->Range[i] = vtkKWRange::New();
    this->ShowExtent[i] = 1;
    this->Extent[i * 2] = VTK_DOUBLE_MAX;
    this->Extent[i * 2 + 1] = VTK_DOUBLE_MIN;
    }
}

//----------------------------------------------------------------------------
vtkKWExtent::~vtkKWExtent()
{
  if (this->Command)
    {
    delete [] this->Command;
    }

  for (int i = 0; i < 3; i++)
    {
    this->Range[i]->Delete();
    this->Range[i] = NULL;
    }

}

//----------------------------------------------------------------------------
void vtkKWExtent::Create(vtkKWApplication *app)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app);

  for (int i = 0; i < 3; i++)
    {
    this->Range[i]->SetParent(this);
    this->Range[i]->ShowLabelOn();
    this->Range[i]->ShowEntriesOn();
    this->Range[i]->Create(app);
    this->Range[i]->SetCommand(this, "ExtentChangedCallback");
    this->Range[i]->AdjustResolutionOn();
    }

  this->Range[0]->GetLabel()->SetText("X (Units)");
  this->Range[1]->GetLabel()->SetText("Y (Units)");
  this->Range[2]->GetLabel()->SetText("Z (Units)");
  
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

  this->Range[0]->UnpackSiblings();

  int is_horiz = 
    (this->Range[0]->GetOrientation() == vtkKWRange::OrientationHorizontal);

  // Repack everything

  ostrstream tk_cmd;

  for (int i = 0; i < 3; i++)
    {
    if (this->ShowExtent[i])
      {
      tk_cmd << "pack "
             << this->Range[i]->GetWidgetName() << " "
             << "-padx 2 -pady 2 -fill both -expand yes -anchor w "
             << "-side " << (is_horiz ? "top" : "left") << endl;
      }
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

  this->Range[0]->SetResolution((x2<x1) ? ((x1-x2) / res) : ((x2-x1) / res));
  this->Range[1]->SetResolution((y2<y1) ? ((y1-y2) / res) : ((y2-y1) / res));
  this->Range[2]->SetResolution((z2<z1) ? ((z1-z2) / res) : ((z2-z1) / res));
  
  this->Range[0]->SetWholeRange(x1, x2);
  this->Range[1]->SetWholeRange(y1, y2);
  this->Range[2]->SetWholeRange(z1, z2);

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
void vtkKWExtent::GetExtentRange(double extent_range[6])
{
  this->GetExtentRange(extent_range[0], extent_range[1],
                       extent_range[2], extent_range[3],
                       extent_range[4], extent_range[5]);
}

//----------------------------------------------------------------------------
void vtkKWExtent::GetExtentRange(double &x1, double &x2, 
                                 double &y1, double &y2, 
                                 double &z1, double &z2)
{
  if (this->Range[0])
    {
    this->Range[0]->GetWholeRange(x1, x2);
    }
  if (this->Range[1])
    {
    this->Range[1]->GetWholeRange(y1, y2);
    }
  if (this->Range[2])
    {
    this->Range[2]->GetWholeRange(z1, z2);
    }
}

//----------------------------------------------------------------------------
double* vtkKWExtent::GetExtentRange()
{
  static double extent_range[6];
  this->GetExtentRange(extent_range);
  return extent_range;
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

  this->Range[0]->SetRange(x1, x2);
  this->Range[1]->SetRange(y1, y2);
  this->Range[2]->SetRange(z1, z2);
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetExtent(double er[6])
{
  this->SetExtent(er[0], er[1], er[2], er[3], er[4], er[5]);
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetShowExtent(int index, int arg)
{
  if (index < 0 || index > 2 || this->ShowExtent[index] == arg)
    {
    return;
    }

  this->ShowExtent[index] = arg;
  this->Pack();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWExtent::ExtentChangedCallback()
{
  // first check to see if anything changed.
  // Normally something should have changed, but 
  // on initialization this isn;t the case.

  if (this->Extent[0] == this->Range[0]->GetRange()[0] &&
      this->Extent[1] == this->Range[0]->GetRange()[1] &&
      this->Extent[2] == this->Range[1]->GetRange()[0] &&
      this->Extent[3] == this->Range[1]->GetRange()[1] &&
      this->Extent[4] == this->Range[2]->GetRange()[0] &&
      this->Extent[5] == this->Range[2]->GetRange()[1])
    {
    return;
    }
  
  this->Extent[0] = this->Range[0]->GetRange()[0];
  this->Extent[1] = this->Range[0]->GetRange()[1];
  this->Extent[2] = this->Range[1]->GetRange()[0];
  this->Extent[3] = this->Range[1]->GetRange()[1];
  this->Extent[4] = this->Range[2]->GetRange()[0];
  this->Extent[5] = this->Range[2]->GetRange()[1];
 
  if ( this->Command )
    {
    this->Script("eval %s",this->Command);
    }
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetCommand(vtkObject *obj, const char *method)
{ 
  this->SetObjectMethodCommand(&this->Command, obj, method);
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetStartCommand(vtkObject* obj, 
                                  const char *method)
{ 
  for (int i = 0; i < 3; i++)
    {
    if (this->Range[i])
      {
      this->Range[i]->SetStartCommand(obj, method);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetEndCommand(vtkObject* obj, 
                                const char *method)
{ 
  for (int i = 0; i < 3; i++)
    {
    if (this->Range[i])
      {
      this->Range[i]->SetEndCommand(obj, method);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetDisableCommands(int v)
{ 
  for (int i = 0; i < 3; i++)
    {
    if (this->Range[i])
      {
      this->Range[i]->SetDisableCommands(v);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetOrientation(int v)
{ 
  for (int i = 0; i < 3; i++)
    {
    if (this->Range[i])
      {
      this->Range[i]->SetOrientation(v);
      }
    }

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetLabelPosition(int v)
{ 
  for (int i = 0; i < 3; i++)
    {
    if (this->Range[i])
      {
      this->Range[i]->SetLabelPosition(v);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetEntry1Position(int v)
{ 
  for (int i = 0; i < 3; i++)
    {
    if (this->Range[i])
      {
      this->Range[i]->SetEntry1Position(v);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetEntry2Position(int v)
{ 
  for (int i = 0; i < 3; i++)
    {
    if (this->Range[i])
      {
      this->Range[i]->SetEntry2Position(v);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetSliderCanPush(int v)
{ 
  for (int i = 0; i < 3; i++)
    {
    if (this->Range[i])
      {
      this->Range[i]->SetSliderCanPush(v);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetThickness(int v)
{ 
  for (int i = 0; i < 3; i++)
    {
    if (this->Range[i])
      {
      this->Range[i]->SetThickness(v);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetInternalThickness(double v)
{ 
  for (int i = 0; i < 3; i++)
    {
    if (this->Range[i])
      {
      this->Range[i]->SetInternalThickness(v);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetRequestedLength(int v)
{ 
  for (int i = 0; i < 3; i++)
    {
    if (this->Range[i])
      {
      this->Range[i]->SetRequestedLength(v);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWExtent::SetSliderSize(int v)
{ 
  for (int i = 0; i < 3; i++)
    {
    if (this->Range[i])
      {
      this->Range[i]->SetSliderSize(v);
      }
    }
}

//----------------------------------------------------------------------------
vtkKWRange* vtkKWExtent::GetRange(int index)
{ 
  if (index < 0 || index > 2)
    {
    return NULL;
    }
  return this->Range[index];
}

// ---------------------------------------------------------------------------
void vtkKWExtent::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  for (int i = 0; i < 3; i++)
    {
    this->PropagateEnableState(this->Range[i]);
    }
}

//----------------------------------------------------------------------------
void vtkKWExtent::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Extent: " << this->GetExtent() << endl;
  for (int i = 0; i < 3; i++)
    {
    os << indent << "ShowExtent[" << i << "]: " 
       << (this->ShowExtent[i] ? "On" : "Off") << endl;
    }
}

