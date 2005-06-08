/*=========================================================================

  Program:   ParaView
  Module:    vtkPVComparativeVisManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVComparativeVisManager - manager of comparative visualizations
// .SECTION Description
// vtkPVComparativeVisManager is responsible of managing one or more
// comparative visualizations. It can store, generate, show and hide
// visualizations. 
// .SECTION See Also
// vtkPVComparativeVis

#ifndef __vtkPVComparativeVisManager_h
#define __vtkPVComparativeVisManager_h

#include "vtkObject.h"

class vtkInteractorStyleTrackballMultiActor;
class vtkPVAnimationCue;
class vtkPVApplication;
class vtkPVComparativeVis;
//BTX
struct vtkPVComparativeVisManagerInternals;
//ETX

class VTK_EXPORT vtkPVComparativeVisManager : public vtkObject
{
public:
  static vtkPVComparativeVisManager* New();
  vtkTypeRevisionMacro(vtkPVComparativeVisManager, vtkObject);
  void PrintSelf(ostream& os ,vtkIndent indent);

  // Description:
  // Application should be set before first call to 
  // GenerateVisualization().
  void SetApplication(vtkPVApplication*);

  // Description:
  // Show the current visualization (SelectedVisualizationName).
  // This sets up the main window in a way appropriate for comparative
  // vis. This includes setting up the interactor, disabling certain
  // menus and removing panels/toolbars.
  int Show();

  // Description:
  // Hide a visualization if shown. Also restores the state of
  // the main window to prior Show().
  void Hide();

  // Description:
  // Returns the number of registered visualizations.
  unsigned int GetNumberOfVisualizations();

  // Description:
  // Retrieve a visualization bu index or name.
  vtkPVComparativeVis* GetVisualization(unsigned int idx);
  vtkPVComparativeVis* GetVisualization(const char* name); 

  // Description:
  // Add a visualization.
  void AddVisualization(vtkPVComparativeVis* vis); 

  // Description:
  // Generate a visualization.
  void GenerateVisualization(vtkPVComparativeVis* vis); 

  // Description:
  // Remove a visualization.
  void RemoveVisualization(const char* name); 

  // Description:
  // Set/Get the name of the currently selected visualization.
  // This is the one that is displayed with Show().
  vtkSetStringMacro(SelectedVisualizationName);
  vtkGetStringMacro(SelectedVisualizationName);

  // Description:
  // Returns the currently shown visualization. Returns NULL
  // if no managed visualization is shown.
  vtkGetObjectMacro(CurrentlyDisplayedVisualization, vtkPVComparativeVis);

protected:
  vtkPVComparativeVisManager();
  ~vtkPVComparativeVisManager();

  vtkPVApplication* Application;

  vtkInteractorStyleTrackballMultiActor* IStyle;

  char* SelectedVisualizationName;

  vtkPVComparativeVis* CurrentlyDisplayedVisualization;

private:
  // PIMPL
  vtkPVComparativeVisManagerInternals* Internal;

  vtkPVComparativeVisManager(const vtkPVComparativeVisManager&); // Not implemented.
  void operator=(const vtkPVComparativeVisManager&); // Not implemented.
};

#endif

