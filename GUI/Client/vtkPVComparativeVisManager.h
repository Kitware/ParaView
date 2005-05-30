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
// .NAME vtkPVComparativeVisManager
// .SECTION Description

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

  void SetApplication(vtkPVApplication*);

  void Show();
  void Hide();

  unsigned int GetNumberOfVisualizations();

  vtkPVComparativeVis* GetVisualization(unsigned int idx);
  vtkPVComparativeVis* GetVisualization(const char* name); 

  void AddVisualization(vtkPVComparativeVis* vis); 

  void RemoveVisualization(const char* name); 

  vtkSetStringMacro(CurrentVisualization);
  vtkGetStringMacro(CurrentVisualization);

protected:
  vtkPVComparativeVisManager();
  ~vtkPVComparativeVisManager();

  vtkPVApplication* Application;
  vtkPVComparativeVisManagerInternals* Internal;

  vtkInteractorStyleTrackballMultiActor* IStyle;

  char* CurrentVisualization;

private:
  vtkPVComparativeVisManager(const vtkPVComparativeVisManager&); // Not implemented.
  void operator=(const vtkPVComparativeVisManager&); // Not implemented.
};

#endif

