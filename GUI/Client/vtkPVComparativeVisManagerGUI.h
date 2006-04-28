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

class vtkCVProgressObserver;
class vtkKWFrame;
class vtkKWFrameWithLabel;
class vtkKWListBox;
class vtkKWPushButton;
class vtkPVComparativeVisDialog;
class vtkPVComparativeVisManager;
class vtkPVComparativeVisProgressDialog;
class vtkSMComparativeVisProxy;

class VTK_EXPORT vtkPVComparativeVisManagerGUI : public vtkKWTopLevel
{
public:
  static vtkPVComparativeVisManagerGUI* New();
  vtkTypeRevisionMacro(vtkPVComparativeVisManagerGUI,vtkKWTopLevel);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  // object that manages the comparative visualizations.
  vtkGetObjectMacro(Manager, vtkPVComparativeVisManager);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  virtual void UpdateEnableState();

  // Description:
  // Called by the list box when a visualization is selected.
  void ItemSelected();

  // Description:
  // Saves the state of comparative visualizations to file as
  // as Tcl script.
  virtual void SaveState(ofstream *file);

  // Description:
  // Returns the list box of comparative visualizations.
  vtkGetObjectMacro(ComparativeVisList, vtkKWListBox);

protected:
  vtkPVComparativeVisManagerGUI();
  ~vtkPVComparativeVisManagerGUI();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  vtkKWFrame* MainFrame;

  // List of visualizations
  vtkKWFrameWithLabel* ListFrame;
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

  // Popup dialog to show progress during generate
  vtkPVComparativeVisProgressDialog* ProgressDialog;

  // Underlying data
  vtkPVComparativeVisManager* Manager;

  int InShow;
  int VisSelected;

  void UpdateProgress(double prog);

  vtkSMComparativeVisProxy* VisBeingGenerated;

//BTX
  friend class vtkCVProgressObserver;
//ETX

private:
  vtkCVProgressObserver* ProgressObserver;

  vtkPVComparativeVisManagerGUI(const vtkPVComparativeVisManagerGUI&); // Not implemented
  void operator=(const vtkPVComparativeVisManagerGUI&); // Not Implemented
};

#endif
