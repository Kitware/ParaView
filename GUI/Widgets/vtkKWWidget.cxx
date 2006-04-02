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
#include "vtkKWTopLevel.h"
#include "vtkKWDragAndDropTargetSet.h"
#include "vtkKWBalloonHelpManager.h"
#include "vtkObjectFactory.h"
#include "vtkKWIcon.h"

#include <vtksys/stl/vector>
#include <vtksys/stl/algorithm>
#include <vtksys/SystemTools.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWWidget );
vtkCxxRevisionMacro(vtkKWWidget, "1.144");

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
  this->BalloonHelpIcon          = NULL;  
  this->BalloonHelpManager       = NULL;

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

  if (this->BalloonHelpManager )
    {
    this->SetBalloonHelpManager(NULL);
    }

  if (this->BalloonHelpString)
    {
    this->SetBalloonHelpString(NULL);
    }

  if (this->BalloonHelpIcon)
    {
    this->SetBalloonHelpIcon(NULL);
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
  if (this->Parent && p && this->IsCreated())
    {
    vtkErrorMacro("Error attempt to reparent a widget that has been created!");
    return;
    }

  if (this->Parent)
    {
    vtkKWWidget *tmp = this->Parent;
    this->Parent = NULL;
    tmp->UnRegister(this);
    tmp->RemoveChild(this);
    }

  if (p)
    {
    this->Parent = p;
    p->Register(this);
    p->AddChild(this);
    }
}

//----------------------------------------------------------------------------
vtkKWApplication* vtkKWWidget::GetApplication()
{
  if (!this->Superclass::GetApplication() &&
      this->Parent && this->Parent->GetApplication())
    {
    this->SetApplication(this->Parent->GetApplication());
    }

  return this->Superclass::GetApplication();
}

//----------------------------------------------------------------------------
const char *vtkKWWidget::GetWidgetName()
{
  static unsigned long count = 0;

  // Is the name is already set the just return it

  if (this->WidgetName)
    {
    return this->WidgetName;
    }

  // Create this widgets name

  char local[256];

  if (this->Parent)
    {
    const char *tmp = this->Parent->GetWidgetName();
    sprintf(local, "%s.%lu", tmp, count);
    }
  else
    {
    sprintf(local, ".%lu", count);
    }
  count++;

  this->WidgetName = new char [strlen(local) + 1];
  strcpy(this->WidgetName, local);

  return this->WidgetName;
}

//----------------------------------------------------------------------------
void vtkKWWidget::Create()
{
  this->CreateSpecificTkWidget(NULL, NULL);
}

//----------------------------------------------------------------------------
int vtkKWWidget::CreateSpecificTkWidget(const char *type, 
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

  vtkKWApplication *app = this->GetApplication();
  if (!app)
    {
    vtkErrorMacro("Can not create widget if its application attribute was not set. Make sure that you called the SetApplication method on this widget, or that you set its parent to a widget which application attribute is set already.");
    return 0;
    }

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

    this->AddBalloonHelpBindings();
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

  this->AddBalloonHelpBindings();
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetBalloonHelpIcon(vtkKWIcon *arg)
{
  if (this->BalloonHelpIcon == arg)
    {
    return;
    }

  if (this->BalloonHelpIcon)
    {
    this->BalloonHelpIcon->UnRegister(this);
    }
    
  this->BalloonHelpIcon = arg;

  if (this->BalloonHelpIcon)
    {
    this->BalloonHelpIcon->Register(this);
    }

  this->Modified();

  this->AddBalloonHelpBindings();
}

//----------------------------------------------------------------------------
vtkKWBalloonHelpManager* vtkKWWidget::GetBalloonHelpManager()
{
  if (this->BalloonHelpManager)
    {
    return this->BalloonHelpManager;
    }

  if (this->GetApplication())
    {
    return this->GetApplication()->GetBalloonHelpManager();
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetBalloonHelpManager(vtkKWBalloonHelpManager *arg)
{
  if (this->BalloonHelpManager == arg)
    {
    return;
    }

  if (this->BalloonHelpManager)
    {
    this->BalloonHelpManager->RemoveBindings(this);
    this->BalloonHelpManager->UnRegister(this);
    }
    
  this->BalloonHelpManager = arg;

  if (this->BalloonHelpManager)
    {
    this->BalloonHelpManager->Register(this);
    this->AddBalloonHelpBindings();
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWWidget::AddBalloonHelpBindings()
{
  if (this->IsCreated() && (this->BalloonHelpString || this->BalloonHelpIcon))
    {
    vtkKWBalloonHelpManager *mgr = this->GetBalloonHelpManager();
    if (mgr)
      {
      mgr->AddBindings(this);
      }
    }
}

//----------------------------------------------------------------------------
vtkKWTopLevel* vtkKWWidget::GetParentTopLevel()
{
  vtkKWTopLevel* toplevel =0;
  vtkKWWidget* widget = this->GetParent();
  while(widget)
    {
    if((toplevel = vtkKWTopLevel::SafeDownCast(widget)))
      {
      return toplevel;
      }
    widget = widget->GetParent();
    }
  return toplevel;
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
  if (this->IsCreated())
    {
    this->Script("catch {eval pack forget %s}", this->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWWidget::UnpackSiblings()
{
  if (this->GetParent() && this->GetParent()->IsCreated())
    {
    this->GetParent()->Script("catch {eval pack forget [pack slaves %s]} \n "
                              "catch {eval grid forget [grid slaves %s]}",
                              this->GetParent()->GetWidgetName(),
                              this->GetParent()->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWWidget::UnpackChildren()
{
  if (this->IsCreated())
    {
    this->Script("catch {eval pack forget [pack slaves %s]} \n "
                 "catch {eval grid forget [grid slaves %s]}",
                 this->GetWidgetName(),this->GetWidgetName());
    }
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
void vtkKWWidget::SetBinding(const char *event, 
                             vtkObject *object, const char *method)
{
  if (this->IsCreated())
    {
    char *command = NULL;
    this->SetObjectMethodCommand(&command, object, method);
    this->Script("bind %s %s {%s}", this->GetWidgetName(), event, command);
    delete [] command;
    }
}

//----------------------------------------------------------------------------
void vtkKWWidget::SetBinding(const char *event, const char *command)
{
  this->SetBinding(event, NULL, command);
}

//----------------------------------------------------------------------------
const char* vtkKWWidget::GetBinding(const char *event)
{
  if (this->IsCreated())
    {
    return this->Script("bind %s %s", this->GetWidgetName(), event);
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWWidget::AddBinding(const char *event, 
                             vtkObject *object, const char *method)
{
  if (this->IsCreated())
    {
    char *command = NULL;
    this->SetObjectMethodCommand(&command, object, method);
    this->Script("bind %s %s {+%s}", this->GetWidgetName(), event, command);
    delete [] command;
    }
}

//----------------------------------------------------------------------------
void vtkKWWidget::AddBinding(const char *event, const char *command)
{
  this->AddBinding(event, NULL, command);
}

//----------------------------------------------------------------------------
void vtkKWWidget::RemoveBinding(const char *event, 
                                vtkObject *object, const char *method)
{
  if (this->IsCreated())
    {
    char *command = NULL;
    this->SetObjectMethodCommand(&command, object, method);

    // Retrieve the bindings, remove the command, re-assign

    vtksys_stl::string bindings(
      this->Script("bind %s %s", this->GetWidgetName(), event));

    vtksys::SystemTools::ReplaceString(bindings, command, "");
  
    this->Script(
      "bind %s %s {%s}", this->GetWidgetName(), event, bindings.c_str());
    delete [] command;
    }
}

//----------------------------------------------------------------------------
void vtkKWWidget::RemoveBinding(const char *event)
{
  if (this->IsCreated())
    {
    this->Script("bind %s %s {}", this->GetWidgetName(), event);
    }
}

//----------------------------------------------------------------------------
void vtkKWWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "BalloonHelpString: " 
     << (this->BalloonHelpString ? this->BalloonHelpString : "None") << endl;
  os << indent << "Parent: " << this->GetParent() << endl;
  os << indent << "Enabled: " << (this->GetEnabled() ? "On" : "Off") << endl;

  os << indent << "BalloonHelpIcon: ";
  if (this->BalloonHelpIcon)
    {
    os << this->BalloonHelpIcon << endl;
    }
  else
    {
    os << "None" << endl;
    }


  os << indent << "BalloonHelpManager: ";
  if (this->BalloonHelpManager)
    {
    os << this->BalloonHelpManager << endl;
    }
  else
    {
    os << "None" << endl;
    }

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

