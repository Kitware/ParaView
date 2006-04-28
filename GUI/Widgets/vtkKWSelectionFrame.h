/*=========================================================================

  Module:    vtkKWSelectionFrame.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWSelectionFrame - Selection Frame 
// .SECTION Description
// The selction frame is what contains a render widget.  
// It is called a "selection frame" because in its title bar, you can 
// select which render widget to display in it.
// .SECTION See Also
// vtkKWSelectionFrameLayoutManager

#ifndef __vtkKWSelectionFrame_h
#define __vtkKWSelectionFrame_h

#include "vtkKWCompositeWidget.h"

class vtkKWFrame;
class vtkKWLabel;
class vtkKWMenuButton;
class vtkKWPushButton;
class vtkKWSelectionFrameInternals;
class vtkKWToolbarSet;

class KWWidgets_EXPORT vtkKWSelectionFrame : public vtkKWCompositeWidget
{
public:
  static vtkKWSelectionFrame* New();
  vtkTypeRevisionMacro(vtkKWSelectionFrame, vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get title
  virtual void SetTitle(const char *title);
  virtual const char* GetTitle();

  // Description:
  // Select/Deselect the window
  virtual void SetSelected(int);
  vtkGetMacro(Selected, int);
  vtkBooleanMacro(Selected, int);

  // Description:
  // Set the selection list (array of num strings).
  // The selection list is represented as a pull down menu, which
  // visibility can be set. As a convenience, any entry made of two
  // dashes "--" is used as a separator.
  // This selection list can be used, for example, to display the titles
  // of other selection frames that can be switched with the current 
  // selection frame.
  virtual void SetSelectionList(int num, const char **list);
  vtkGetObjectMacro(SelectionList, vtkKWMenuButton);

  // Description:
  // Specifies a command to associate with the widget. This command is 
  // invoked when an item is picked by the user in the selection list.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - item selected in the list: const char *
  // - pointer to this object: vtkKWSelectionFrame*
  virtual void SetSelectionListCommand(vtkObject *object, const char *method);

  // Description:
  // Set/Get the selection list visibility.
  virtual void SetSelectionListVisibility(int);
  vtkGetMacro(SelectionListVisibility, int);
  vtkBooleanMacro(SelectionListVisibility, int);

  // Description:
  // Allow the close functionality (button and menu entry)
  // If set, a close button is added in the top right corner,
  // and a "Close" entry is added to the end of the selection list.
  virtual void SetAllowClose(int);
  vtkGetMacro(AllowClose, int);
  vtkBooleanMacro(AllowClose, int);
  vtkGetObjectMacro(CloseButton, vtkKWPushButton);

  // Description:
  // Specifies a command to associate with the widget. This command is 
  // typically invoked when the widget is closed (using the close button
  // or the Close() method).
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - pointer to this object: vtkKWSelectionFrame*
  virtual void SetCloseCommand(vtkObject *object, const char *method);

  // Description:
  // Close the selection frame. It can be re-implemented by
  // subclasses to add more functionalities, release resources, etc.
  // The current implementation invokes the CloseCommand, if any.
  virtual void Close();

  // Description:
  // Allow title to be changed (menu entry)
  // If set, a "Change title" entry is added to the end of the selection list,
  // enabling the title to be changed using the ChangeTitleCommand. There
  // is actually no code or user interface to change the title, it is left
  // to the ChangeTitleCommand.
  virtual void SetAllowChangeTitle(int);
  vtkGetMacro(AllowChangeTitle, int);
  vtkBooleanMacro(AllowChangeTitle, int);

  // Description:
  // Specifies a command to associate with the widget. This command is 
  // typically invoked when the "Change title" menu entry is selected.
  // This command is usually implemented by a different class and will, 
  // for example, query the user for a new title, check that this title meet
  // some constraints, and call SetTitle() on this object (which in turn will
  // trigger the TitleChangedCommand).
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - pointer to this object: vtkKWSelectionFrame*
  virtual void SetChangeTitleCommand(vtkObject *object, const char *method);

  // Description:
  // Specifies a command to associate with the widget. This command is 
  // typically invoked when the title is changed.
  // This command can be used, for example, to notify a layout manager that
  // it should refresh its list of available selection frame titles 
  // (see vtkKWSelectionFrameLayoutManager).
  // Do not confuse this command with the ChangeTitleCommand, which is invoked
  // when the "Change Title" menu entry is selected by the user, and is used
  // to allow a third-party class to provide some user-dialog and change
  // the title (given some potential constraints). This user-dialog will, in
  // turn, most probably call SetTitle, which will trigger TitleChangedCommand.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - pointer to this object: vtkKWSelectionFrame*
  virtual void SetTitleChangedCommand(vtkObject *object, const char *method);

  // Description:
  // Specifies a command to associate with the widget. This command is 
  // typically invoked when the frame title is selected by the user
  // (click in title bar).
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - pointer to this object: vtkKWSelectionFrame*
  virtual void SetSelectCommand(vtkObject *object, const char *method);

  // Description:
  // Specifies a command to associate with the widget. This command is 
  // typically invoked when the f frame title is double-clicked on.
  // Note that this will also invoke the SelectCommand, since the first
  // click acts as a select event.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - pointer to this object: vtkKWSelectionFrame*
  virtual void SetDoubleClickCommand(vtkObject *object, const char *method);

  // Description:
  // Set/Get the title foregroud/background color (in both normal and 
  // selected mode). 
  vtkGetVector3Macro(TitleColor, double);
  virtual void SetTitleColor(double r, double g, double b);
  virtual void SetTitleColor(double rgb[3])
    { this->SetTitleColor(rgb[0], rgb[1], rgb[2]); };
  vtkGetVector3Macro(TitleSelectedColor, double);
  virtual void SetTitleSelectedColor(double r, double g, double b);
  virtual void SetTitleSelectedColor(double rgb[3])
    { this->SetTitleSelectedColor(rgb[0], rgb[1], rgb[2]); };
  vtkGetVector3Macro(TitleBackgroundColor, double);
  virtual void SetTitleBackgroundColor(double r, double g, double b);
  virtual void SetTitleBackgroundColor(double rgb[3])
    { this->SetTitleBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  vtkGetVector3Macro(TitleSelectedBackgroundColor, double);
  virtual void SetTitleSelectedBackgroundColor(double r, double g, double b);
  virtual void SetTitleSelectedBackgroundColor(double rgb[3])
    { this->SetTitleSelectedBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get the title bar visibility.
  virtual void SetTitleBarVisibility(int);
  vtkGetMacro(TitleBarVisibility, int);
  vtkBooleanMacro(TitleBarVisibility, int);

  // Description:
  // Set/Get the toolbar set visibility, and retrieve the toolbar set.
  // The toolbar set is usually displayed below the title bar
  virtual vtkKWToolbarSet* GetToolbarSet();
  virtual void SetToolbarSetVisibility(int);
  vtkGetMacro(ToolbarSetVisibility, int);
  vtkBooleanMacro(ToolbarSetVisibility, int);

  // Description:
  // Retrieve the title bar user frame. This frame sits in the title
  // bar, on the right side of the title itself, and be used to insert
  // user-defined UI elements. It is not visible if TitleBarVisibility
  // is Off.
  virtual vtkKWFrame* GetTitleBarUserFrame();

  // Description:
  // Retrieve the body frame. This is the main frame, below the title bar,
  // where to pack the real contents of whatever that object is supposed
  // to display (say, a render widget).
  vtkGetObjectMacro(BodyFrame, vtkKWFrame);
  
  // Description:
  // Set/Get the left user frame visibility, and retrieve the frame.
  // The left user frame is displayed on the left side of the BodyFrame.
  virtual vtkKWFrame* GetLeftUserFrame();
  virtual void SetLeftUserFrameVisibility(int);
  vtkGetMacro(LeftUserFrameVisibility, int);
  vtkBooleanMacro(LeftUserFrameVisibility, int);

  // Description:
  // Set/Get the left user frame visibility, and retrieve the frame.
  // The left user frame is displayed on the left side of the BodyFrame.
  virtual vtkKWFrame* GetRightUserFrame();
  virtual void SetRightUserFrameVisibility(int);
  vtkGetMacro(RightUserFrameVisibility, int);
  vtkBooleanMacro(RightUserFrameVisibility, int);

  // Description:
  // Set/Get the outer selection frame width and color. The outer selection
  // frame is a thin frame around the whole widget which color is changed
  // when the widget is selected. This is useful, for example, when the
  // title bar is not visible (the title bar color also changes when the
  // widget is selected). Set the widget of the selection frame to 0 to
  // discard this feature. Colors can be customized.
  virtual void SetOuterSelectionFrameWidth(int);
  vtkGetMacro(OuterSelectionFrameWidth, int);
  vtkGetVector3Macro(OuterSelectionFrameColor, double);
  virtual void SetOuterSelectionFrameColor(double r, double g, double b);
  virtual void SetOuterSelectionFrameColor(double rgb[3])
    { this->SetOuterSelectionFrameColor(rgb[0], rgb[1], rgb[2]); };
  vtkGetVector3Macro(OuterSelectionFrameSelectedColor, double);
  virtual void SetOuterSelectionFrameSelectedColor(
    double r, double g, double b);
  virtual void SetOuterSelectionFrameSelectedColor(double rgb[3])
    { this->SetOuterSelectionFrameSelectedColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Callbacks. Internal, do not use.
  virtual void CloseCallback();
  virtual void SelectionListCallback(const char *menuItem);
  virtual void SelectCallback();
  virtual void DoubleClickCallback();
  virtual void ChangeTitleCallback();
  
protected:
  vtkKWSelectionFrame();
  ~vtkKWSelectionFrame();
  
  // Description:
  // Create the widget.
  virtual void CreateWidget();
  
  vtkKWFrame      *OuterSelectionFrame;
  vtkKWFrame      *TitleBarFrame;
  vtkKWMenuButton *SelectionList;
  vtkKWPushButton *CloseButton;
  vtkKWLabel      *Title;
  vtkKWFrame      *BodyFrame;

  virtual void Pack();
  virtual void Bind();
  virtual void UnBind();

  virtual int SetColor(double *color, double r, double g, double b);
  virtual void UpdateSelectedAspect();
  virtual void UpdateSelectionList();

  double TitleColor[3];
  double TitleSelectedColor[3];
  double TitleBackgroundColor[3];
  double TitleSelectedBackgroundColor[3];

  double OuterSelectionFrameColor[3];
  double OuterSelectionFrameSelectedColor[3];

  char *SelectionListCommand;
  char *CloseCommand;
  char *SelectCommand;
  char *DoubleClickCommand;
  char *ChangeTitleCommand;
  char *TitleChangedCommand;
  virtual void InvokeSelectionListCommand(const char*, vtkKWSelectionFrame*);
  virtual void InvokeCloseCommand(vtkKWSelectionFrame *obj);
  virtual void InvokeSelectCommand(vtkKWSelectionFrame *obj);
  virtual void InvokeDoubleClickCommand(vtkKWSelectionFrame *obj);
  virtual void InvokeChangeTitleCommand(vtkKWSelectionFrame *obj);
  virtual void InvokeTitleChangedCommand(vtkKWSelectionFrame *obj);

  int Selected;
  int SelectionListVisibility;
  int AllowClose;
  int AllowChangeTitle;
  int ToolbarSetVisibility;
  int LeftUserFrameVisibility;
  int RightUserFrameVisibility;
  int TitleBarVisibility;
  int OuterSelectionFrameWidth;

  // PIMPL Encapsulation for STL containers

  vtkKWSelectionFrameInternals *Internals;

private:

  vtkKWToolbarSet *ToolbarSet;
  vtkKWFrame      *LeftUserFrame;
  vtkKWFrame      *RightUserFrame;
  vtkKWFrame      *TitleBarUserFrame;

  vtkKWSelectionFrame(const vtkKWSelectionFrame&);  // Not implemented
  void operator=(const vtkKWSelectionFrame&);  // Not implemented
};

#endif

