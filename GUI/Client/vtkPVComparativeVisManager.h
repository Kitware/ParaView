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

class vtkPVAnimationCue;
class vtkPVApplication;
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

  void TraverseAllCues();

  void RemoveAllCues();


  void SaveAllGeometry();

  void PlayAll();

protected:
  vtkPVComparativeVisManager();
  ~vtkPVComparativeVisManager();

  vtkPVApplication* Application;
  vtkPVComparativeVisManagerInternals* Internal;

  void TraverseCue(vtkPVAnimationCue* cue);
  void SaveGeometry();
  void PlayOne(unsigned int idx);

  void ExecuteEvent(vtkObject* , unsigned long event, unsigned int paramIdx);

//BTX
  friend class vtkCVAnimationSceneObserver;
//ETX

private:
  vtkPVComparativeVisManager(const vtkPVComparativeVisManager&); // Not implemented.
  void operator=(const vtkPVComparativeVisManager&); // Not implemented.
};

#endif

