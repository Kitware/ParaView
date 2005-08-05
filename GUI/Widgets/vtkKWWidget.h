/*=========================================================================

  Module:    vtkKWWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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

class vtkKWWindowBase;
class vtkKWDragAndDropTargetSet;
class vtkKWWidgetInternals;

class KWWIDGETS_EXPORT vtkKWWidget : public vtkKWObject
{
public:
  static vtkKWWidget* New();
  vtkTypeRevisionMacro(vtkKWWidget,vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the parent widget for this widget
  virtual void SetParent(vtkKWWidget *p);
  vtkGetObjectMacro(Parent, vtkKWWidget);

  // Description:
  // Create the widget.
  // The parent should be set before calling this method.
  // Subclasses should implement a Create() method with the same signature.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Get the name of the underlying Tk widget being used.
  // The Create() method should be called before invoking this method.
  // Note that setting the widget name manually is *not* recommended ; use
  // it only if you know what you are doing, say, for example, if
  // you have to map an external Tk widget to a vtkKWWidget object.
  virtual const char *GetWidgetName();
  vtkSetStringMacro(WidgetName);

  // Description:
  // Query if the widget was created successfully.
  virtual int IsCreated();

  // Description:
  // Query if the widget is "alive" (i.e. IsCreated()) and has not been 
  // deleted as far as Tk is concerned.
  virtual int IsAlive();
  
  // Description:
  // Query if the widget is mapped (i.e, on screen)
  virtual int IsMapped();
  
  // Description:
  // Set focus to this widget.
  virtual void Focus();

  // Description:
  // Set/Get the enabled state.
  virtual void SetEnabled(int);
  vtkBooleanMacro(Enabled, int);
  vtkGetMacro(Enabled, int);

  // Description:
  // Set/add/remove a binding to a widget. 
  // Whenever the 'event' is triggered on the widget, the 'method' is invoked
  // on the 'object' (or called like a regular command if 'object' is NULL)
  virtual void SetBinding(
    const char *event, vtkObject *object, const char *method);
  virtual void SetBinding(
    const char *event, const char *command);
  virtual void AddBinding(
    const char *event, vtkObject *object, const char *method);
  virtual void AddBinding(
    const char *event, const char *command);
  virtual void RemoveBinding(const char *event);

  // Description:
  // Convenience method to get the parent vtkKWWindowBase for
  // this Widget if there is one.
  // NOTE: this may return NULL if the Widget is not in a window.
  vtkKWWindowBase* GetParentWindow();

  // Description:
  // Query if widget is packed
  virtual int IsPacked();
  virtual int GetNumberOfPackedChildren();

  // Description:
  // Unpack widget, unpack siblings (slave's of parent widget), unpack children
  virtual void Unpack();
  virtual void UnpackSiblings();
  virtual void UnpackChildren();
  
  // Description:
  // Setting this string enables balloon help for this widget.
  virtual void SetBalloonHelpString(const char *str);
  vtkGetStringMacro(BalloonHelpString);

  // Description:
  // Query if there are drag and drop targets between this widget and
  // other widgets. Get the targets.
  // IMPORTANT: the vtkKWDragAndDropTargetSet object is lazy-allocated, i.e.
  // allocated only when it is needed, as GetDragAndDropTargetSet() is called.
  // Therefore, to check if the instance *has* drag and drop targets, use 
  // HasDragAndDropTargetSet(), not GetDragAndDropTargetSet().
  virtual int HasDragAndDropTargetSet();
  virtual vtkKWDragAndDropTargetSet* GetDragAndDropTargetSet();

  // Description:
  // Grab the widget (locally)
  virtual void Grab();
  virtual void ReleaseGrab();
  virtual int IsGrabbed();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description::
  // Override Unregister since widgets have loops.
  virtual void UnRegister(vtkObjectBase *o);

  // Description:
  // Get the net reference count of this widget. That is the
  // reference count of this widget minus its children.
  virtual int  GetNetReferenceCount();

  // Description:
  // Query children from this widget
  virtual int HasChild(vtkKWWidget *w);
  virtual int GetNumberOfChildren();
  virtual vtkKWWidget* GetNthChild(int rank);
  virtual vtkKWWidget* GetChildWidgetWithName(const char *);
  virtual void RemoveAllChildren();

  // Description:
  // Create a specific Tk widget of type 'type', with optional arguments 
  // 'args' and map it to this instance.
  // Use the Create() method to create this widget instance instead, do *not*
  // use the CreateSpecificTkWidget() method unless you are calling from
  // a subclass to implement a specific kind of Tk widget as a vtkKWWidget
  // subclass, or unless you have to map an external pure Tk widget into a
  // vtkKWWidget.
  // This method should be called by all subclasses to ensure that flags are
  // set correctly (typically from the subclass's Create() method).
  // If 'type' is NULL, this method will still perform some checkings and
  // set the proper flags indicating that the widget has been created. It will
  // then be up to the subclass to create the appropriate Tk widget after
  // calling the superclass's Create().
  // Please *do* refrain from using 'args' to pass arbitrary Tk
  // option settings, let the user call SetConfigurationOption() instead, 
  // or much better, create C++ methods as front-ends to those settings. 
  // For example, the SetBackgroundColor() method can/should be used to set
  // the corresponding -bg Tk option. 
  // Ideally, the 'args' parameter should only be used to specify options that
  // can *not* be changed using Tk's 'configure' 
  // (i.e. SetConfigurationOptions()), and therefore that have to be passed
  // at widget's creation time. For example the -visual and -class options 
  // of the 'toplevel' widget.
  // Return 1 on success, 0 otherwise.
  virtual int CreateSpecificTkWidget(
    vtkKWApplication *app, const char *type, const char *args = NULL);

protected:
  vtkKWWidget();
  ~vtkKWWidget();

  // Description:
  // Add/Remove a child to/from this widget
  virtual void AddChild(vtkKWWidget *w);
  virtual void RemoveChild(vtkKWWidget *w);

  // Description:
  // The name of the underlying Tk widget being used.
  char *WidgetName;

  // Description:
  // The parent of the widget
  vtkKWWidget *Parent;

  // Ballon help
  // The tooltip associated to the widget
  char *BalloonHelpString;
  
  // Description:
  // PIMPL Encapsulation for STL containers
  vtkKWWidgetInternals *Internals;

  // Description:
  // Convenience function to that propagates the Enabled state of
  // the instance to another subwidget (preferably a sub-widget).
  // It calls SetEnabled(this->GetEnabled()) on the 'widget' parameter
  virtual void PropagateEnableState(vtkKWWidget* widget);

private:
  
  // Description:
  // The Drag & Drop targets, if any. In private: so that it can be
  // lazy-created
  vtkKWDragAndDropTargetSet* DragAndDropTargetSet;

  int WidgetIsCreated;
  int Enabled;

  vtkKWWidget(const vtkKWWidget&); // Not implemented
  void operator=(const vtkKWWidget&); // Not implemented
};

#endif
