/*=========================================================================

  Module:    vtkKWObject.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWObject - Superclass that supports basic Tcl functionality
// .SECTION Description
// vtkKWObject is the superclass for most application classes.
// It is a direct subclass of vtkObject but adds functionality for 
// invoking Tcl scripts, obtaining the Tcl name for an instance, etc. 
// This class requires a vtkKWApplication in order to work (as do all classes).
// .SECTION See Also
// vtkKWApplication

#ifndef __vtkKWObject_h
#define __vtkKWObject_h

#include "vtkObject.h"

#include "vtkTcl.h" // Needed for Tcl interpreter
#include "vtkKWWidgets.h" // Needed for export symbols directives

class vtkKWApplication;
class vtkCallbackCommand;

class KWWidgets_EXPORT vtkKWObject : public vtkObject
{
public:
  static vtkKWObject* New();
  vtkTypeRevisionMacro(vtkKWObject,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the name of the Tcl object this instance represents.
  const char *GetTclName();

  // Description:
  // Set/Get the application instance for this object.
  vtkGetObjectMacro(Application,vtkKWApplication);
  virtual void SetApplication (vtkKWApplication* arg);

  //BTX
  // Description:
  // Invoke some Tcl script code and perform argument substitution.
  virtual const char* Script(const char *EventString, ...);
  //ETX
  
  // Description:
  // Add/Remove a callback command observer. 
  // This AddCallbackCommandObserver() method makes sure the
  // CallbackCommand object is setup properly, then add an observer on
  // 'object', if it does not exist already. This observer is triggered by
  // 'event' and will invoke the CallbackCommand's Execute() method.
  // This method is prefered over the vtkObject::AddObserver method as
  // it takes care of initializing CallbackCommand, and eventually keep
  // track of observers that have been added, so that they can be removed
  // properly using RemoveCallbackCommandObserver(s).
  // Listeners (i.e. classes that add observers) can process the events
  // sent by the object/observer pairs by re-implementing the protected
  // ProcessCallbackCommandEvents virtual method.
  virtual void AddCallbackCommandObserver(
    vtkObject *object, unsigned long event);
  virtual void RemoveCallbackCommandObserver(
    vtkObject *object, unsigned long event);

  // Description:
  // Add all the default observers needed by that object, or remove
  // all the observers that were added through AddCallbackCommandObserver.
  // Subclasses can override these methods to add/remove their own default
  // observers, but should call the superclass too.
  virtual void AddCallbackCommandObservers() {};
  virtual void RemoveCallbackCommandObservers();

protected:
  vtkKWObject();
  ~vtkKWObject();

  // Description:
  // Convenience method that can be used to create a Tcl callback command.
  // The 'command' argument is a pointer to the command to be created.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // Note that 'command' is allocated automatically using the 'new' 
  // operator. If it is not NULL, it is deallocated first using 'delete []'.
  virtual void SetObjectMethodCommand(
    char **command, vtkObject *object, const char *method);

  // Description:
  // Convenience method to invoke a Tcl callback command that was created 
  // using the SetObjectMethodCommand method. This simply checks if the 
  // command is not empty and evaluate it from Tcl.
  virtual void InvokeObjectMethodCommand(const char *command);

  // Description:
  // Get the callback command. 
  // Its ClientData is set to this vtkKWObject instance
  // itself, do not change it. Its Execute() method calls the static
  // ProcessCallbackCommandEventsFunction method, passing it its ClientData,
  // which in turn is converted back to a vtkKWObject pointer. The virtual
  // ProcessCallbackCommandEvents method is invokved on that pointer.
  // Subclasses can override this method to set specific flags, like
  // the AbortFlagOnExecute flag.
  virtual vtkCallbackCommand* GetCallbackCommand();

  // Description:
  // Processes the events that are passed through CallbackCommand (or others).
  // Subclasses can override this method to process their own events, but
  // should call the superclass too.
  // Also check AddCallbackCommandObserver and RemoveCallbackCommandObserver.
  virtual void ProcessCallbackCommandEvents(
    vtkObject *caller, unsigned long event, void *calldata);

  // Description:
  // Static callback function that is invoked by the 
  // CallbackCommand's Execute() method. It converts its clientdata back to
  // a vtkKWObject pointer and invoke its virtual 
  // ProcessCallbackCommandEvents method. Subclass should *not* reimplement
  // this method, but the virtual ProcessCallbackCommandEvents instead.
  static void ProcessCallbackCommandEventsFunction(
    vtkObject *object, unsigned long event, void *clientdata, void *calldata);

private:

  vtkKWApplication *Application;
  char *TclName;
  
  // Description:
  // The callback command. In private, so that GetCallbackCommand() can be
  // used to lazy allocate it.
  vtkCallbackCommand *CallbackCommand;

  vtkKWObject(const vtkKWObject&); // Not implemented
  void operator=(const vtkKWObject&); // Not implemented
};

#endif

