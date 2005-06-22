/*=========================================================================

  Module:    vtkKWWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWWidget.h"

#include "vtkKWApplication.h"
#include "vtkKWIcon.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWWindowBase.h"
#include "vtkKWDragAndDropTargetSet.h"
#include "vtkObjectFactory.h"
#include "vtkKWBalloonHelpManager.h"

#include <vtksys/stl/vector>
#include <vtksys/stl/algorithm>
#include <vtksys/SystemTools.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWWidget );
vtkCxxRevisionMacro(vtkKWWidget, "1.133");

//----------------------------------------------------------------------------
class vtkKWWidgetInternals
{
public:
  typedef vtksys_stl::vector<vtkKWWidget*> WidgetsContainer;
  typedef vtksys_stl::vector<vtkKWWidget*>::iterator WidgetsContainerIterator;

  WidgetsContainer *Children;

  vtkKWWidgetInternals() { this->Children = NULL; };
  ~vtkKWWidgetInternals() { delete this->Children; };
};

//----------------------------------------------------------------------------
vtkKWWidget::vtkKWWidget()
{
  // Instantiate the PIMPL Encapsulation for STL containers

  this->Internals = new vtkKWWidgetInternals;

  this->WidgetName               = NULL;
  this->Parent                   = NULL;

  this->BalloonHelpString        = NULL;  

  this->Enabled                  = 1;

  this->WidgetIsCreated          = 0;

  this->DragAndDropTargetSet       = NULL;
}

//----------------------------------------------------------------------------
vtkKWWidget::~vtkKWWidget()
{
  if (this->Internals)
    {
    delete this->Internals;
    this->Internals = NULL;
    }

  if (this->DragAndDropTargetSet)
    {
    this->DragAndDropTargetSet->Delete();
    this->DragAndDropTargetSet = NULL;
    }

  if (this->BalloonHelpString)
    {
    this->SetBalloonHelpString(NULL);
    }

  if (this->IsCreated())
    {
    this->Script("destroy %s", this->GetWidgetName());
    }

  if (this->WidgetName)
    {
    delete [] this->WidgetName;
    this->WidgetName = NULL;
    }

  this->SetParent(NULL);
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetParent(vtkKWWidget *p)
{
  if (this->Parent && p)
    {
    vtkErrorMacro("Error attempt to reparent a widget!");
    return;
    }
  if (this->Parent)
    {
    vtkKWWidget *tmp = this->Parent;
    this->Parent = NULL;
    tmp->UnRegister(this);
    tmp->RemoveChild(this);
    }
  else if (p)
    {
    this->Parent = p;
    p->Register(this);
    p->AddChild(this);
    }
}

//----------------------------------------------------------------------------
const char *vtkKWWidget::GetWidgetName()
{
  static unsigned long count = 0;

  // is the name is already set the just return it
  if (this->WidgetName)
    {
    return this->WidgetName;
    }

  // create this widgets name
  char local[256];
  // get the parents name
  if (this->Parent)
    {
    const char *tmp = this->Parent->GetWidgetName();
    sprintf(local,"%s.%lu",tmp,count);
    }
  else
    {
    sprintf(local,".%lu",count);
    }
  count++;
  this->WidgetName = new char [strlen(local)+1];
  strcpy(this->WidgetName,local);
  return this->WidgetName;
}

//----------------------------------------------------------------------------
void vtkKWWidget::Create(vtkKWApplication *app)
{
  this->CreateSpecificTkWidget(app, NULL, NULL);
}

//----------------------------------------------------------------------------
int vtkKWWidget::CreateSpecificTkWidget(vtkKWApplication *app, 
                                        const char *type, 
                                        const char *args)
{
  if (this->IsCreated())
    {
    if (type)
      {
      vtkErrorMacro(
        << this->GetClassName() << " (" << type << ") already created");
      }
    else
      {
      vtkErrorMacro(<< this->GetClassName() << " already created");
      }
    return 0;
    }

  if (!app)
    {
    vtkErrorMacro("Can not create widget with NULL application. Make sure you did not SafeDowncast a vtkKWApplication to a more specific subclass.");
    return 0;
    }

  this->SetApplication(app);

  if (this->HasDragAndDropTargetSet())
    {
    this->GetDragAndDropTargetSet()->SetApplication(app);
    }

  const char *ret = NULL;

  if (!type)
    {
    this->WidgetIsCreated = 1;
    }
  else
    {
    if (args)
      {
      ret = this->Script("%s %s %s", type, this->GetWidgetName(), args);
      }
    else
      {
      ret = this->Script("%s %s", type, this->GetWidgetName());
      }
    if (ret && strcmp(ret, this->GetWidgetName()))
      {
      vtkErrorMacro("Error creating the widget " << this->GetWidgetName() 
                    << " of type " << type << ": " << ret);
      return 0;
      }

    this->WidgetIsCreated = 1;

    /* Update enable state
       At this point, the widget is considered created, although for all 
       all subclasses calling this method, only a part of the widget has
       really been created (for ex., this method will be used to create the
       main container for the widget, like a frame, and the subclass will
       create the subwidgets to put in). 
       Ideally, we could have a function that explicitly sets when a widget
       is fully created or not.
       Anyway, most subclasses override the virtual UpdateEnableState()
       method to propagate the Enabled state to their subwidgets components.
       If we call it now, it will immediately go inside the overriden method
       and try to act on widget that have propably not been created yet: it
       is therefore important that the overriden UpdateEnableState() is smart
       enough and test if each subwidget has really been created 
       (subobj->IsCreated() instead of just this->IsCreated(), which will 
       return true at the moment). It is the case at the moment.
       Also, each subclass should call UpdateEnableState() at the end of their
       own Create() method, so that the code that is supposed to be executed
       in the overriden UpdateEnableState() for the subwidgets is really 
       executed now that they have been created.
       The call here, even if it goes down to the subclass, is still needed.
    */
   
    this->UpdateEnableState();

    // If the balloon help string has been set, make sure the bindings
    // are set too, now that we have been created

    if (this->BalloonHelpString)
      {
      vtkKWBalloonHelpManager *mgr = 
        this->GetApplication()->GetBalloonHelpManager();
      if (mgr)
        {
        mgr->AddBindings(this);
        }
      }
    }

  return 1;
}

// ---------------------------------------------------------------------------
int vtkKWWidget::IsCreated()
{
  return (this->GetApplication() != NULL && this->WidgetIsCreated);
}

//----------------------------------------------------------------------------
void vtkKWWidget::AddChild(vtkKWWidget *child) 
{
  if (this->Internals)
    {
    if (!this->Internals->Children)
      {
      this->Internals->Children = new vtkKWWidgetInternals::WidgetsContainer;
      }
    this->Internals->Children->push_back(child);
    child->Register(this);
    }
}

//----------------------------------------------------------------------------
int vtkKWWidget::HasChild(vtkKWWidget *child) 
{
  if (this->GetNumberOfChildren())
    {
    return vtksys_stl::find(this->Internals->Children->begin(),
                           this->Internals->Children->end(),
                           child) == this->Internals->Children->end() ? 0 : 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWWidget::RemoveChild(vtkKWWidget *child) 
{
  if (this->GetNumberOfChildren())
    {
    this->Internals->Children->erase(
      vtksys_stl::find(this->Internals->Children->begin(),
                      this->Internals->Children->end(),
                      child));
    child->UnRegister(this);
    }
}

//----------------------------------------------------------------------------
void vtkKWWidget::RemoveAllChildren()
{
  int nb_children = this->GetNumberOfChildren();
  if (nb_children)
    {
    do
      {
      vtkKWWidget *child = this->GetNthChild(nb_children - 1);
      child->SetParent(NULL);
      // No need for:  child->UnRegister(this);
      // => child->SetParent(NULL) will call us again with RemoveChild(child)
      // which UnRegister child.
      nb_children = this->GetNumberOfChildren();
      } while (nb_children);
    this->Internals->Children->clear();
    }
}

//----------------------------------------------------------------------------
vtkKWWidget* vtkKWWidget::GetNthChild(int rank)
{
  if (rank >= 0 && rank < this->GetNumberOfChildren())
    {
    return (*this->Internals->Children)[rank];
    }
  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWWidget::GetNumberOfChildren()
{
  if (this->Internals && this->Internals->Children)
    {
    return this->Internals->Children->size();
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkKWWidget* vtkKWWidget::GetChildWidgetWithName(const char *name)
{
  int nb_children = this->GetNumberOfChildren();
  if (name && nb_children)
    {
    for (int i = 0; i < nb_children; i++)
      {
      vtkKWWidget *child = this->GetNthChild(i);
      const char *wname = child->GetWidgetName();
      if (wname && !strcmp(wname, name))
        {
        return child;
        }
      }
    }

  return NULL;
}
       
//----------------------------------------------------------------------------
int  vtkKWWidget::GetNetReferenceCount() 
{
  int child_counts = 0;

  int nb_children = this->GetNumberOfChildren();
  if (nb_children)
    {
    for (int i = 0; i < nb_children; i++)
      {
      vtkKWWidget *child = this->GetNthChild(i);
      child_counts += child->GetNetReferenceCount();
      }
    child_counts -= 2 * nb_children;
    }

  return this->ReferenceCount + child_counts;
}

//----------------------------------------------------------------------------
void vtkKWWidget::UnRegister(vtkObjectBase *o)
{
  // Delete the children if we are about to be deleted

  int nb_children = this->GetNumberOfChildren();
  if (nb_children && 
      this->ReferenceCount == nb_children + 1 &&
      !this->HasChild((vtkKWWidget*)(o)))
    {
    this->RemoveAllChildren();
    }
  
  this->Superclass::UnRegister(o);
}

//----------------------------------------------------------------------------
void vtkKWWidget::Focus()
{
  if (this->IsCreated())
    {
    this->Script("focus %s", this->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetBind(vtkKWObject* CalledObject, const char *Event, const char *CommandString)
{
  this->Script("bind %s %s { %s %s }", this->GetWidgetName(), 
               Event, CalledObject->GetTclName(), CommandString);
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetBind(const char *Event, const char *CommandString)
{
  this->Script("bind %s %s { %s }", this->GetWidgetName(), Event, CommandString);
}

//----------------------------------------------------------------------------
void vtkKWWidget::UnsetBind(const char *Event)
{
  this->SetBind(Event, "");
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetBind(const char *event, const char *widget, const char *command)
{
  this->Script("bind %s %s { %s %s }", this->GetWidgetName(), event, widget, command);
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetBindAll(const char *event, const char *widget, const char *command)
{
  this->Script("bind all %s { %s %s }", event, widget, command);
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetBindAll(const char *event, const char *command)
{
  this->Script("bind all %s { %s }", event, command);
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetCommand(vtkKWObject* CalledObject, const char * CommandString)
{
  if (!this->IsCreated())
    {
    return;
    }

  char *command = NULL;
  this->SetObjectMethodCommand(&command, CalledObject, CommandString);
  this->Script("%s configure -command {%s}", this->GetWidgetName(), command);
  delete [] command;
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetBalloonHelpString(const char *str)
{
  if (this->BalloonHelpString == NULL && str == NULL)
    {
    return;
    }

  if (this->BalloonHelpString)
    {
    delete [] this->BalloonHelpString;
    this->BalloonHelpString = NULL;
    }

  if (str != NULL)
    {
    this->BalloonHelpString = new char[strlen(str) + 1];
    strcpy(this->BalloonHelpString, str);
    }
  
  if (this->BalloonHelpString && this->GetApplication() && this->IsCreated())
    {
    vtkKWBalloonHelpManager *mgr = 
      this->GetApplication()->GetBalloonHelpManager();
    if (mgr)
      {
      mgr->AddBindings(this);
      }
    }
}

//----------------------------------------------------------------------------
vtkKWWindowBase* vtkKWWidget::GetWindow()
{
  vtkKWWindowBase* win =0;
  vtkKWWidget* widget = this->GetParent();
  while(widget)
    {
    if((win = vtkKWWindowBase::SafeDownCast(widget)))
      {
      return win;
      }
    widget = widget->GetParent();
    }
  return win;
}

//----------------------------------------------------------------------------
const char* vtkKWWidget::GetType()
{
  if (this->IsCreated())
    {
    return this->Script("winfo class %s", this->GetWidgetName());
    }
  return "None";
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetEnabled(int e)
{
  if ( this->Enabled == e )
    {
    return;
    }
  this->Enabled = e;

  this->UpdateEnableState();

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWWidget::UpdateEnableState()
{
}

//----------------------------------------------------------------------------
void vtkKWWidget::PropagateEnableState(vtkKWWidget* widget)
{
  if ( !widget || widget == this )
    {
    return;
    }
  widget->SetEnabled(this->GetEnabled());
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetStateOption(int flag)
{
  if (this->IsAlive())
    {
    this->SetConfigurationOption("-state", (flag ? "normal" : "disabled")); 
    }
}

//----------------------------------------------------------------------------
int vtkKWWidget::GetStateOption()
{
  if (this->IsAlive())
    {
    return !strcmp(this->GetConfigurationOption("-state"), "normal") ? 1 : 0; 
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWWidget::GetBackgroundColor(double *r, double *g, double *b)
{
  vtkKWTkUtilities::GetOptionColor(this, "-background", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWWidget::GetBackgroundColor()
{
  static double rgb[3];
  this->GetBackgroundColor(rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetBackgroundColor(double r, double g, double b)
{
  vtkKWTkUtilities::SetOptionColor(this, "-background", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWWidget::GetForegroundColor(double *r, double *g, double *b)
{
  vtkKWTkUtilities::GetOptionColor(this, "-foreground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWWidget::GetForegroundColor()
{
  static double rgb[3];
  this->GetForegroundColor(rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetForegroundColor(double r, double g, double b)
{
  vtkKWTkUtilities::SetOptionColor(this, "-foreground", r, g, b);
}

//----------------------------------------------------------------------------
int vtkKWWidget::HasConfigurationOption(const char* option)
{
  if (!this->IsCreated())
    {
    vtkWarningMacro("Widget is not created yet !");
    return 0;
    }

  return (this->GetApplication() && 
          !this->GetApplication()->EvaluateBooleanExpression(
            "catch {%s cget %s}",
            this->GetWidgetName(), option));
}

//----------------------------------------------------------------------------
const char* vtkKWWidget::GetConfigurationOption(const char* option)
{
  if (!this->HasConfigurationOption(option))
    {
    return NULL;
    }

  return this->Script("%s cget %s", this->GetWidgetName(), option);
}

//----------------------------------------------------------------------------
int vtkKWWidget::GetConfigurationOptionAsInt(const char* option)
{
  if (!this->HasConfigurationOption(option))
    {
    return 0;
    }

  return atoi(this->Script("%s cget %s", this->GetWidgetName(), option));
}

//----------------------------------------------------------------------------
const char* vtkKWWidget::ConvertInternalStringToTclString(
  const char *source, int options)
{
  if (!source || !this->IsCreated())
    {
    return NULL;
    }

  static vtksys_stl::string dest;
  const char *res = source;

  // Handle the encoding

  int app_encoding = this->GetApplication()->GetCharacterEncoding();
  if (app_encoding != VTK_ENCODING_NONE &&
      app_encoding != VTK_ENCODING_UNKNOWN)
    {
    // Get the Tcl encoding name

    const char *tcl_encoding_name = 
      vtkKWTkOptions::GetCharacterEncodingAsTclOptionValue(app_encoding);

    // Check if we have that encoding
    
    Tcl_Encoding tcl_encoding = 
      Tcl_GetEncoding(
        this->GetApplication()->GetMainInterp(), tcl_encoding_name);
    if (tcl_encoding != NULL)
      {
      Tcl_FreeEncoding(tcl_encoding);
      
      // Convert from that encoding
      // We need to escape interpretable chars to perform that conversion

      dest = vtksys::SystemTools::EscapeChars(source, "[]$\"");
      res = source = this->Script(
        "encoding convertfrom %s \"%s\"", tcl_encoding_name, dest.c_str());
      }
    }

  // Escape
  
  vtksys_stl::string escape_chars;
  if (options)
    {
    if (options & vtkKWWidget::ConvertStringEscapeCurlyBraces)
      {
      escape_chars += "{}";
      }
    if (options & vtkKWWidget::ConvertStringEscapeInterpretable)
      {
      escape_chars += "[]$\"";
      }
    dest = 
      vtksys::SystemTools::EscapeChars(source, escape_chars.c_str());
    res = source = dest.c_str();
    }

  return res;
}

//----------------------------------------------------------------------------
const char* vtkKWWidget::ConvertTclStringToInternalString(
  const char *source, int options)
{
  if (!source || !this->IsCreated())
    {
    return NULL;
    }

  static vtksys_stl::string dest;
  const char *res = source;

  // Handle the encoding

  int app_encoding = this->GetApplication()->GetCharacterEncoding();
  if (app_encoding != VTK_ENCODING_NONE &&
      app_encoding != VTK_ENCODING_UNKNOWN)
    {
    // Convert from that encoding
    // We need to escape interpretable chars to perform that conversion

    dest = vtksys::SystemTools::EscapeChars(source, "[]$\"");
    res = source = this->Script(
      "encoding convertfrom identity \"%s\"", dest.c_str());
    }
  
  // Escape
  
  vtksys_stl::string escape_chars;
  if (options)
    {
    if (options & vtkKWWidget::ConvertStringEscapeCurlyBraces)
      {
      escape_chars += "{}";
      }
    if (options & vtkKWWidget::ConvertStringEscapeInterpretable)
      {
      escape_chars += "[]$\"";
      }
    dest = 
      vtksys::SystemTools::EscapeChars(source, escape_chars.c_str());
    res = source = dest.c_str();
    }

  return res;
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetTextOption(const char *str, const char *option)
{
  if (!option || !this->IsCreated())
    {
    return;
    }

  const char *val = this->ConvertInternalStringToTclString(
    str, vtkKWWidget::ConvertStringEscapeInterpretable);
  this->Script("%s configure %s \"%s\"", 
               this->GetWidgetName(), option, val ? val : "");
}

//----------------------------------------------------------------------------
const char* vtkKWWidget::GetTextOption(const char *option)
{
  if (!option || !this->IsCreated())
    {
    return "";
    }

  return this->ConvertTclStringToInternalString(
    this->GetConfigurationOption(option));
}

//----------------------------------------------------------------------------
int vtkKWWidget::IsAlive()
{
  if (!this->IsCreated())
    {
    return 0;
    }

  return atoi(this->Script("winfo exists %s", this->GetWidgetName()));
}

//----------------------------------------------------------------------------
int vtkKWWidget::IsMapped()
{
  return this->IsAlive() && this->GetApplication()->EvaluateBooleanExpression(
    "winfo ismapped %s", this->GetWidgetName());
}

//----------------------------------------------------------------------------
int vtkKWWidget::IsPacked()
{
  return this->IsCreated() && !this->GetApplication()->EvaluateBooleanExpression(
    "catch {pack info %s}", this->GetWidgetName());
}

//----------------------------------------------------------------------------
int vtkKWWidget::GetNumberOfPackedChildren()
{
  if (!this->IsCreated())
    {
    return 0;
    }
  return atoi(this->Script("llength [pack slaves %s]", this->GetWidgetName()));
}

//----------------------------------------------------------------------------
void vtkKWWidget::Unpack()
{
  this->Script("catch {eval pack forget %s}",
               this->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWWidget::UnpackSiblings()
{
  if (this->GetParent())
    {
    this->Script("catch {eval pack forget [pack slaves %s]} \n "
                 "catch {eval grid forget [grid slaves %s]}",
                 this->GetParent()->GetWidgetName(),
                 this->GetParent()->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWWidget::UnpackChildren()
{
  this->Script("catch {eval pack forget [pack slaves %s]} \n "
               "catch {eval grid forget [grid slaves %s]}",
               this->GetWidgetName(),this->GetWidgetName());
}

//----------------------------------------------------------------------------
int vtkKWWidget::HasDragAndDropTargetSet()
{
  return this->DragAndDropTargetSet ? 1 : 0;
}

//----------------------------------------------------------------------------
vtkKWDragAndDropTargetSet* vtkKWWidget::GetDragAndDropTargetSet()
{
  // Lazy allocation. Create the drag and drop container only when it is needed

  if (!this->DragAndDropTargetSet)
    {
    this->DragAndDropTargetSet = vtkKWDragAndDropTargetSet::New();
    this->DragAndDropTargetSet->SetApplication(this->GetApplication());
    this->DragAndDropTargetSet->SetSource(this);
    }

  return this->DragAndDropTargetSet;
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetAnchor(int anchor)
{
  this->SetConfigurationOption(
    "-anchor", vtkKWTkOptions::GetAnchorAsTkOptionValue(anchor));
}

//----------------------------------------------------------------------------
int vtkKWWidget::GetAnchor()
{
  return vtkKWTkOptions::GetAnchorFromTkOptionValue(
    this->GetConfigurationOption("-anchor"));
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetHighlightThickness(int width)
{
  this->SetConfigurationOptionAsInt("-highlightthickness", width);
}

//----------------------------------------------------------------------------
int vtkKWWidget::GetHighlightThickness()
{
  return this->GetConfigurationOptionAsInt("-highlightthickness");
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetBorderWidth(int width)
{
  this->SetConfigurationOptionAsInt("-bd", width);
}

//----------------------------------------------------------------------------
int vtkKWWidget::GetBorderWidth()
{
  return this->GetConfigurationOptionAsInt("-bd");
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetRelief(int relief)
{
  this->SetConfigurationOption(
    "-relief", vtkKWTkOptions::GetReliefAsTkOptionValue(relief));
}

//----------------------------------------------------------------------------
int vtkKWWidget::GetRelief()
{
  return vtkKWTkOptions::GetReliefFromTkOptionValue(
    this->GetConfigurationOption("-relief"));
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetPadX(int arg)
{
  this->SetConfigurationOptionAsInt("-padx", arg);
}

//----------------------------------------------------------------------------
int vtkKWWidget::GetPadX()
{
  return this->GetConfigurationOptionAsInt("-padx");
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetPadY(int arg)
{
  this->SetConfigurationOptionAsInt("-pady", arg);
}

//----------------------------------------------------------------------------
int vtkKWWidget::GetPadY()
{
  return this->GetConfigurationOptionAsInt("-pady");
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetImageOption(int icon_index,
                                 const char *blend_color_option,
                                 const char *image_option)
{
  vtkKWIcon *icon = vtkKWIcon::New();
  icon->SetImage(icon_index);
  this->SetImageOption(icon, blend_color_option, image_option);
  icon->Delete();
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetImageOption(vtkKWIcon* icon,
                                 const char *blend_color_option,
                                 const char *image_option)
{
  if (!icon)
    {
    return;
    }

  this->SetImageOption(icon->GetData(), 
                       icon->GetWidth(), 
                       icon->GetHeight(), 
                       icon->GetPixelSize(),
                       0,
                       blend_color_option, 
                       image_option);
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetImageOption(const unsigned char* data, 
                                 int width, 
                                 int height,
                                 int pixel_size,
                                 unsigned long buffer_length,
                                 const char *blend_color_option,
                                 const char *image_option)
{
  if (!this->IsCreated())
    {
    vtkWarningMacro("Widget is not created yet !");
    return;
    }

  if (!image_option || !*image_option)
    {
    image_option = "-image";
    }

  if (!this->HasConfigurationOption(image_option))
    {
    return;
    }

  if (!this->HasConfigurationOption(image_option))
    {
    return;
    }

  ostrstream image_name;
  image_name << this->GetWidgetName() << "." << &image_option[1] << ends;

#if (TK_MAJOR_VERSION == 8) && (TK_MINOR_VERSION < 4)
  // This work-around is put here to "fix" what looks like a bug
  // in Tk. Without this, there seems to be some weird problems
  // with Tk picking some alpha values for some colors.
  this->Script("catch {destroy %s}", image_name.str());
#endif
  
  if (!vtkKWTkUtilities::UpdatePhoto(this->GetApplication(),
                                     image_name.str(),
                                     data, 
                                     width, height, pixel_size,
                                     buffer_length,
                                     this->GetWidgetName(),
                                     blend_color_option))
    {
    vtkWarningMacro("Error updating Tk photo " << image_name.str());
    image_name.rdbuf()->freeze(0);
    return;
    }

  this->SetImageOption(image_name.str(), image_option);

  image_name.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetImageOption(const char *image_name,
                                 const char *image_option)
{
  if (!this->IsCreated())
    {
    vtkWarningMacro("Widget is not created yet !");
    return;
    }

  if (!image_option || !*image_option)
    {
    image_option = "-image";
    }

  if (!this->HasConfigurationOption(image_option))
    {
    return;
    }

  if (!image_name)
    {
    image_name = "";
    }

  this->Script("%s configure %s {%s}", 
               this->GetWidgetName(), image_option, image_name);
}

//----------------------------------------------------------------------------
const char* vtkKWWidget::GetImageOption(const char *image_option)
{
  if (!image_option || !*image_option)
    {
    image_option = "-image";
    }

  return this->GetConfigurationOption(image_option);
}

//----------------------------------------------------------------------------
void vtkKWWidget::Grab()
{
  if (!this->IsCreated())
    {
    return;
    }

  this->Script("grab %s", this->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWWidget::ReleaseGrab()
{
  if (!this->IsCreated())
    {
    return;
    }

  this->Script("grab release %s", this->GetWidgetName());
}

//----------------------------------------------------------------------------
int vtkKWWidget::IsGrabbed()
{
  if (!this->IsCreated())
    {
    return 0;
    }

  const char *res = this->Script("grab status %s", this->GetWidgetName());
  return (!strcmp(res, "none") ? 0 : 1);
}

//----------------------------------------------------------------------------
int vtkKWWidget::SetConfigurationOption(
  const char *option, const char *value)
{
  if (!this->IsCreated())
    {
    vtkWarningMacro("Widget is not created yet !");
    return 0;
    }

  if (!option || !value)
    {
    vtkWarningMacro("Wrong option or value !");
    return 0;
    }

  const char *res = 
    this->Script("%s configure %s {%s}", this->GetWidgetName(), option, value);

  // 'configure' is not supposed to return anything, so let's assume
  // any output is an error

  if (res && *res)
    {
    vtksys_stl::string err_msg(res);
    vtksys_stl::string tcl_name(this->GetTclName());
    vtksys_stl::string widget_name(this->GetWidgetName());
    vtksys_stl::string type(this->GetType());
    vtkErrorMacro(
      "Error configuring " << tcl_name.c_str() << " (" << type.c_str() << ": " 
      << widget_name.c_str() << ") with option: [" << option 
      << "] and value [" << value << "] => " << err_msg.c_str());
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWWidget::SetConfigurationOptionAsInt(
  const char *option, int value)
{
  char buffer[20];
  sprintf(buffer, "%d", value);
  return this->SetConfigurationOption(option, buffer);
}

//----------------------------------------------------------------------------
int vtkKWWidget::TakeScreenDump(const char* fname,
                                int top, int bottom, int left, int right)
{
  return vtkKWTkUtilities::TakeScreenDump(
    this, fname, top, bottom, left, right);
}

//----------------------------------------------------------------------------
void vtkKWWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "BalloonHelpString: " 
     << (this->BalloonHelpString ? this->BalloonHelpString : "None") << endl;
  os << indent << "Parent: " << this->GetParent() << endl;
  os << indent << "Enabled: " << (this->GetEnabled() ? "On" : "Off") << endl;
  os << indent << "DragAndDropTargetSet: ";
  if (this->DragAndDropTargetSet)
    {
    os << this->DragAndDropTargetSet << endl;
    }
  else
    {
    os << "None" << endl;
    }
  os << indent << "WidgetName: ";
  if (this->WidgetName)
    {
    os << this->WidgetName << endl;
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "IsCreated: " << (this->IsCreated() ? "Yes" : "No") << endl;
}

