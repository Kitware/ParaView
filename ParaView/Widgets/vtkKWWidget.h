/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWWidget.h
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
// .NAME vtkKWWidget - superclass of KW widgets
// .SECTION Description
// This class is the superclass of all UI based objects in the
// Kitware toolkit. It contains common methods such as specifying
// the parent widget, generating and returning the Tcl widget name
// for an instance, and managing children. It overrides the 
// Unregister method to handle circular reference counts between
// child and parent widgets.

#ifndef __vtkKWWidget_h
#define __vtkKWWidget_h

#include "vtkKWObject.h"
#include "vtkKWWidgetCollection.h"
class vtkKWWindow;

class VTK_EXPORT vtkKWWidget : public vtkKWObject
{
public:
  static vtkKWWidget* New();
  vtkTypeMacro(vtkKWWidget,vtkKWObject);

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app,const char *name, const char *args);

  // Description:
  // Get the name of the underlying tk widget being used
  // the parent should be set before calling this method.
  const char *GetWidgetName();

  // Description:
  // Set/Get the parent widget for this widget
  void SetParent(vtkKWWidget *p);
  vtkGetObjectMacro(Parent,vtkKWWidget);

  // Description:
  // Add/Remove/Get a child to this Widget
  void AddChild(vtkKWWidget *w) {this->Children->AddItem(w);};
  void RemoveChild(vtkKWWidget *w);
  vtkKWWidgetCollection *GetChildren() {return this->Children;};
  
  // Description::
  // Override Unregister since widgets have loops.
  void UnRegister(vtkObject *o);

  // Description:
  // Get the net reference count of this widget. That is the
  // reference count of this widget minus its children.
  virtual int  GetNetReferenceCount();
  // Description:
  // A method to set callback functions on objects.  The first argument is
  // the KWObject that will have the method called on it.  The second is the
  // name of the method to be called and any arguments in string form.
  // The calling is done via TCL wrappers for the KWObject.
  virtual void SetCommand( vtkKWObject* Object, const char* MethodAndArgString);
  
  // Description:
  // A method to set binding on the object.
  // This method sets binding:
  // bind this->GetWidgetName() event { object->GetTclName() command }
  void SetBind(vtkKWObject* object, const char *event, const char *command);

  // Description:
  // A method to set binding on the object.
  // This method sets binding:
  // bind this->GetWidgetName() event { command }  
  void SetBind(const char *event, const char *command);

  // Description:
  // A method to set binding on the object.
  // This method sets binding:
  // bind this->GetWidgetName() event { widget command }  
  void SetBind(const char *event, const char *widget, const char *command);

  // Description:
  // Set focus to this widget.
  void Focus();

  // Description: a method to create a callback string from a KWObject.
  // The caller is resposible for deleting the returned string.  
  char* CreateCommand(vtkKWObject* Object, const char* MethodAndArgString);

  // Description:
  // Setting this string enables balloon help for this widget.
  virtual void SetBalloonHelpString(const char *str);
  vtkGetStringMacro(BalloonHelpString);
  
  // Description:
  // Adjusts the placement of the baloon help
  vtkSetMacro(BalloonHelpJustification,int);
  vtkGetMacro(BalloonHelpJustification,int);  
  void SetBalloonHelpJustificationToLeft(){
    this->SetBalloonHelpJustification(0);};
  void SetBalloonHelpJustificationToRight(){
    this->SetBalloonHelpJustification(2);};
  
  // Description:
  // Get the containing vtkKWWindow for this Widget if there is one.
  // NOTE: this may return NULL if the Widget is not in a window.
  vtkKWWindow* GetWindow();
protected:
  vtkKWWidget();
  ~vtkKWWidget();
  vtkKWWidget(const vtkKWWidget&) {};
  void operator=(const vtkKWWidget&) {};
  virtual void SerializeRevision(ostream& os, vtkIndent indent);

  char *WidgetName;
  vtkKWWidget *Parent;
  vtkKWWidgetCollection *Children; 
  int DeletingChildren;

  char  *BalloonHelpString;
  int   BalloonHelpJustification;
  int   BalloonHelpInitialized;
  void  SetUpBalloonHelpBindings();
  
};


#endif


