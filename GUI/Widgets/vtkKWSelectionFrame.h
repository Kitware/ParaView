/*=========================================================================

  Module:    vtkKWSelectionFrame.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWSelectionFrame
// .SECTION Description

#ifndef __vtkKWSelectionFrame_h
#define __vtkKWSelectionFrame_h

#include "vtkKWWidget.h"

class vtkKWFrame;
class vtkKWLabel;
class vtkKWMenuButton;
class vtkKWToolbarSet;

class VTK_EXPORT vtkKWSelectionFrame : public vtkKWWidget
{
public:
  static vtkKWSelectionFrame* New();
  vtkTypeRevisionMacro(vtkKWSelectionFrame, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args);
  
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
  // that will be called when a selection is made by the user. 
  // This command will be passed both the selected string and 
  // a pointer to this object.
  // The selection list is represented as a pull down menu, which
  // visibility can be set.
  virtual void SetSelectionList(int num, const char **list);
  virtual void SetSelectionListCommand(
    vtkKWObject *object, const char *method);
  virtual void SetShowSelectionList(int);
  vtkGetMacro(ShowSelectionList, int);
  vtkBooleanMacro(ShowSelectionList, int);
  vtkGetObjectMacro(SelectionList, vtkKWMenuButton);

  // Description:
  // Set the select command, called when the frame is selected by the user
  // (click in title bar).
  // This command will be passed a pointer to this object.
  virtual void SetSelectCommand(vtkKWObject *object, const char *method);

  // Description:
  // Set/Get the title foregroud/background color (in both normal and 
  // selected mode). 
  vtkGetVector3Macro(TitleColor, float);
  vtkGetVector3Macro(TitleSelectedColor, float);
  vtkGetVector3Macro(TitleBackgroundColor, float);
  vtkGetVector3Macro(TitleBackgroundSelectedColor, float);
  virtual void SetTitleColor(float r, float g, float b);
  virtual void SetTitleColor(float rgb[3])
    { this->SetTitleColor(rgb[0], rgb[1], rgb[2]); };
  virtual void SetTitleSelectedColor(float r, float g, float b);
  virtual void SetTitleSelectedColor(float rgb[3])
    { this->SetTitleSelectedColor(rgb[0], rgb[1], rgb[2]); };
  virtual void SetTitleBackgroundColor(float r, float g, float b);
  virtual void SetTitleBackgroundColor(float rgb[3])
    { this->SetTitleBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  virtual void SetTitleBackgroundSelectedColor(float r, float g, float b);
  virtual void SetTitleBackgroundSelectedColor(float rgb[3])
    { this->SetTitleBackgroundSelectedColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set the selection list (array of num strings) and the command
  // that will be called when a selection is made by the user. 
  // This command will be passed both the selected string and 
  // a pointer to this object.
  // The selection list is represented as a pull down menu, which
  // visibility can be set.
  virtual void SetShowToolbarSet(int);
  vtkGetMacro(ShowToolbarSet, int);
  vtkBooleanMacro(ShowToolbarSet, int);
  vtkGetObjectMacro(ToolbarSet, vtkKWToolbarSet);

  // Description:
  // Callbacks
  virtual void SelectionListCallback(const char *menuItem);
  virtual void SelectCallback();
  
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Access to sub-widgets
  vtkGetObjectMacro(TitleBarRightSubframe, vtkKWFrame);
  vtkGetObjectMacro(BodyFrame, vtkKWFrame);
  
protected:
  vtkKWSelectionFrame();
  ~vtkKWSelectionFrame();
  
  vtkKWFrame      *TitleBar;
  vtkKWMenuButton *SelectionList;
  vtkKWToolbarSet *ToolbarSet;
  vtkKWLabel      *Title;
  vtkKWFrame      *TitleBarRightSubframe;
  vtkKWFrame      *BodyFrame;

  virtual void Pack();
  virtual void Bind();
  virtual void UnBind();

  virtual int SetColor(float *color, float r, float g, float b);
  virtual void UpdateColors();

  float TitleColor[3];
  float TitleSelectedColor[3];
  float TitleBackgroundColor[3];
  float TitleBackgroundSelectedColor[3];

  char *SelectionListCommand;
  char *SelectCommand;

  int Selected;
  int ShowSelectionList;
  int ShowToolbarSet;

private:
  vtkKWSelectionFrame(const vtkKWSelectionFrame&);  // Not implemented
  void operator=(const vtkKWSelectionFrame&);  // Not implemented
};

#endif

