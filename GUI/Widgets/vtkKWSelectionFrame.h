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

#ifndef __vtkKWSelectionFrame_h
#define __vtkKWSelectionFrame_h

#include "vtkKWCompositeWidget.h"

class vtkKWFrame;
class vtkKWLabel;
class vtkKWMenuButton;
class vtkKWPushButton;
class vtkKWToolbarSet;
class vtkKWSelectionFrameInternals;

class KWWIDGETS_EXPORT vtkKWSelectionFrame : public vtkKWCompositeWidget
{
public:
  static vtkKWSelectionFrame* New();
  vtkTypeRevisionMacro(vtkKWSelectionFrame, vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);
  
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
  // Set the selection list (array of num strings) and the command
  // that is called when a selection is made by the user. 
  // This command is passed both the selected string and 
  // a pointer to this object.
  // The selection list is represented as a pull down menu, which
  // visibility can be set.
  virtual void SetSelectionList(int num, const char **list);
  virtual void SetSelectionListCommand(vtkObject *object, const char *method);
  vtkGetObjectMacro(SelectionList, vtkKWMenuButton);
  virtual void SetSelectionListVisibility(int);
  vtkGetMacro(SelectionListVisibility, int);
  vtkBooleanMacro(SelectionListVisibility, int);

  // Description:
  // Allow the close functionality (button and menu entry)
  // If set, a close button is added in the top right corner,
  // and a "Close" entry is added to the end of the selection list.
  // Set the close command, called when the the close button or the menu entry
  // is selected by the user.
  // This command is passed a pointer to this object.
  virtual void SetAllowClose(int);
  vtkGetMacro(AllowClose, int);
  vtkBooleanMacro(AllowClose, int);
  virtual void SetCloseCommand(vtkObject *object, const char *method);
  vtkGetObjectMacro(CloseButton, vtkKWPushButton);

  // Description:
  // Allow title to be changed (menu entry)
  // If set, a "Change title" entry is added to the end of the selection list.
  // Set the command called when the menu entry is selected by the user.
  // This command is passed a pointer to this object.
  virtual void SetAllowChangeTitle(int);
  vtkGetMacro(AllowChangeTitle, int);
  vtkBooleanMacro(AllowChangeTitle, int);
  virtual void SetChangeTitleCommand(vtkObject *object, const char *method);

  // Description:
  // Set the command called when the frame title is selected by the user
  // (click in title bar).
  // This command is passed a pointer to this object.
  virtual void SetSelectCommand(vtkObject *object, const char *method);

  // Description:
  // Set the command called when the frame title is double-clicked on.
  // Note that this will also invoke the SelectCommand, since the first
  // click acts as a select event.
  // This command is passed a pointer to this object.
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
  // Callbacks
  virtual void CloseCallback();
  virtual void SelectionListCallback(const char *menuItem);
  virtual void SelectCallback();
  virtual void DoubleClickCallback();
  virtual void ChangeTitleCallback();
  
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWSelectionFrame();
  ~vtkKWSelectionFrame();
  
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

  char *CloseCommand;
  char *SelectionListCommand;
  char *SelectCommand;
  char *DoubleClickCommand;
  char *ChangeTitleCommand;

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

