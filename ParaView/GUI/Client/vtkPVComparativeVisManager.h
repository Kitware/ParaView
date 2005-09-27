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
// vtkSMComparativeVisProxy

#ifndef __vtkPVComparativeVisManager_h
#define __vtkPVComparativeVisManager_h

#include "vtkKWObject.h"

class vtkInteractorStyleTrackballMultiActor;
class vtkPVAnimationCue;
class vtkPVApplication;
class vtkSMComparativeVisProxy;
//BTX
struct vtkPVComparativeVisManagerInternals;
//ETX

class VTK_EXPORT vtkPVComparativeVisManager : public vtkKWObject
{
public:
  static vtkPVComparativeVisManager* New();
  vtkTypeRevisionMacro(vtkPVComparativeVisManager, vtkKWObject);
  void PrintSelf(ostream& os ,vtkIndent indent);

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
  vtkSMComparativeVisProxy* GetVisualization(unsigned int idx);
  vtkSMComparativeVisProxy* GetVisualization(const char* name); 

  // Description:
  // Add a visualization.
  void AddVisualization(vtkSMComparativeVisProxy* vis); 

  // Description:
  // Generate a visualization.
  void GenerateVisualization(vtkSMComparativeVisProxy* vis); 

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
  vtkGetObjectMacro(CurrentlyDisplayedVisualization, vtkSMComparativeVisProxy);

  // Description:
  // Saves the state of comparative visualizations to file as
  // as Tcl script.
  virtual void SaveState(ofstream *file);
  
protected:
  vtkPVComparativeVisManager();
  ~vtkPVComparativeVisManager();

  vtkInteractorStyleTrackballMultiActor* IStyle;

  char* SelectedVisualizationName;

  vtkSMComparativeVisProxy* CurrentlyDisplayedVisualization;

  vtkPVApplication* GetPVApplication();

private:
  // PIMPL
  vtkPVComparativeVisManagerInternals* Internal;

  vtkPVComparativeVisManager(const vtkPVComparativeVisManager&); // Not implemented.
  void operator=(const vtkPVComparativeVisManager&); // Not implemented.
};

#endif

