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

#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWPushButton.h"
#include "vtkKWRadioButton.h"
#include "vtkKWTkUtilities.h"
#include "vtkObjectFactory.h"

#include <vtksys/stl/list>
#include <vtksys/stl/algorithm>

#ifdef _WIN32
static int vtkKWToolbarGlobalToolbarAspect = vtkKWToolbar::ToolbarAspectFlat;
static int vtkKWToolbarGlobalWidgetsAspect = vtkKWToolbar::WidgetsAspectFlat;
#else
static int vtkKWToolbarGlobalToolbarAspect = vtkKWToolbar::ToolbarAspectRelief;
static int vtkKWToolbarGlobalWidgetsAspect = vtkKWToolbar::WidgetsAspectRelief;
#endif

const char *vtkKWToolbar::ToolbarAspectRegKey = "ToolbarFlatFrame";
const char *vtkKWToolbar::WidgetsAspectRegKey = "ToolbarFlatButtons";

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWToolbar );
vtkCxxRevisionMacro(vtkKWToolbar, "1.68");

//----------------------------------------------------------------------------
class vtkKWToolbarInternals
{
public:

  typedef vtksys_stl::list<vtkKWWidget*> WidgetsContainer;
  typedef vtksys_stl::list<vtkKWWidget*>::iterator WidgetsContainerIterator;

  WidgetsContainer Widgets;
};

//----------------------------------------------------------------------------
vtkKWToolbar::vtkKWToolbar()
{
  this->Frame   = vtkKWFrame::New();
  this->Handle  = vtkKWFrame::New();

  this->ToolbarAspect             = vtkKWToolbar::GetGlobalToolbarAspect();
  this->WidgetsAspect             = vtkKWToolbar::GetGlobalWidgetsAspect();
  this->Resizable                 = 0;
  this->Expanding                 = 0;

  this->WidgetsFlatAdditionalPadX = 0;
  this->WidgetsFlatAdditionalPadY = 0;

  this->WidgetsPadX = 1;
  this->WidgetsPadY = 1;

  // This widget is used to keep track of default options

  this->DefaultOptionsWidget = vtkKWRadioButton::New();

  // Internal structs

  this->Internals = new vtkKWToolbarInternals;

  this->Name = NULL;
}

//----------------------------------------------------------------------------
vtkKWToolbar::~vtkKWToolbar()
{
  if (this->IsAlive())
    {
    this->UnBind();
    }

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

  this->RemoveAllWidgets();
  if (this->Internals)
    {
    delete this->Internals;
    }
  
  this->SetName(NULL);
}

//----------------------------------------------------------------------------
void vtkKWToolbar::Bind()
{
  this->SetBinding("<Configure>", this, "ScheduleResize");
}

//----------------------------------------------------------------------------
void vtkKWToolbar::UnBind()
{
  this->RemoveBinding("<Configure>");
}
//----------------------------------------------------------------------------
void vtkKWToolbar::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();
  
  this->Bind();

  // Create the widgets container itself

  this->Frame->SetParent(this);
  this->Frame->Create();
  
  // Create a "toolbar handle"

  this->Handle->SetParent(this);
  this->Handle->Create();
  this->Handle->SetBorderWidth(2);
  this->Handle->SetReliefToRaised();

  // Create the default options repository (never packed, just a way
  // to keep track of default options)

  this->DefaultOptionsWidget->SetParent(this);
  this->DefaultOptionsWidget->Create();

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
  this->PropagateEnableState(widget);

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
      vtksys_stl::find(this->Internals->Widgets.begin(),
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
  this->PropagateEnableState(widget);

  this->UpdateWidgets();
}

//----------------------------------------------------------------------------
int vtkKWToolbar::HasWidget(vtkKWWidget *widget)
{
  if (!widget || !this->Internals)
    {
    return 0;
    }

  vtkKWToolbarInternals::WidgetsContainerIterator location_pos = 
    vtksys_stl::find(this->Internals->Widgets.begin(),
                     this->Internals->Widgets.end(),
                     widget);
  return (location_pos == this->Internals->Widgets.end() ? 0 : 1);
}

//----------------------------------------------------------------------------
int vtkKWToolbar::GetNumberOfWidgets()
{
  if (this->Internals)
    {
    return this->Internals->Widgets.size();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWToolbar::RemoveWidget(vtkKWWidget *widget)
{
  if (!widget || !this->Internals)
    {
    return;
    }

  vtkKWToolbarInternals::WidgetsContainerIterator location_pos = 
    vtksys_stl::find(this->Internals->Widgets.begin(),
                 this->Internals->Widgets.end(),
                 widget);
  if (location_pos == this->Internals->Widgets.end())
    {
    vtkErrorMacro("Unable to remove widget from toolbar");
    return;
    }

  (*location_pos)->UnRegister(this);
  this->Internals->Widgets.erase(location_pos);

  this->UpdateWidgets();
}

//----------------------------------------------------------------------------
void vtkKWToolbar::RemoveAllWidgets()
{
  if (!this->Internals)
    {
    return;
    }

  vtkKWToolbarInternals::WidgetsContainerIterator it = 
    this->Internals->Widgets.begin();
  vtkKWToolbarInternals::WidgetsContainerIterator end = 
    this->Internals->Widgets.end();
  for (; it != end; ++it)
    {
    if (*it)
      {
      (*it)->UnRegister(this);
      }
    }
  this->Internals->Widgets.clear();

  this->UpdateWidgets();
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
          vtkKWCoreWidget *core = vtkKWCoreWidget::SafeDownCast(*it);
          if (core->HasConfigurationOption(options[i]) && core->IsCreated())
            {
            const char *option = core->GetConfigurationOption(options[i]);
            if (!strcmp(name, option))
              {
              return core;
              }
            }
          }
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
vtkKWWidget* vtkKWToolbar::GetNthWidget(int rank)
{
  if (this->Internals)
    {
    vtkKWToolbarInternals::WidgetsContainerIterator it = 
      this->Internals->Widgets.begin();
    vtkKWToolbarInternals::WidgetsContainerIterator end = 
      this->Internals->Widgets.end();
    for (; it != end && rank >= 0; ++it, --rank)
      {
      if (rank == 0)
        {
        return *it;
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
                                               vtkObject *object, 
                                               const char *method,
                                               const char *help)
{
  if (!this->IsCreated())
    {
    return 0;
    }

  vtkKWRadioButton *rb = vtkKWRadioButton::New();

  rb->SetParent(this->GetFrame());
  rb->Create();
  rb->IndicatorVisibilityOff();
  rb->SetValueAsInt(value);

  if (image_name)
    {
    rb->SetHighlightThickness(0);
    rb->SetConfigurationOption("-image", image_name);
    rb->SetConfigurationOption(
      "-selectimage", (select_image_name ? select_image_name : image_name));
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

  this->AddWidget(rb);

  rb->Delete();

  return rb;
}

//----------------------------------------------------------------------------
vtkKWWidget* vtkKWToolbar::AddCheckButtonImage(const char *image_name, 
                                               const char *select_image_name, 
                                               const char *variable_name, 
                                               vtkObject *object, 
                                               const char *method,
                                               const char *help)
{
  if (!this->IsCreated())
    {
    return 0;
    }

  vtkKWCheckButton *cb = vtkKWCheckButton::New();

  cb->SetParent(this->GetFrame());
  cb->Create();
  cb->IndicatorVisibilityOff();

  if (image_name)
    {
    cb->SetHighlightThickness(0);
    cb->SetConfigurationOption("-image", image_name);
    cb->SetConfigurationOption(
      "-selectimage", (select_image_name ? select_image_name : image_name));
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

  this->AddWidget(cb);

  cb->Delete();

  return cb;
}

//----------------------------------------------------------------------------
void vtkKWToolbar::ScheduleResize()
{  
  if (this->Expanding || !this->IsCreated())
    {
    return;
    }
  this->Expanding = 1;
  this->Script("after idle {catch {%s Resize}}", this->GetTclName());
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
  if (!this->IsCreated() || 
      !this->Internals || this->Internals->Widgets.size() <= 0)
    {
    return;
    }

  vtkKWToolbarInternals::WidgetsContainerIterator it = 
    this->Internals->Widgets.begin();
  vtkKWToolbarInternals::WidgetsContainerIterator end = 
    this->Internals->Widgets.end();
  for (; it != end; ++it)
    {
    vtkKWPushButton *pb = vtkKWPushButton::SafeDownCast(*it);
    vtkKWCheckButton *cb = vtkKWCheckButton::SafeDownCast(*it);
    vtkKWRadioButton *rb = vtkKWRadioButton::SafeDownCast(*it);
    if (pb)
      {
      if (this->WidgetsAspect == vtkKWToolbar::WidgetsAspectFlat)
        {
        pb->SetReliefToFlat();
        pb->SetOverReliefToSolid();
        pb->SetBorderWidth(1);
        }
      else if (this->WidgetsAspect == vtkKWToolbar::WidgetsAspectRelief)
        {
        pb->SetReliefToRaised();
        pb->SetOverReliefToNone();
        pb->SetBorderWidth(1);
        }
      }
    else if (cb)
      {
      if (this->WidgetsAspect == vtkKWToolbar::WidgetsAspectFlat)
        {
        cb->SetReliefToFlat();
        cb->SetOffReliefToFlat();
        cb->SetOverReliefToSolid();
        cb->SetBorderWidth(1);
        }
      else if (this->WidgetsAspect == vtkKWToolbar::WidgetsAspectRelief)
        {
        cb->SetReliefToFlat();
        cb->SetOffReliefToRaised();
        cb->SetOverReliefToNone();
        cb->SetBorderWidth(1);
        }
      }
    else if (rb)
      {
      if (this->WidgetsAspect == vtkKWToolbar::WidgetsAspectFlat)
        {
        rb->SetReliefToFlat();
        rb->SetOffReliefToFlat();
        rb->SetOverReliefToSolid();
        rb->SetBorderWidth(1);
        }
      else if (this->WidgetsAspect == vtkKWToolbar::WidgetsAspectRelief)
        {
        rb->SetReliefToFlat();
        rb->SetOffReliefToRaised();
        rb->SetOverReliefToNone();
        rb->SetBorderWidth(1);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbar::ConstrainWidgetsLayout()
{
  if (!this->IsCreated() || 
      !this->Internals || this->Internals->Widgets.size() <= 0)
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
      int reqw = 0;
      vtkKWTkUtilities::GetWidgetRequestedSize((*it), &reqw, NULL);
      totReqWidth += this->WidgetsPadX + reqw;
      if (this->WidgetsAspect == vtkKWToolbar::WidgetsAspectFlat)
        {
        totReqWidth += this->WidgetsFlatAdditionalPadX;
        }
      }
    }

  int width = 0;
  vtkKWTkUtilities::GetWidgetSize(this, &width, NULL);

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
          << " -in " << this->GetFrame()->GetWidgetName()
          << " -padx " 
          << (this->WidgetsPadX + 
              (this->WidgetsAspect == vtkKWToolbar::WidgetsAspectFlat ? 
               this->WidgetsFlatAdditionalPadX : 0))
          << " -pady "
          << (this->WidgetsPadY + 
              (this->WidgetsAspect == vtkKWToolbar::WidgetsAspectFlat ? 
               this->WidgetsFlatAdditionalPadY : 0))
          << endl;
        num++;
        if (num == numPerRow) 
          { 
          row++; 
          num = 0;
          }
        }
      }

    while (row >= 0)
      {
      s << "grid rowconfigure " << this->GetFrame()->GetWidgetName() 
        << " "<< row << " -weight 1 " << endl;
      --row;
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
    << " -in " << this->GetFrame()->GetWidgetName()
    << " -padx " 
    << (this->WidgetsPadX + 
        (this->WidgetsAspect == vtkKWToolbar::WidgetsAspectFlat ? 
         this->WidgetsFlatAdditionalPadX : 0))
    << " -pady "
    << (this->WidgetsPadY + 
        (this->WidgetsAspect == vtkKWToolbar::WidgetsAspectFlat ? 
         this->WidgetsFlatAdditionalPadY : 0))
    << endl;

  s << "grid rowconfigure " << this->GetFrame()->GetWidgetName() 
    << " 0 -weight 1 " << endl;

  s << ends;
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
void vtkKWToolbar::SetWidgetsPadX(int arg)
{
  if (arg == this->WidgetsPadX)
    {
    return;
    }

  this->WidgetsPadX = arg;
  this->Modified();

  this->UpdateWidgetsLayout();
}

//----------------------------------------------------------------------------
void vtkKWToolbar::SetWidgetsPadY(int arg)
{
  if (arg == this->WidgetsPadX)
    {
    return;
    }

  this->WidgetsPadY = arg;
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

  if (this->ToolbarAspect == vtkKWToolbar::ToolbarAspectFlat)
    {
    this->SetReliefToFlat();
    this->SetBorderWidth(0);
    this->Script("pack %s -ipadx 0 -ipady 0 -padx 0 -pady 0 %s",
                 this->Frame->GetWidgetName(), common_opts);
    }
  else if (this->ToolbarAspect == vtkKWToolbar::ToolbarAspectRelief)
    {
    this->SetReliefToRaised();
    this->SetBorderWidth(1);
    this->Script("pack %s -ipadx 1 -ipady 1 -padx 0 -pady 0 %s",
                 this->Frame->GetWidgetName(), common_opts);
    }
  else
    {
    this->Script("pack %s -ipadx 0 -ipady 0 -padx 0 -pady 0 %s",
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
void vtkKWToolbar::SetToolbarAspect(int f)
{
  if (f < vtkKWToolbar::ToolbarAspectRelief)
    {
    f = vtkKWToolbar::ToolbarAspectRelief;
    }
  else if (f > vtkKWToolbar::ToolbarAspectUnChanged)
    {
    f = vtkKWToolbar::ToolbarAspectUnChanged;
    }

  if (this->ToolbarAspect == f)
    {
    return;
    }

  this->ToolbarAspect = f;
  this->Modified();

  this->UpdateToolbarFrameAspect();
  this->UpdateWidgets();
}

void vtkKWToolbar::SetToolbarAspectToFlat()
{
  this->SetToolbarAspect(vtkKWToolbar::ToolbarAspectFlat);
}

void vtkKWToolbar::SetToolbarAspectToRelief()
{
  this->SetToolbarAspect(vtkKWToolbar::ToolbarAspectRelief);
}

void vtkKWToolbar::SetToolbarAspectToUnChanged()
{
  this->SetToolbarAspect(vtkKWToolbar::ToolbarAspectUnChanged);
}

int vtkKWToolbar::GetGlobalToolbarAspect() 
{ 
  return vtkKWToolbarGlobalToolbarAspect; 
}

void vtkKWToolbar::SetGlobalToolbarAspect(int val) 
{ 
  vtkKWToolbarGlobalToolbarAspect = val; 
};

void vtkKWToolbar::SetGlobalToolbarAspectToFlat()
{
  vtkKWToolbar::SetGlobalToolbarAspect(vtkKWToolbar::ToolbarAspectFlat);
}

void vtkKWToolbar::SetGlobalToolbarAspectToRelief()
{
  vtkKWToolbar::SetGlobalToolbarAspect(vtkKWToolbar::ToolbarAspectRelief);
}

void vtkKWToolbar::SetGlobalToolbarAspectToUnChanged()
{
  vtkKWToolbar::SetGlobalToolbarAspect(vtkKWToolbar::ToolbarAspectUnChanged);
}

//----------------------------------------------------------------------------
void vtkKWToolbar::SetWidgetsAspect(int f)
{
  if (f < vtkKWToolbar::WidgetsAspectRelief)
    {
    f = vtkKWToolbar::WidgetsAspectRelief;
    }
  else if (f > vtkKWToolbar::WidgetsAspectUnChanged)
    {
    f = vtkKWToolbar::WidgetsAspectUnChanged;
    }

  if (this->WidgetsAspect == f)
    {
    return;
    }

  this->WidgetsAspect = f;
  this->Modified();

  this->UpdateWidgets();
}

void vtkKWToolbar::SetWidgetsAspectToFlat()
{
  this->SetWidgetsAspect(vtkKWToolbar::WidgetsAspectFlat);
}

void vtkKWToolbar::SetWidgetsAspectToRelief()
{
  this->SetWidgetsAspect(vtkKWToolbar::WidgetsAspectRelief);
}

void vtkKWToolbar::SetWidgetsAspectToUnChanged()
{
  this->SetWidgetsAspect(vtkKWToolbar::WidgetsAspectUnChanged);
}

int vtkKWToolbar::GetGlobalWidgetsAspect() 
{ 
  return vtkKWToolbarGlobalWidgetsAspect; 
}

void vtkKWToolbar::SetGlobalWidgetsAspect(int val) 
{ 
  vtkKWToolbarGlobalWidgetsAspect = val; 
};

void vtkKWToolbar::SetGlobalWidgetsAspectToFlat()
{
  vtkKWToolbar::SetGlobalWidgetsAspect(vtkKWToolbar::WidgetsAspectFlat);
}

void vtkKWToolbar::SetGlobalWidgetsAspectToRelief()
{
  vtkKWToolbar::SetGlobalWidgetsAspect(vtkKWToolbar::WidgetsAspectRelief);
}

void vtkKWToolbar::SetGlobalWidgetsAspectToUnChanged()
{
  vtkKWToolbar::SetGlobalWidgetsAspect(vtkKWToolbar::WidgetsAspectUnChanged);
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
    this->PropagateEnableState(*it);
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbar::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Frame: " << this->Frame << endl;
  os << indent << "Resizable: " << (this->Resizable ? "On" : "Off") << endl;
  os << indent << "ToolbarAspect: " << this->ToolbarAspect << endl;
  os << indent << "WidgetsAspect: " << this->WidgetsAspect << endl;
  os << indent << "WidgetsPadX: " << this->WidgetsPadX << endl;
  os << indent << "WidgetsPadY: " << this->WidgetsPadY << endl;
  os << indent << "WidgetsFlatAdditionalPadX: " << this->WidgetsFlatAdditionalPadX << endl;
  os << indent << "WidgetsFlatAdditionalPadY: " << this->WidgetsFlatAdditionalPadY << endl;
  os << indent << "Name: " << (this->Name ? this->Name : "None") << endl;
}

