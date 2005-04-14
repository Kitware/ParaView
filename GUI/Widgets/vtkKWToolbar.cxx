/*=========================================================================

  Module:    vtkKWToolbar.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWToolbar.h"

#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWRadioButton.h"
#include "vtkObjectFactory.h"

#include <kwsys/stl/list>
#include <kwsys/stl/algorithm>

#ifdef _WIN32
static int vtkKWToolbarGlobalFlatAspect        = 1;
static int vtkKWToolbarGlobalWidgetsFlatAspect = 1;
#else
static int vtkKWToolbarGlobalFlatAspect        = 0;
static int vtkKWToolbarGlobalWidgetsFlatAspect = 0;
#endif

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

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWToolbar );
vtkCxxRevisionMacro(vtkKWToolbar, "1.44");

int vtkKWToolbarCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);

//----------------------------------------------------------------------------
class vtkKWToolbarInternals
{
public:

  typedef kwsys_stl::list<vtkKWWidget*> WidgetsContainer;
  typedef kwsys_stl::list<vtkKWWidget*>::iterator WidgetsContainerIterator;

  WidgetsContainer Widgets;
};

//----------------------------------------------------------------------------
vtkKWToolbar::vtkKWToolbar()
{
  this->CommandFunction = vtkKWToolbarCommand;

  this->Frame   = vtkKWFrame::New();
  this->Handle  = vtkKWFrame::New();

  this->FlatAspect                = vtkKWToolbar::GetGlobalFlatAspect();
  this->WidgetsFlatAspect         = vtkKWToolbar::GetGlobalWidgetsFlatAspect();
  this->Resizable                 = 0;
  this->Expanding                 = 0;

  this->WidgetsFlatAdditionalPadX = 1;
  this->WidgetsFlatAdditionalPadY = 0;

#if defined(WIN32)
  this->PadX = 0;
  this->PadY = 0;
#else
  this->PadX = 1;
  this->PadY = 1;
#endif

  // This widget is used to keep track of default options

  this->DefaultOptionsWidget = vtkKWRadioButton::New();

  // Internal structs

  this->Internals = new vtkKWToolbarInternals;
}

//----------------------------------------------------------------------------
vtkKWToolbar::~vtkKWToolbar()
{
  if (this->Frame)
    {
    this->Frame->Delete();
    this->Frame = 0;
    }

  if (this->Handle)
    {
    this->Handle->Delete();
    this->Handle = 0;
    }

  if (this->DefaultOptionsWidget)
    {
    this->DefaultOptionsWidget->Delete();
    this->DefaultOptionsWidget = NULL;
    }

  if (this->Internals)
    {
    vtkKWToolbarInternals::WidgetsContainerIterator it = 
      this->Internals->Widgets.begin();
    vtkKWToolbarInternals::WidgetsContainerIterator end = 
      this->Internals->Widgets.end();
    for (; it != end; ++it)
      {
      if (*it)
        {
        (*it)->Delete();
        }
      }
    delete this->Internals;
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbar::Bind()
{
  if (this->IsCreated())
    {
    this->Script("bind %s <Configure> {%s ScheduleResize}",
               this->GetWidgetName(), this->GetTclName());
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbar::UnBind()
{
  if (this->IsCreated())
    {
    this->Script("bind %s <Configure> {}", this->GetWidgetName());
    }
}
//----------------------------------------------------------------------------
void vtkKWToolbar::Create(vtkKWApplication *app)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(app, "frame", NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }
  
  this->Bind();

  // Create the widgets container itself

  this->Frame->SetParent(this);
  this->Frame->Create(app, "");
  
  // Create a "toolbar handle"

  this->Handle->SetParent(this);
  this->Handle->Create(app, "-bd 2 -relief raised");

  // Create the default options repository (never packed, just a way
  // to keep track of default options)

  this->DefaultOptionsWidget->SetParent(this);
  this->DefaultOptionsWidget->Create(app, "");

  // Update aspect

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWToolbar::AddWidget(vtkKWWidget *widget)
{
  if (!widget || !this->Internals)
    {
    return;
    }
    
  this->Internals->Widgets.push_back(widget);

  widget->Register(this);
  widget->SetEnabled(this->Enabled);

  this->UpdateWidgets();
}

//----------------------------------------------------------------------------
void vtkKWToolbar::InsertWidget(vtkKWWidget *location, vtkKWWidget *widget)
{
  if (!widget || !this->Internals)
    {
    return;
    }

  if (!location)
    {
    this->Internals->Widgets.push_front(widget);
    }
  else
    {
    vtkKWToolbarInternals::WidgetsContainerIterator location_pos = 
      kwsys_stl::find(this->Internals->Widgets.begin(),
                   this->Internals->Widgets.end(),
                   location);
    if (location_pos == this->Internals->Widgets.end())
      {
      this->Internals->Widgets.push_front(widget);
      }
    else
      {
      this->Internals->Widgets.insert(location_pos, widget);
      }
    }

  widget->Register(this);
  widget->SetEnabled(this->Enabled);

  this->UpdateWidgets();
}

//----------------------------------------------------------------------------
void vtkKWToolbar::RemoveWidget(vtkKWWidget *widget)
{
  if (!widget || !this->Internals)
    {
    return;
    }

  vtkKWToolbarInternals::WidgetsContainerIterator location_pos = 
    kwsys_stl::find(this->Internals->Widgets.begin(),
                 this->Internals->Widgets.end(),
                 widget);
  if (location_pos == this->Internals->Widgets.end())
    {
    vtkErrorMacro("Unable to remove widget from toolbar");
    }
  else
    {
    (*location_pos)->Delete();
    this->Internals->Widgets.erase(location_pos);
    this->UpdateWidgets();
    }
}

//----------------------------------------------------------------------------
vtkKWWidget* vtkKWToolbar::GetWidget(const char *name)
{
  if (name && this->Internals)
    {
    const char *options[4] = { "-label", "-text", "-image", "-selectimage" };

    vtkKWToolbarInternals::WidgetsContainerIterator it = 
      this->Internals->Widgets.begin();
    vtkKWToolbarInternals::WidgetsContainerIterator end = 
      this->Internals->Widgets.end();
    for (; it != end; ++it)
      {
      if (*it)
        {
        for (int i = 0; i < 4; i++)
          {
          if ((*it)->HasConfigurationOption(options[i]))
            {
            const char *option = 
              this->Script("%s cget %s", (*it)->GetWidgetName(), options[i]);
            if (!strcmp(name, option))
              {
              return (*it);
              }
            }
          }
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
vtkKWWidget* vtkKWToolbar::AddRadioButtonImage(int value, 
                                               const char *image_name, 
                                               const char *select_image_name, 
                                               const char *variable_name, 
                                               vtkKWObject *object, 
                                               const char *method,
                                               const char *help,
                                               const char *extra)
{
  if (!this->IsCreated())
    {
    return 0;
    }

  vtkKWRadioButton *rb = vtkKWRadioButton::New();

  rb->SetParent(this->GetFrame());
  rb->Create(this->GetApplication(), NULL);
  rb->SetIndicator(0);
  rb->SetValue(value);

  if (image_name)
    {
    this->Script(
      "%s configure -highlightthickness 0 -image %s -selectimage %s", 
      rb->GetWidgetName(), 
      image_name, (select_image_name ? select_image_name : image_name));
    }

  if (object && method)
    {
    rb->SetCommand(object, method);
    }

  if (variable_name)
    {
    rb->SetVariableName(variable_name);
    }

  if (help)
    {
    rb->SetBalloonHelpString(help);
    }

  this->ConfigureOptions(extra);

  this->AddWidget(rb);

  rb->Delete();

  return rb;
}

//----------------------------------------------------------------------------
vtkKWWidget* vtkKWToolbar::AddCheckButtonImage(const char *image_name, 
                                               const char *select_image_name, 
                                               const char *variable_name, 
                                               vtkKWObject *object, 
                                               const char *method,
                                               const char *help,
                                               const char *extra)
{
  if (!this->IsCreated())
    {
    return 0;
    }

  vtkKWCheckButton *cb = vtkKWCheckButton::New();

  cb->SetParent(this->GetFrame());
  cb->Create(this->GetApplication(), NULL);
  cb->SetIndicator(0);

  if (image_name)
    {
    this->Script(
      "%s configure -highlightthickness 0 -image %s -selectimage %s", 
      cb->GetWidgetName(), 
      image_name, (select_image_name ? select_image_name : image_name));
    }

  if (object && method)
    {
    cb->SetCommand(object, method);
    }

  if (variable_name)
    {
    cb->SetVariableName(variable_name);
    }

  if (help)
    {
    cb->SetBalloonHelpString(help);
    }

  this->ConfigureOptions(extra);

  this->AddWidget(cb);

  cb->Delete();

  return cb;
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
  if (!this->Internals || this->Internals->Widgets.size() <= 0)
    {
    return;
    }

  ostrstream s;

  vtkKWToolbarInternals::WidgetsContainerIterator it = 
    this->Internals->Widgets.begin();
  vtkKWToolbarInternals::WidgetsContainerIterator end = 
    this->Internals->Widgets.end();
  for (; it != end; ++it)
    {
    // Change the relief of buttons (let's say that everything that
    // has a -command will qualify, -state could have been used, or
    // a match on the widget type, etc).

    if ((*it) && (*it)->HasConfigurationOption("-command"))
      {
      int use_relief = (*it)->HasConfigurationOption("-relief");
      if ((*it)->HasConfigurationOption("-indicatoron"))
        {
        use_relief = atoi(
          this->Script("%s cget -indicatoron", (*it)->GetWidgetName()));
        }
        
      if (use_relief)
        {
        s << (*it)->GetWidgetName() << " config -relief " 
          << (this->WidgetsFlatAspect ? "flat" : "raised") << endl;
        }
      else
        {
        // Can not use -relief, try to hack -bd by specifying
        // an empty border as the negative current value (i.e.
        // the negative value will be handled as 0, but still will enable
        // us to retrieve the old value using abs() later on).

        if ((*it)->HasConfigurationOption("-bd"))
          {
          int bd = atoi(
            this->Script("%s cget -bd", (*it)->GetWidgetName()));
          s << (*it)->GetWidgetName() << " config -bd "
            << (this->WidgetsFlatAspect ? -abs(bd) : abs(bd)) << endl;
          }
        }

      // If radiobutton, remove the select color border in flat aspect

      if ((*it)->HasConfigurationOption("-selectcolor"))
        {
        if (this->WidgetsFlatAspect)
          {
          s << (*it)->GetWidgetName() << " config -selectcolor [" 
            << (*it)->GetWidgetName() << " cget -bg]" << endl; 
          }
        else
          {
          s << (*it)->GetWidgetName() << " config -selectcolor [" 
            << this->DefaultOptionsWidget->GetWidgetName() 
            << " cget -selectcolor]" << endl; 
          }
        }

      // Do not use active background in flat mode either

      if ((*it)->HasConfigurationOption("-activebackground"))
        {
        if (this->WidgetsFlatAspect)
          {
          s << (*it)->GetWidgetName() << " config -activebackground [" 
            << (*it)->GetWidgetName() << " cget -bg]" << endl; 
          }
        else
          {
          s << (*it)->GetWidgetName() << " config -activebackground [" 
            << this->DefaultOptionsWidget->GetWidgetName() 
            << " cget -activebackground]" << endl; 
          }
        }
      }
    }

  s << ends;
  this->Script(s.str());
  s.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWToolbar::ConstrainWidgetsLayout()
{
  if (!this->Internals || this->Internals->Widgets.size() <= 0)
    {
    return;
    }

  int totReqWidth = 0;

  vtkKWToolbarInternals::WidgetsContainerIterator it = 
    this->Internals->Widgets.begin();
  vtkKWToolbarInternals::WidgetsContainerIterator end = 
    this->Internals->Widgets.end();
  for (; it != end; ++it)
    {
    if (*it)
      {
      totReqWidth += this->PadX + atoi(
        this->Script("winfo reqwidth %s", (*it)->GetWidgetName()));
      if (this->WidgetsFlatAspect)
        {
        totReqWidth += this->WidgetsFlatAdditionalPadX;
        }
      }
    }

  int width = atoi(
    this->Script("winfo width %s", this->GetWidgetName()));

  int widthWidget = totReqWidth / this->Internals->Widgets.size();
  int numPerRow = width / widthWidget;

  if ( numPerRow > 0 )
    {
    int row = 0, num = 0;
    ostrstream s;

    it = this->Internals->Widgets.begin();
    for (; it != end; ++it)
      {
      if ((*it))
        {
        s << "grid " << (*it)->GetWidgetName() << " -row " 
          << row << " -column " << num << " -sticky news "
          << " -ipadx " 
          << (this->PadX + (this->WidgetsFlatAspect ? 
                            this->WidgetsFlatAdditionalPadX : 0))
          << " -ipady "
          << (this->PadY + (this->WidgetsFlatAspect ? 
                            this->WidgetsFlatAdditionalPadY : 0))
          << endl;
        num++;
        if ( num == numPerRow ) 
          { 
          row++; 
          num=0;
          }
        }
      }
    s << ends;
    this->Script(s.str());
    s.rdbuf()->freeze(0);
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbar::UpdateWidgetsLayout()
{
  if (!this->IsCreated() || 
      !this->Internals || this->Internals->Widgets.size() <= 0)
    {
    return;
    }

  this->GetFrame()->UnpackChildren();

  // If this toolbar is resizable, then constrain it to the current size

  if (this->Resizable)
    {
    this->ConstrainWidgetsLayout();
    return;
    }

  ostrstream s;
  s << "grid "; 

  vtkKWToolbarInternals::WidgetsContainerIterator it = 
    this->Internals->Widgets.begin();
  vtkKWToolbarInternals::WidgetsContainerIterator end = 
    this->Internals->Widgets.end();
  for (; it != end; ++it)
    {
    if ((*it))
      {
      s << " " << (*it)->GetWidgetName();
      }
    }

  s << " -sticky news -row 0 "
    << " -ipadx " 
    << (this->PadX + (this->WidgetsFlatAspect ? 
                      this->WidgetsFlatAdditionalPadX : 0))
    << " -ipady "
    << (this->PadY + (this->WidgetsFlatAspect ? 
                      this->WidgetsFlatAdditionalPadY : 0))
    << ends;

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
void vtkKWToolbar::SetPadX(int arg)
{
  if (arg == this->PadX)
    {
    return;
    }

  this->PadX = arg;
  this->Modified();

  this->UpdateWidgetsLayout();
}

//----------------------------------------------------------------------------
void vtkKWToolbar::SetPadY(int arg)
{
  if (arg == this->PadX)
    {
    return;
    }

  this->PadY = arg;
  this->Modified();

  this->UpdateWidgetsLayout();
}

//----------------------------------------------------------------------------
void vtkKWToolbar::SetWidgetsFlatAdditionalPadX(int arg)
{
  if (arg == this->WidgetsFlatAdditionalPadX)
    {
    return;
    }

  this->WidgetsFlatAdditionalPadX = arg;
  this->Modified();

  this->UpdateWidgetsLayout();
}

//----------------------------------------------------------------------------
void vtkKWToolbar::SetWidgetsFlatAdditionalPadY(int arg)
{
  if (arg == this->WidgetsFlatAdditionalPadX)
    {
    return;
    }

  this->WidgetsFlatAdditionalPadY = arg;
  this->Modified();

  this->UpdateWidgetsLayout();
}

//----------------------------------------------------------------------------
void vtkKWToolbar::UpdateToolbarFrameAspect()
{
  if (!this->IsCreated())
    {
    return;
    }

  const char *common_opts = " -side left -anchor nw -fill both -expand n";

  if (this->FlatAspect)
    {
    this->Script("%s config -relief flat -bd 0", this->GetWidgetName());

    this->Script("pack %s -ipadx 0 -ipady 0 -padx 0 -pady 0 %s",
                 this->Frame->GetWidgetName(), common_opts);
    }
  else
    {
    this->Script("%s config -relief raised -bd 1", this->GetWidgetName());

    this->Script("pack %s -ipadx 1 -ipady 1 -padx 0 -pady 0 %s",
                 this->Frame->GetWidgetName(), common_opts);
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbar::Update()
{
  this->UpdateEnableState();

  this->UpdateToolbarFrameAspect();
}

//----------------------------------------------------------------------------
void vtkKWToolbar::SetFlatAspect(int f)
{
  if (this->FlatAspect == f)
    {
    return;
    }

  this->FlatAspect = f;
  this->Modified();

  this->UpdateToolbarFrameAspect();
  this->UpdateWidgets();
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkKWToolbar::SetResizable(int r)
{
  if (this->Resizable == r)
    {
    return;
    }

  this->Resizable = r;
  this->Modified();

  this->UpdateWidgets();
}

//----------------------------------------------------------------------------
void vtkKWToolbar::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  vtkKWToolbarInternals::WidgetsContainerIterator it = 
    this->Internals->Widgets.begin();
  vtkKWToolbarInternals::WidgetsContainerIterator end = 
    this->Internals->Widgets.end();
  for (; it != end; ++it)
    {
    if ((*it))
      {
      (*it)->SetEnabled(this->Enabled);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbar::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Frame: " << this->Frame << endl;
  os << indent << "Resizable: " << (this->Resizable ? "On" : "Off") << endl;
  os << indent << "FlatAspect: " << (this->FlatAspect ? "On" : "Off") << endl;
  os << indent << "WidgetsFlatAspect: " << (this->WidgetsFlatAspect ? "On" : "Off") << endl;
  os << indent << "PadX: " << this->PadX << endl;
  os << indent << "PadY: " << this->PadY << endl;
  os << indent << "WidgetsFlatAdditionalPadX: " << this->WidgetsFlatAdditionalPadX << endl;
  os << indent << "WidgetsFlatAdditionalPadY: " << this->WidgetsFlatAdditionalPadY << endl;
}

