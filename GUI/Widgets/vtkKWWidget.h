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

class vtkKWIcon;
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
  // Create the corresponding Tk widget.
  // This should be called by all subclasses to ensure that flags are set
  // correctly.
  // If 'type' is NULL, this method will only set the proper flags indicating
  // that the widget has been created and perform some checkings. It will be
  // up to the subclass to create the appropriate Tk widget after calling
  // the superclass's Create().
  // Return 1 on success, 0 otherwise.
  int Create(vtkKWApplication *app, const char *type, const char *args);
  int IsCreated();

  // Description:
  // Set tk configuration options of the widget
  void ConfigureOptions(const char* opts);

  // Description:
  // Query if widget is "alive" (i.e. is created and not deleted as far as
  // Tk is concered, i.e. 'winfo exists')
  int IsAlive();
  
  // Description:
  // Query if widget is mapped (on screen)
  int IsMapped();
  
  // Description:
  // Get the name of the underlying Tk widget being used.
  // The parent should be set before calling this method, as it uses
  // the parent's widget name and appends a number.
  // Note that setting the widget name manually is *not* recommended, use
  // this method only if you know what you are doing, say, for example, if
  // you have to map a external pure Tk widget to a vtkKWWidget object.
  const char *GetWidgetName();
  vtkSetStringMacro(WidgetName);

  // Description:
  // Set/Get the parent widget for this widget
  void SetParent(vtkKWWidget *p);
  vtkGetObjectMacro(Parent, vtkKWWidget);

  // Description:
  // Add/Remove/Get a child to this Widget
  virtual void AddChild(vtkKWWidget *w);
  virtual void RemoveChild(vtkKWWidget *w);
  virtual int HasChild(vtkKWWidget *w);
  virtual void RemoveAllChildren();
  virtual int GetNumberOfChildren();
  virtual vtkKWWidget* GetNthChild(int rank);
  virtual vtkKWWidget* GetChildWidgetWithName(const char *);

  // Description::
  // Override Unregister since widgets have loops.
  virtual void UnRegister(vtkObjectBase *o);

  // Description:
  // Get the net reference count of this widget. That is the
  // reference count of this widget minus its children.
  virtual int  GetNetReferenceCount();

  // Description:
  // A method to set callback functions on objects.  The first argument is
  // the KWObject that will have the method called on it.  The second is the
  // name of the method to be called and any arguments in string form.
  // The calling is done via TCL wrappers for the KWObject.
  virtual void SetCommand(vtkKWObject* Object, const char* MethodAndArgString);

  // Description:
  // Get the Tk string type of a widget.
  virtual const char* GetType();
  
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
  // A method to set binding on the object.
  // This method sets binding:
  // bind all event { widget command }  
  void SetBindAll(const char *event, const char *widget, const char *command);

  // Description:
  // A method to set binding on the object.
  // This method sets binding:
  // bind all event { command }  
  void SetBindAll(const char *event, const char *command);

  // Description:
  // This method unsets the bind for specific event.
  void UnsetBind(const char *event);

  // Description:
  // Set or get enabled state.
  virtual void SetEnabled(int);
  vtkBooleanMacro(Enabled, int);
  vtkGetMacro(Enabled, int);

  // Description:
  // Set focus to this widget.
  void Focus();

  // Description:
  // Get the containing vtkKWWindowBase for this Widget if there is one.
  // NOTE: this may return NULL if the Widget is not in a window.
  vtkKWWindowBase* GetWindow();

  // Description:
  // Convenience method to Set/Get the current background and foreground colors
  // of the widget (either using 0 -> 255 int, or normalized 0.0 -> 1.0 float).
  virtual void GetBackgroundColor(int *r, int *g, int *b);
  virtual void SetBackgroundColor(int r, int g, int b);
  virtual void GetBackgroundColor(double *r, double *g, double *b);
  virtual void SetBackgroundColor(double r, double g, double b);
  virtual void GetForegroundColor(int *r, int *g, int *b);
  virtual void SetForegroundColor(int r, int g, int b);
  virtual void GetForegroundColor(double *r, double *g, double *b);
  virtual void SetForegroundColor(double r, double g, double b);
  
  // Description:
  // Query if widget has a given Tk configuration option (ex: "-state"), 
  // and get the option as int
  int HasConfigurationOption(const char* option);
  int GetConfigurationOptionAsInt(const char* option);

  // Description:
  // Set/Get the textual value of a Tk option (defaut is -text option) given a
  // pointer to a string.
  // The characted encoding used in the string will be retrieved by querying
  // the widget's application CharacterEncoding ivar. Conversion from that
  // encoding to Tk internal encoding will be performed automatically.
  //BTX
  void SetTextOption(const char *text, const char *option = "-text");
  const char* GetTextOption(const char *option = "-text");
  //ETX

  // Description:
  // Convenience method to Set/Get the -state option to "normal" (if true) or
  // "disabled" (if false).
  void SetStateOption(int flag);
  int GetStateOption();

  // Description:
  // Query if widget is packed
  int IsPacked();
  int GetNumberOfPackedChildren();

  // Description:
  // Unpack widget, unpack siblings (slave's of parent widget), unpack children
  void Unpack();
  void UnpackSiblings();
  void UnpackChildren();
  
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
  // Some constant that can be used to specify anchoring
  //BTX
  enum
  {
    ANCHOR_N      = 0,
    ANCHOR_NE     = 1,
    ANCHOR_E      = 2,
    ANCHOR_SE     = 3,
    ANCHOR_S      = 4,
    ANCHOR_SW     = 5,
    ANCHOR_W      = 6,
    ANCHOR_NW     = 7,
    ANCHOR_CENTER = 8
  };
  //ETX
  static const char* GetAnchorAsString(int);

  // Description:
  // Set image option using either icon, predefined icon index (see 
  // vtkKWIcon.h) or pixel data (pixels and the structure of the
  // image, i.e. width, height, pixel_size ; if buffer_length = 0, it
  // is computed using width * height * pixel_size, otherwise used as 
  // a hint whereas the data is in Base64 / Zlib format).
  // If RGBA (pixel_size > 3), blend pixels with background color of
  // the widget (otherwise 0.5, 0.5, 0.5 gray). If blend_color_option is not
  // NULL,  use this option as blend color instead of background (-bg) 
  // (ex: -fg, -selectcolor)
  // An image is created and associated to the Tk -image option, 
  // or image_option if not NULL (ex: -selectimage).
  virtual void SetImageOption(int icon_index,
                              const char *blend_color_option = 0,
                              const char *image_option = 0);
  virtual void SetImageOption(vtkKWIcon *icon,
                              const char *blend_color_option = 0,
                              const char *image_option = 0);
  virtual void SetImageOption(const unsigned char *data, 
                              int width, int height, int pixel_size = 4,
                              unsigned long buffer_length = 0,
                              const char *blend_color_option = 0,
                              const char *image_option = 0);
  virtual void SetImageOption(const char *image_name,
                              const char *image_option = 0);
  virtual const char* GetImageOption(const char *image_option = 0);
  
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

  // Description:
  // Take screendump of the widget and store it into the png file.
  int TakeScreenDump(const char *fname,
                     int top=0, int bottom=0, int left=0, int right=0);

protected:
  vtkKWWidget();
  ~vtkKWWidget();

  char        *WidgetName;

  vtkKWWidget *Parent;

  // Ballon help

  char  *BalloonHelpString;
  
  // Encoding methods
  static const char* GetTclCharacterEncodingAsString(int);

  // Description:
  // Convert a Tcl string (stored internally as UTF-8/Unicode) to another
  // internal format (given the widget's application CharacterEncoding), 
  // and vice-versa.
  // If no_curly_braces is true, curly braces will be removed from the
  // string, so that the resulting string can be used to set an option
  // using the usual {%s} syntax.

  const char* ConvertTclStringToInternalString(
    const char *str, int no_curly_braces = 1);
  const char* ConvertInternalStringToTclString(
    const char *str, int no_curly_braces = 1);

  virtual void PropagateEnableState(vtkKWWidget* widget);

  // PIMPL Encapsulation for STL containers

  vtkKWWidgetInternals *Internals;

private:
  
  vtkKWDragAndDropTargetSet* DragAndDropTargetSet;

  int WidgetIsCreated;
  int Enabled;

  vtkKWWidget(const vtkKWWidget&); // Not implemented
  void operator=(const vtkKWWidget&); // Not implemented
};

#endif
