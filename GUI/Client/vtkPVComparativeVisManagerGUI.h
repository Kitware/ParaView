/*=========================================================================

  Module:    vtkPVComparativeVisManagerGUI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVComparativeVisManagerGUI - user interface to vtkPVComparativeVisManager
// .SECTION Description
// vtkPVComparativeVisManagerGUI provides user interface to 
// vtkPVComparativeVisManager. It does not store state but uses
// vtkPVComparativeVisManager as state data.

#ifndef __vtkPVComparativeVisManagerGUI_h
#define __vtkPVComparativeVisManagerGUI_h

#include "vtkKWTopLevel.h"

class vtkKWFrame;
class vtkKWFrameLabeled;
class vtkKWListBox;
class vtkKWPushButton;
class vtkPVComparativeVisDialog;
class vtkPVComparativeVisManager;

class VTK_EXPORT vtkPVComparativeVisManagerGUI : public vtkKWTopLevel
{
public:
  static vtkPVComparativeVisManagerGUI* New();
  vtkTypeRevisionMacro(vtkPVComparativeVisManagerGUI,vtkKWTopLevel);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Update the widget from the cv manager.
  void Update();

  // Description:
  // Brings up the visualization dialog with a new visualization.
  void AddVisualization();

  // Description:
  // Brings up the visualization dialog for the current visualization.
  void EditVisualization();

  // Description:
  // Deletes the selected visualization.
  void DeleteVisualization();

  // Description:
  // Shows the selected visualization.
  void ShowVisualization();

  // Description:
  // Hides the currently shown visualization.
  void HideVisualization();

  // Description:
  // Should be called by PrepareDelete() of window.
  void PrepareForDelete();

  // Description:
  // Returns the comparative manager object. This is the actual
  // object that manager the comparative visualizations.
  vtkGetObjectMacro(Manager, vtkPVComparativeVisManager);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  virtual void UpdateEnableState();

  // Description:
  // Called by the list box when a visualization is selected.
  void ItemSelected();

protected:
  vtkPVComparativeVisManagerGUI();
  ~vtkPVComparativeVisManagerGUI();

  vtkKWFrame* MainFrame;

  // List of visualizations
  vtkKWFrameLabeled* ListFrame;
  vtkKWListBox* ComparativeVisList;

  // Buttons
  vtkKWFrame* CommandFrame;
  vtkKWPushButton* CreateButton;
  vtkKWPushButton* EditButton;
  vtkKWPushButton* DeleteButton;
  vtkKWPushButton* ShowButton;
  vtkKWPushButton* HideButton;
  vtkKWPushButton* CloseButton;

  // Popup dialog to edit a visualization
  vtkPVComparativeVisDialog* EditDialog;

  // Underlying data
  vtkPVComparativeVisManager* Manager;

  int InShow;
  int VisSelected;

private:
  vtkPVComparativeVisManagerGUI(const vtkPVComparativeVisManagerGUI&); // Not implemented
  void operator=(const vtkPVComparativeVisManagerGUI&); // Not Implemented
};

#endif
