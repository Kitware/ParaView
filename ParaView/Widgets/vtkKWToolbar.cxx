/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWToolbar.cxx
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
#include "vtkKWApplication.h"
#include "vtkKWToolbar.h"
#include "vtkObjectFactory.h"
#include "vtkVector.txx"
#include "vtkVectorIterator.txx"

#ifndef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION

template class VTK_EXPORT vtkAbstractList<vtkKWWidget*>;
template class VTK_EXPORT vtkVector<vtkKWWidget*>;
template class VTK_EXPORT vtkAbstractIterator<vtkIdType,vtkKWWidget*>;
template class VTK_EXPORT vtkVectorIterator<vtkKWWidget*>;

#endif

static int vtkKWToolbarGlobalFlatAspect = 0;
static int vtkKWToolbarGlobalWidgetsFlatAspect = 0;

int vtkKWToolbar::GetGlobalFlatAspect() 
{ 
  return vtkKWToolbarGlobalFlatAspect; 
}
void vtkKWToolbar::SetGlobalFlatAspect(int val) 
{ 
  vtkKWToolbarGlobalFlatAspect = val; 
};

int vtkKWToolbar::GetGlobalWidgetsFlatAspect() 
{ 
  return vtkKWToolbarGlobalWidgetsFlatAspect; 
}
void vtkKWToolbar::SetGlobalWidgetsFlatAspect(int val) 
{ 
  vtkKWToolbarGlobalWidgetsFlatAspect = val; 
};

//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWToolbar );
vtkCxxRevisionMacro(vtkKWToolbar, "1.20");


int vtkKWToolbarCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWToolbar::vtkKWToolbar()
{
  this->CommandFunction = vtkKWToolbarCommand;
  this->Expanding = 0;

  this->Frame = vtkKWWidget::New();
  this->Frame->SetParent(this);

  this->Separator = vtkKWWidget::New();
  this->Separator->SetParent(this);

  this->Widgets = vtkVector<vtkKWWidget*>::New();

  this->FlatAspect = vtkKWToolbar::GetGlobalFlatAspect();
  this->WidgetsFlatAspect = vtkKWToolbar::GetGlobalWidgetsFlatAspect();

  this->Resizable = 0;
}

//----------------------------------------------------------------------------
vtkKWToolbar::~vtkKWToolbar()
{
  this->Frame->Delete();
  this->Frame = 0;

  this->Separator->Delete();
  this->Separator = 0;

  this->Widgets->Delete();
}

//----------------------------------------------------------------------------
void vtkKWToolbar::AddWidget(vtkKWWidget *widget)
{
  if (this->Widgets->AppendItem(widget) == VTK_OK)
    {
    this->UpdateWidgets();
    }
  else
    {
    vtkErrorMacro("Unable to add widget to toolbar");
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbar::InsertWidget(vtkKWWidget *location, vtkKWWidget *widget)
{
  int res;
  if (!location)
    {
    res = this->Widgets->PrependItem(widget);
    }

  vtkIdType loc;
  if (this->Widgets->FindItem(location, loc) == VTK_OK)
    {
    res = this->Widgets->InsertItem(loc, widget);
    }
  else
    {
    res = this->Widgets->PrependItem(widget);
    }

  if (res)
    {
    this->UpdateWidgets();
    }
  else
    {
    vtkErrorMacro("Unable to insert widget in toolbar");
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbar::RemoveWidget(vtkKWWidget *widget)
{
  vtkIdType loc;
  if (this->Widgets->FindItem(widget, loc) == VTK_OK)
    {
    if (this->Widgets->RemoveItem(loc) == VTK_OK)
      {
      this->UpdateWidgets();
      return;
      }
    }
  vtkErrorMacro("Unable to remove widget from toolbar");
}

//----------------------------------------------------------------------------
void vtkKWToolbar::Create(vtkKWApplication *app)
{
  // Must set the application

  if (this->Application)
    {
    vtkErrorMacro("widget already created");
    return;
    }
  this->SetApplication(app);

  // Note that there are two frames here. It used to be mandatory in the past. 
  // Let's keep it like this in case we want to add something in the toolbar
  // like a "grip" on the left for example, to detach/drag the toolbar.

  // Create the main frame for this widget

  this->Script("frame %s", this->GetWidgetName());

  this->Script("bind %s <Configure> {%s ScheduleResize}",
               this->GetWidgetName(), this->GetTclName());

  // Create the widgets container itself

  this->Frame->Create(app, "frame", "-bd 0");

  // Create a "toolbar separator" for the flat aspect

  this->Separator->Create(app, "frame", "-bd 2 -relief raised");

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWToolbar::ScheduleResize()
{  
  if (this->Expanding)
    {
    return;
    }
  this->Expanding = 1;
  this->Script("after idle {%s Resize}", this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkKWToolbar::Resize()
{
  this->UpdateWidgets();
  this->Expanding = 0;
}

//----------------------------------------------------------------------------
void vtkKWToolbar::UpdateWidgetsAspect()
{
  if (this->Widgets->GetNumberOfItems() <= 0)
    {
    return;
    }

  vtkVectorIterator<vtkKWWidget*>* it = this->Widgets->NewIterator();
  ostrstream s;

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    vtkKWWidget* widget = 0;
    // Change the relief of buttons (let's say that everything that
    // has a -command will qualify, -state could have been used, or
    // a match on the widget type, etc.
    if (it->GetData(widget) == VTK_OK && 
        widget->HasConfigurationOption("-command"))
      {
      int use_relief = widget->HasConfigurationOption("-relief");
      if (widget->HasConfigurationOption("-indicatoron"))
        {
        this->Script("%s cget -indicatoron", widget->GetWidgetName());
        use_relief = this->GetIntegerResult(this->Application);
        }
        
      if (use_relief)
        {
        s << widget->GetWidgetName() << " config -relief " 
          << (this->WidgetsFlatAspect ? "flat" : "raised") << endl;
        }
      else
        {
        // Can not use -relief, try to hack -bd by specifying
        // an empty border using the negative current value
        if (widget->HasConfigurationOption("-bd"))
          {
          this->Script("%s cget -bd", widget->GetWidgetName());
          int bd = this->GetIntegerResult(this->Application);
          s << widget->GetWidgetName() << " config -bd "
            << (this->WidgetsFlatAspect ? -abs(bd) : abs(bd)) << endl;
          }
        }
      }
    it->GoToNextItem();
    }
  it->Delete();

  s << ends;
  this->Script(s.str());
  s.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWToolbar::ConstrainWidgetsLayout()
{
  if (this->Widgets->GetNumberOfItems() <= 0)
    {
    return;
    }

  int totReqWidth = 0;

  vtkVectorIterator<vtkKWWidget*>* it = this->Widgets->NewIterator();
    
  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    vtkKWWidget* widget = 0;
    if (it->GetData(widget) == VTK_OK)
      {
      this->Script("winfo reqwidth %s", widget->GetWidgetName());
      totReqWidth += this->GetIntegerResult(this->Application);
      }
    it->GoToNextItem();
    }

  this->Script("winfo width %s", this->GetWidgetName());
  int width = this->GetIntegerResult(this->Application);

  int widthWidget = totReqWidth / this->Widgets->GetNumberOfItems();
  int numPerRow = width / widthWidget;

  if ( numPerRow > 0 )
    {
    int row = 0, num = 0;
    ostrstream s;

    it->InitTraversal();
    while (!it->IsDoneWithTraversal())
      {
      vtkKWWidget* widget = 0;
      if (it->GetData(widget) == VTK_OK)
        {
        s << "grid " << widget->GetWidgetName() << " -row " 
          << row << " -column " << num << " -sticky news " << endl;
        num++;
        if ( num == numPerRow ) 
          { 
          row++; 
          num=0;
          }
        }
      it->GoToNextItem();
      }
    s << ends;
    this->Script(s.str());
    s.rdbuf()->freeze(0);
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkKWToolbar::UpdateWidgetsLayout()
{
  if (this->Widgets->GetNumberOfItems() <= 0)
    {
    return;
    }

  this->Script("catch {eval grid forget [grid slaves %s]}",
               this->GetFrame()->GetWidgetName());

  // If this toolbar is resizable, then constrain it to the current size

  if (this->Resizable)
    {
    this->ConstrainWidgetsLayout();
    return;
    }

  ostrstream s;
  s << "grid "; 

  vtkVectorIterator<vtkKWWidget*>* it = this->Widgets->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    vtkKWWidget* widget = 0;
    if (it->GetData(widget) == VTK_OK)
      {
      s << " " << widget->GetWidgetName();
      }
    it->GoToNextItem();
    }
  it->Delete();

  s << " -sticky news -row 0" << ends;
  this->Script(s.str());
  s.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWToolbar::UpdateWidgets()
{
  this->UpdateWidgetsAspect();
  this->UpdateWidgetsLayout();
}

//----------------------------------------------------------------------------
void vtkKWToolbar::Update()
{
  if (this->FlatAspect)
    {
    this->Script(
      "%s config -relief flat -bd 0", 
      this->GetWidgetName());

    this->Script(
      "pack %s -ipadx 2 -ipady 0 -padx 0 -pady 3 -side left -anchor nw -fill both -expand n",
      this->Frame->GetWidgetName());

    this->Script(
      "pack %s -ipadx 1 -side left -anchor nw -fill y -expand n -before %s",
      this->Separator->GetWidgetName(),
      this->Frame->GetWidgetName());
    }
  else
    {
    this->Script(
      "%s config -relief raised -bd 1", 
      this->GetWidgetName());

    this->Script(
      "pack forget %s", 
      this->Separator->GetWidgetName());

    this->Script(
      "pack %s -ipadx 2 -ipady 2 -padx 1 -pady 0 -side left -anchor nw -fill both -expand n",
      this->Frame->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbar::Pack(const char *options)
{
  this->Script("pack %s -padx 2 -pady 2 -anchor n %s", 
               this->GetWidgetName(), options);

  if (this->Resizable)
    {
    this->Script("pack %s -fill both -expand y", 
                 this->GetWidgetName());
    }
  else
    {
    this->Script("pack %s -fill both -expand n", 
                 this->GetWidgetName());
    }
}

//-----------------------------------------------------------------------------
void vtkKWToolbar::SetFlatAspect(int f)
{
  if (this->FlatAspect == f)
    {
    return;
    }
  this->FlatAspect = f;
  this->Modified();
  this->Update();
  this->UpdateWidgets();
  if (this->IsPacked())
    {
    this->Pack();
    }
}

//-----------------------------------------------------------------------------
void vtkKWToolbar::SetWidgetsFlatAspect(int f)
{
  if (this->WidgetsFlatAspect == f)
    {
    return;
    }
  this->WidgetsFlatAspect = f;
  this->Modified();
  this->UpdateWidgets();
}

//-----------------------------------------------------------------------------
void vtkKWToolbar::SetResizable(int r)
{
  if (this->Resizable == r)
    {
    return;
    }
  this->Resizable = r;
  this->Modified();
  this->Update();
  this->UpdateWidgets();
  if (this->IsPacked())
    {
    this->Pack();
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbar::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Frame: " << this->Frame << endl;
  os << indent << "Separator: " << this->Separator << endl;
  os << indent << "Resizable: " << (this->Resizable ? "On" : "Off") << endl;
  os << indent << "FlatAspect: " << (this->FlatAspect ? "On" : "Off") << endl;
  os << indent << "WidgetsFlatAspect: " << (this->WidgetsFlatAspect ? "On" : "Off") << endl;
}
