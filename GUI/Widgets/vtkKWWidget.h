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
#include "vtkKWTkOptions.h" // For option constants

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
  // Set/Get the parent widget for this widget
  virtual void SetParent(vtkKWWidget *p);
  vtkGetObjectMacro(Parent, vtkKWWidget);

  // Description:
  // Create the widget.
  // The parent should be set before calling this method.
  // Subclasses should implement a Create() method.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Get the name of the underlying Tk widget being used.
  // The Create() method should be called before invoking this method.
  // Note that setting the widget name manually is *not* recommended ; use
  // it only if you know what you are doing, say, for example, if
  // you have to map an external pure Tk widget to a vtkKWWidget object.
  virtual const char *GetWidgetName();
  vtkSetStringMacro(WidgetName);

  // Description:
  // Query if the widget was created successfully.
  virtual int IsCreated();

  // Description:
  // Query if the widget is "alive" (i.e. IsCreate() and has not been deleted
  // as far as Tk is concerned)
  virtual int IsAlive();
  
  // Description:
  // Query if the widget is mapped (i.e, on screen)
  virtual int IsMapped();
  
  // Description:
  // Add/Remove/Get a child to/from this widget
  virtual void AddChild(vtkKWWidget *w);
  virtual void RemoveChild(vtkKWWidget *w);
  virtual int HasChild(vtkKWWidget *w);
  virtual void RemoveAllChildren();
  virtual int GetNumberOfChildren();
  virtual vtkKWWidget* GetNthChild(int rank);
  virtual vtkKWWidget* GetChildWidgetWithName(const char *);

  // Description:
  // A method to set callback functions on objects.  The first argument is
  // the KWObject that will have the method called on it.  The second is the
  // name of the method to be called and any arguments in string form.
  // The calling is done via TCL wrappers for the KWObject.
  virtual void SetCommand(vtkKWObject* Object, const char* MethodAndArgString);

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
  virtual void Focus();

  // Description:
  // Get the containing vtkKWWindowBase for this Widget if there is one.
  // NOTE: this may return NULL if the Widget is not in a window.
  vtkKWWindowBase* GetWindow();

  // Description:
  // Convenience method to Set/Get the current background and foreground colors
  // of the widget
  virtual void GetBackgroundColor(double *r, double *g, double *b);
  virtual double* GetBackgroundColor();
  virtual void SetBackgroundColor(double r, double g, double b);
  virtual void SetBackgroundColor(double rgb[3])
    { this->SetBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  virtual void GetForegroundColor(double *r, double *g, double *b);
  virtual double* GetForegroundColor();
  virtual void SetForegroundColor(double r, double g, double b);
  virtual void SetForegroundColor(double rgb[3])
    { this->SetForegroundColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get Tk configuration option (ex: "-state") 
  // Please make sure you check the class (and subclasses) API for
  // a C++ method acting as a front-end for the corresponding Tk option.
  // For example, the SetBackgroundColor() method can be used to set the 
  // corresponding -bg Tk option. 
  // SetConfigurationOption returns 1 on success, 0 otherwise.
  virtual int SetConfigurationOption(const char* option, const char *value);
  virtual int SetConfigurationOptionAsInt(const char* option, int value);
  virtual int HasConfigurationOption(const char* option);
  virtual const char* GetConfigurationOption(const char* option);
  virtual int GetConfigurationOptionAsInt(const char* option);

  // Description:
  // Set/Get the textual value of a Tk option (defaut is -text option) given a
  // pointer to a string.
  // The characted encoding used in the string will be retrieved by querying
  // the widget's application CharacterEncoding ivar. Conversion from that
  // encoding to Tk internal encoding will be performed automatically.
  //BTX
  virtual void SetTextOption(const char *text, const char *option = "-text");
  virtual const char* GetTextOption(const char *option = "-text");
  //ETX

  // Description:
  // Convenience method to Set/Get the -state option to "normal" (if true) or
  // "disabled" (if false).
  virtual void SetStateOption(int flag);
  virtual int GetStateOption();

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
  // Set/Get the anchoring.
  // Specifies how the information in a widget (e.g. text or a bitmap) is to
  // be displayed in the widget.
  // Valid constants can be found in vtkKWTkOptions::AnchorType.
  virtual void SetAnchor(int);
  virtual int GetAnchor();
  virtual void SetAnchorToNorth() 
    { this->SetAnchor(vtkKWTkOptions::AnchorNorth); };
  virtual void SetAnchorToNorthEast() 
    { this->SetAnchor(vtkKWTkOptions::AnchorNorthEast); };
  virtual void SetAnchorToEast() 
    { this->SetAnchor(vtkKWTkOptions::AnchorEast); };
  virtual void SetAnchorToSouthEast() 
    { this->SetAnchor(vtkKWTkOptions::AnchorSouthEast); };
  virtual void SetAnchorToSouth() 
    { this->SetAnchor(vtkKWTkOptions::AnchorSouth); };
  virtual void SetAnchorToSouthWest() 
    { this->SetAnchor(vtkKWTkOptions::AnchorSouthWest); };
  virtual void SetAnchorToWest() 
    { this->SetAnchor(vtkKWTkOptions::AnchorWest); };
  virtual void SetAnchorToNorthWest() 
    { this->SetAnchor(vtkKWTkOptions::AnchorNorthWest); };
  virtual void SetAnchorToCenter() 
    { this->SetAnchor(vtkKWTkOptions::AnchorCenter); };

  // Description:
  // Set/get the highlight thickness, a non-negative value indicating the
  // width of the highlight rectangle to draw around the outside of the
  // widget when it has the input focus.
  virtual void SetHighlightThickness(int);
  virtual int GetHighlightThickness();
  
  // Description:
  // Set/get the border width, a non-negative value
  // indicating the width of the 3-D border to draw around the outside of
  // the widget (if such a border is being drawn; the Relief option typically
  // determines this).
  virtual void SetBorderWidth(int);
  virtual int GetBorderWidth();
  
  // Description:
  // Set/Get the 3-D effect desired for the widget. 
  // The value indicates how the interior of the widget should appear
  // relative to its exterior. 
  // Valid constants can be found in vtkKWTkOptions::ReliefType.
  virtual void SetRelief(int);
  virtual int GetRelief();
  virtual void SetReliefToRaised() 
    { this->SetRelief(vtkKWTkOptions::ReliefRaised); };
  virtual void SetReliefToSunken() 
    { this->SetRelief(vtkKWTkOptions::ReliefSunken); };
  virtual void SetReliefToFlat() 
    { this->SetRelief(vtkKWTkOptions::ReliefFlat); };
  virtual void SetReliefToRidge() 
    { this->SetRelief(vtkKWTkOptions::ReliefRidge); };
  virtual void SetReliefToSolid() 
    { this->SetRelief(vtkKWTkOptions::ReliefSolid); };
  virtual void SetReliefToGroove() 
    { this->SetRelief(vtkKWTkOptions::ReliefGroove); };

  // Description:
  // Set/Get the padding that will be applied around each widget (in pixels).
  // Specifies a non-negative value indicating how much extra space to request
  // for the widget in the X and Y-direction. When computing how large a
  // window it needs, the widget will add this amount to the width it would
  // normally need (as determined by the width of the things displayed
  // in the widget); if the geometry manager can satisfy this request, the 
  // widget will end up with extra internal space around what it displays 
  // inside. 
  virtual void SetPadX(int);
  virtual int GetPadX();
  virtual void SetPadY(int);
  virtual int GetPadY();

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

  // Description::
  // Override Unregister since widgets have loops.
  virtual void UnRegister(vtkObjectBase *o);

  // Description:
  // Get the net reference count of this widget. That is the
  // reference count of this widget minus its children.
  virtual int  GetNetReferenceCount();

  // Description:
  // Create a specific Tk widget of type 'type', with optional arguments 
  // 'args' and map it to this instance.
  // Use the Create() method to create this widget instance instead, do *not*
  // use the CreateWidget() method unless you are calling from a subclass to
  // implement a specific kind of Tk widget as a vtkKWWidget subclass, or
  // unless you have to map an external pure Tk widget into a vtkKWWidget.
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

  char *WidgetName;

  vtkKWWidget *Parent;

  // Ballon help

  char *BalloonHelpString;
  
  // Description:
  // Get the Tk string type of the widget.
  virtual const char* GetType();
  
  // Description:
  // Convert a Tcl string (stored internally as UTF-8/Unicode) to another
  // internal format (given the widget's application CharacterEncoding), 
  // and vice-versa.
  // The 'source' string is the source to convert.
  // It returns a pointer to a static buffer where the converted string
  // can be found (so be quick about it).
  // The 'options' can be set to perform some replacements/escaping.
  // ConvertStringEscapeInterpretable will attempt to escape all characters
  // that can be interpreted (when found between a pair of quotes for
  // example): $ [ ] "
  //BTX
  enum
  {
    ConvertStringEscapeCurlyBraces   = 1,
    ConvertStringEscapeInterpretable = 2
  };
  const char* ConvertTclStringToInternalString(
    const char *source, int options = 0);
  const char* ConvertInternalStringToTclString(
    const char *source, int options = 0);
  //ETX

  // Description:
  // Convenience function to that propagates the Enabled state of
  // the instance to another subwidget (preferably a sub-widget).
  // It calls SetEnabled(this->GetEnabled()) on the 'widget' parameter
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
