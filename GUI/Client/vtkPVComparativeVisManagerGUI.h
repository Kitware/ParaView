/*=========================================================================

  Module:    vtkPVComparativeVisManagerGUI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVComparativeVisManagerGUI -
// .SECTION Description

#ifndef __vtkPVComparativeVisManagerGUI_h
#define __vtkPVComparativeVisManagerGUI_h

#include "vtkKWTopLevel.h"

class vtkKWListBox;
class vtkKWPushButton;
class vtkPVComparativeVisManager;

class VTK_EXPORT vtkPVComparativeVisManagerGUI : public vtkKWTopLevel
{
public:
  static vtkPVComparativeVisManagerGUI* New();
  vtkTypeRevisionMacro(vtkPVComparativeVisManagerGUI,vtkKWTopLevel);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Update the widget from the cv manager.
  void Update();

  // Description:
  // Brings up the visualization dialog with a new visualization
  void AddVisualization();

  // Description:
  // Brings up the visualization dialog for the current visualization
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

protected:
  vtkPVComparativeVisManagerGUI();
  ~vtkPVComparativeVisManagerGUI();

  vtkKWListBox* ComparativeVisList;

  vtkKWPushButton* AddButton;
  vtkKWPushButton* EditButton;
  vtkKWPushButton* DeleteButton;
  vtkKWPushButton* ShowButton;
  vtkKWPushButton* HideButton;

  vtkPVComparativeVisManager* Manager;

private:
  vtkPVComparativeVisManagerGUI(const vtkPVComparativeVisManagerGUI&); // Not implemented
  void operator=(const vtkPVComparativeVisManagerGUI&); // Not Implemented
};

#endif
