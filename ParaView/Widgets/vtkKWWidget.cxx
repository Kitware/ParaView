/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWWidget.cxx
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
#include "vtkKWWidget.h"

#include "vtkKWApplication.h"
#include "vtkKWEvent.h"
#include "vtkKWWidgetCollection.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"
#include "vtkString.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWWidget );

int vtkKWWidgetCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);

//------------------------------------------------------------------------------
vtkKWWidget::vtkKWWidget()
{
  this->WidgetName               = NULL;
  this->Parent                   = NULL;
  this->CommandFunction          = vtkKWWidgetCommand;
  this->Children                 = vtkKWWidgetCollection::New();
  // Make tracking memory leaks easier.
  this->Children->Register(this);
  this->Children->Delete();
  this->DeletingChildren         = 0;
  this->BalloonHelpString        = NULL;  
  this->BalloonHelpJustification = 0;
  this->BalloonHelpInitialized   = 0;
  this->Enabled                  = 1;

  this->TraceName = NULL;
}

//------------------------------------------------------------------------------
vtkKWWidget::~vtkKWWidget()
{
  if (this->BalloonHelpString)
    {
    this->SetBalloonHelpString(NULL);
    }
  this->Children->UnRegister(this);
  this->Children = NULL;
  
  if (this->Application)
    {
    this->Script("destroy %s",this->GetWidgetName());
    }
  if (this->WidgetName)
    {
    delete [] this->WidgetName;
    }
  this->SetParent(NULL);
  this->SetApplication(NULL);
  this->SetTraceName(NULL);
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkKWWidget::Create(vtkKWApplication *app, const char *name, 
                         const char *args)
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("widget already created");
    return;
    }
  this->SetApplication(app);
  this->Script("%s %s %s",name,this->GetWidgetName(),(args?args:""));

  const char* type = this->GetType();
  if ( !vtkString::Equals(type, "Frame") && 
       !vtkString::Equals(type, "Menu")  && 
       !vtkString::Equals(type, "Label")  && 
       !vtkString::Equals(type, "Canvas") && 
       !vtkString::Equals(type, "Scrollbar") && 
       !vtkString::Equals(type, "Listbox") && 
       !vtkString::Equals(type, "Toplevel") )
    {
    this->Script("%s configure -state %s", 
                 this->GetWidgetName(),
                 (this->Enabled ? "normal" : "disabled"));
    }
}

//------------------------------------------------------------------------------
void vtkKWWidget::SetUpBalloonHelpBindings()
{
  this->Script("bind %s <Enter> {+%s BalloonHelpTrigger %s}", 
               this->GetWidgetName(), this->Application->GetTclName(),
               this->GetTclName());
  this->Script("bind %s <ButtonPress> {+%s BalloonHelpWithdraw}", 
               this->GetWidgetName(), this->Application->GetTclName());
  this->Script("bind %s <KeyPress> {+%s BalloonHelpWithdraw}", 
               this->GetWidgetName(), this->Application->GetTclName());
  this->Script("bind %s <Leave> {+%s BalloonHelpCancel}", 
               this->GetWidgetName(), this->Application->GetTclName());
  this->Script("bind %s <B1-Motion> {+%s BalloonHelpWithdraw}", 
               this->GetWidgetName(), this->Application->GetTclName());  
}

//------------------------------------------------------------------------------
int  vtkKWWidget::GetNetReferenceCount() 
{
  int childCounts = 0;
  vtkKWWidget *child;
  
  for (this->Children->InitTraversal(); 
       (child = this->Children->GetNextKWWidget());)
    {
    childCounts += child->GetNetReferenceCount();
    }
  return this->ReferenceCount + childCounts - 
    2*this->Children->GetNumberOfItems();
}

//------------------------------------------------------------------------------
// Removing items in the middle of a traversal is a bad thing.
// UnRegister will handle removing all of the children.
void vtkKWWidget::RemoveChild(vtkKWWidget *w) 
{
  if ( ! this->DeletingChildren)
    {
    this->Children->RemoveItem(w);
    }
}


//------------------------------------------------------------------------------
void vtkKWWidget::UnRegister(vtkObjectBase *o)
{
  if (!this->DeletingChildren)
    {
    // delete the children if we are about to be deleted
    if (this->ReferenceCount == this->Children->GetNumberOfItems() + 1)
      {
      vtkKWWidget *child;
  
      this->DeletingChildren = 1;
      this->Children->InitTraversal();
      while ((child = this->Children->GetNextKWWidget()))
        {
        child->SetParent(NULL);
        }
      this->Children->RemoveAllItems();
      this->DeletingChildren = 0;
      }
    }
  
  this->vtkObject::UnRegister(o);
}

//------------------------------------------------------------------------------
void vtkKWWidget::Focus()
{
  this->Script( "focus %s", this->GetWidgetName() );
}

//------------------------------------------------------------------------------
void vtkKWWidget::SetBind(vtkKWObject* CalledObject, const char *Event, const char *CommandString)
{
  this->Application->Script("bind %s %s { %s %s }", this->GetWidgetName(), 
                            Event, CalledObject->GetTclName(), CommandString);
}

//------------------------------------------------------------------------------
void vtkKWWidget::SetBind(const char *Event, const char *CommandString)
{
  this->Application->Script("bind %s %s { %s }", this->GetWidgetName(), 
                            Event, CommandString);
}

//------------------------------------------------------------------------------
void vtkKWWidget::SetBind(const char *event, const char *widget, const char *command)
{
  this->Application->Script("bind %s %s { %s %s }", this->GetWidgetName(), 
                            event, widget, command);
}

//------------------------------------------------------------------------------
void vtkKWWidget::SetBindAll(const char *event, const char *widget, const char *command)
{
  this->Application->Script("bind all %s { %s %s }", 
                            event, widget, command);
}

//------------------------------------------------------------------------------
void vtkKWWidget::SetBindAll(const char *event, const char *command)
{
  this->Application->Script("bind all %s { %s }", 
                            event, command);
}

//------------------------------------------------------------------------------
void vtkKWWidget::SetCommand(vtkKWObject* CalledObject, const char * CommandString)
{
  char* command = this->CreateCommand(CalledObject, CommandString);
  this->Application->SimpleScript(command);
  delete [] command;
}

//------------------------------------------------------------------------------
char* vtkKWWidget::CreateCommand(vtkKWObject* CalledObject, const char * CommandString)
{
  ostrstream event;
  event << this->GetWidgetName() << " configure -command {" 
        << CalledObject->GetTclName() 
        << " " << CommandString << "} " << ends;

  return event.str();
}

//------------------------------------------------------------------------------
void vtkKWWidget::SetBalloonHelpString(const char *str)
{
//    if (this->Application == NULL)
//      {
//      vtkErrorMacro("Application needs to be set before balloon help.");
//      return;
//      }

  // A little overkill.
  if (this->BalloonHelpString == NULL && str == NULL)
    {
    return;
    }

  // Normal string stuff.
  if (this->BalloonHelpString)
    {
    delete [] this->BalloonHelpString;
    this->BalloonHelpString = NULL;
    }
  if (str != NULL)
    {
    this->BalloonHelpString = new char[strlen(str)+1];
    strcpy(this->BalloonHelpString, str);
    }
  
  if ( this->BalloonHelpString && this->Application && 
       !this->BalloonHelpInitialized )
    {
    this->SetUpBalloonHelpBindings();
    this->BalloonHelpInitialized = 1;
    }
}

//------------------------------------------------------------------------------
void vtkKWWidget::SerializeRevision(ostream& os, vtkIndent indent)
{
  vtkKWObject::SerializeRevision(os,indent);
  os << indent << "vtkKWWidget ";
  this->ExtractRevision(os,"$Revision: 1.38 $");
}

//------------------------------------------------------------------------------
vtkKWWindow* vtkKWWidget::GetWindow()
{
  vtkKWWindow* win =0;
  vtkKWWidget* widget = this->GetParent();
  while(widget)
    {
    if((win = vtkKWWindow::SafeDownCast(widget)))
      {
      return win;
      }
    widget = widget->GetParent();
    }
  return win;
}



//----------------------------------------------------------------------------
// Methods for tracing.

vtkKWWidget *vtkKWWidget::GetChildWidget(const char *traceName)
{
  vtkKWWidget *child;

  this->Children->InitTraversal();
  while ( (child = this->Children->GetNextKWWidget()) )
    {
    if (child->GetTraceName())
      {
      if (strcmp(traceName, child->GetTraceName()) == 0)
        {
        return child;
        }
      }
    }
  return NULL;
}
       

//------------------------------------------------------------------------------
int vtkKWWidget::InitializeTrace()
{
  // There is no need to do anything if there is no trace file.
  if(this->Application == NULL || this->Application->GetTraceFile() == NULL)
    {
    return 0;
    }

  if (this->TraceInitialized)
    {
    return 1;
    }

  // The new general way to initialize objects (from vtkKWObject).
  if (this->TraceReferenceObject && this->TraceReferenceCommand)
    {
    if (this->TraceReferenceObject->InitializeTrace())
      {
      this->Application->AddTraceEntry("set kw(%s) [$kw(%s) %s]",
                                      this->GetTclName(), 
                                      this->TraceReferenceObject->GetTclName(),
                                      this->TraceReferenceCommand);
      this->TraceInitialized = 1;
      return 1;
      }
    }

  // The only other possibility is to have the 
  // parent initialize the childs trace.
  // This will only work if we have a parent and this child has a trace name.
  // The name is the way to get the children from the parent.
  if (this->Parent == NULL || this->TraceName == NULL)
    {
    return 0;
    }
  if ( ! this->Parent->InitializeTrace())
    {
    return 0;
    }

  // Default method for initializing varible form parent.
  // Only works if someone has given this widget a named.
  // The name must be different than all the parents other children.    
  this->Application->AddTraceEntry("set $kw(%s) [$kw(%s) GetChildWidget %s]",
                                  this->GetTclName(), this->Parent->GetTclName(),
                                  this->TraceName);
  this->TraceInitialized = 1;
  return 1;
}  

//------------------------------------------------------------------------------
void vtkKWWidget::GetRGBColor(const char* color,
                              int *rr, int *gg, int *bb)
{
  this->Script("winfo rgb %s %s",
               this->GetWidgetName(), color);
  int r, g, b;
  sscanf( this->Application->GetMainInterp()->result, "%d %d %d",
          &r, &g, &b );
  *rr = static_cast<int>((static_cast<float>(r) / 65535.0)*255.0);
  *gg = static_cast<int>((static_cast<float>(g) / 65535.0)*255.0);
  *bb = static_cast<int>((static_cast<float>(b) / 65535.0)*255.0); 
}

//------------------------------------------------------------------------------
const char* vtkKWWidget::GetType()
{
  if ( this->Application )
    {
    this->Script("winfo class %s", this->GetWidgetName());
    return this->Application->GetMainInterp()->result;
    }
  return "None";
}

//------------------------------------------------------------------------------
void vtkKWWidget::SetEnabled(int e)
{
  if ( this->Enabled == e )
    {
    return;
    }
  this->Enabled = e;

  if ( this->Application )
    {
    const char* type = this->GetType();

    if ( !vtkString::Equals(type, "Frame") && 
         !vtkString::Equals(type, "Menu")  && 
         !vtkString::Equals(type, "Canvas") && 
         !vtkString::Equals(type, "Label") && 
         !vtkString::Equals(type, "Scrollbar") && 
         !vtkString::Equals(type, "Listbox") && 
         !vtkString::Equals(type, "Toplevel"))
      {
      this->Script("%s configure -state %s", this->GetWidgetName(),
                   (this->Enabled ? "normal" : "disabled"));
      }
    }
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkKWWidget::GetBackgroundColor(int *r, int *g, int *b)
{
  ostrstream str;
  str << "lindex [ " << this->GetWidgetName() 
      << " configure -bg ] end" << ends;
  this->Script(str.str());
  this->GetRGBColor(this->Application->GetMainInterp()->result, r, g, b);
  str.rdbuf()->freeze(0);
}

//------------------------------------------------------------------------------
void vtkKWWidget::AddChild(vtkKWWidget *w) 
{
  this->Children->AddItem(w);
}

//----------------------------------------------------------------------------
void vtkKWWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "BalloonHelpJustification: " 
     << this->GetBalloonHelpJustification() << endl;
  os << indent << "BalloonHelpString: " 
     << (this->BalloonHelpString ? this->BalloonHelpString : "none") << endl;
  os << indent << "Children: " << this->GetChildren() << endl;
  os << indent << "Parent: " << this->GetParent() << endl;
  os << indent << "TraceName: " << (this->TraceName?this->TraceName:"none") 
     << endl;
  os << indent << "Enabled: " << (this->Enabled ? "on" : "off") << endl;
}
