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
#include "vtkKWApplication.h"
#include "vtkKWWidget.h"
#include "vtkObjectFactory.h"
#include "vtkKWWindow.h"


//------------------------------------------------------------------------------
vtkKWWidget* vtkKWWidget::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWWidget");
  if(ret)
    {
    return (vtkKWWidget*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWWidget;
}




int vtkKWWidgetCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);

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

  this->TraceInitialized = 0;
  this->TraceName = NULL;
}

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
  this->Script("%s %s %s",name,this->GetWidgetName(),args);
}

void vtkKWWidget::SetUpBalloonHelpBindings()
{
  this->Script("bind %s <Enter> {+%s BalloonHelpTrigger %s}", 
               this->GetWidgetName(), this->Application->GetTclName(),
               this->GetTclName());
  this->Script("bind %s <ButtonPress> {+%s BalloonHelpCancel}", 
               this->GetWidgetName(), this->Application->GetTclName());
  this->Script("bind %s <KeyPress> {+%s BalloonHelpCancel}", 
               this->GetWidgetName(), this->Application->GetTclName());
  this->Script("bind %s <Leave> {+%s BalloonHelpCancel}", 
               this->GetWidgetName(), this->Application->GetTclName());
  this->Script("bind %s <B1-Motion> {+%s BalloonHelpWithdraw}", 
               this->GetWidgetName(), this->Application->GetTclName());
}


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

// Removing items in the middle of a traversal is a bad thing.
// UnRegister will handle removing all of the children.
void vtkKWWidget::RemoveChild(vtkKWWidget *w) 
{
  if ( ! this->DeletingChildren)
    {
    this->Children->RemoveItem(w);
    }
}


void vtkKWWidget::UnRegister(vtkObject *o)
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

void vtkKWWidget::Focus()
{
  this->Script( "focus %s", this->GetWidgetName() );
}

void vtkKWWidget::SetBind(vtkKWObject* CalledObject, const char *Event, const char *CommandString)
{
  this->Application->Script("bind %s %s { %s %s }", this->GetWidgetName(), 
			    Event, CalledObject->GetTclName(), CommandString);
}

void vtkKWWidget::SetBind(const char *Event, const char *CommandString)
{
  this->Application->Script("bind %s %s { %s }", this->GetWidgetName(), 
			    Event, CommandString);
}

void vtkKWWidget::SetBind(const char *event, const char *widget, const char *command)
{
  this->Application->Script("bind %s %s { %s %s }", this->GetWidgetName(), 
			    event, widget, command);
}

void vtkKWWidget::SetBindAll(const char *event, const char *widget, const char *command)
{
  this->Application->Script("bind all %s { %s %s }", 
			    event, widget, command);
}

void vtkKWWidget::SetBindAll(const char *event, const char *command)
{
  this->Application->Script("bind all %s { %s }", 
			    event, command);
}

void vtkKWWidget::SetCommand(vtkKWObject* CalledObject, const char * CommandString)
{
  char* command = this->CreateCommand(CalledObject, CommandString);
  this->Application->SimpleScript(command);
  delete [] command;
}

char* vtkKWWidget::CreateCommand(vtkKWObject* CalledObject, const char * CommandString)
{
  ostrstream event;
  event << this->GetWidgetName() << " configure -command {" 
	<< CalledObject->GetTclName() 
	<< " " << CommandString << "} " << ends;

  return event.str();
}

void vtkKWWidget::SetBalloonHelpString(const char *str)
{
  if (this->Application == NULL)
    {
    vtkErrorMacro("Application needs to be set before balloon help.");
    return;
    }

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
  
  if ( !this->BalloonHelpInitialized )
    {
    this->SetUpBalloonHelpBindings();
    this->BalloonHelpInitialized = 1;
    }
}

void vtkKWWidget::SerializeRevision(ostream& os, vtkIndent indent)
{
  vtkKWObject::SerializeRevision(os,indent);
  os << indent << "vtkKWWidget ";
  this->ExtractRevision(os,"$Revision: 1.22 $");
}

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
       

int vtkKWWidget::InitializeTrace()
{
  if (this->TraceInitialized)
    {
    return 1;
    }

  if (this->Parent == NULL || this->TraceName == NULL)
    {
    return 0;
    }
  if ( ! this->Parent->InitializeTrace())
    {
    return 0;
    }
    
  // Set the variable becasue AddTraceEntry also calls InitializeTrace.
  this->TraceInitialized = 1;
  this->AddTraceEntry("set $kw($s) [$kw(%s) GetChildWidget %s]",
                      this->GetTclName(), this->Parent->GetTclName(),
                      this->TraceName);
  return 1;
}  

int vtkKWWidget::AddTraceEntry(const char *format, ...)
{
  ostream *os;

  if (this->Application == NULL || this->InitializeTrace() == 0)
    {
    return 0;
    }
  os = this->Application->GetTraceFile();
  if (os == NULL)
    {
    return 0;
    }

  char event[6000];

  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  *os << event << endl;

  return 1;
}

