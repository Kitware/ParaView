/*=========================================================================

  Program:   ParaView
  Module:    vtkSMComparativeAnimationCueUndoElement.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMComparativeAnimationCueUndoElement - UndoElement for ComparativeAnimationCue
// .SECTION Description

#ifndef __vtkSMComparativeAnimationCueUndoElement_h
#define __vtkSMComparativeAnimationCueUndoElement_h

#include "vtkSMUndoElement.h"
#include <vtkWeakPointer.h> // needed for vtkWeakPointer.
#include <vtkSmartPointer.h> // needed for vtkSmartPointer.

class vtkPVXMLElement;

class VTK_EXPORT vtkSMComparativeAnimationCueUndoElement : public vtkSMUndoElement
{
public:
  static vtkSMComparativeAnimationCueUndoElement* New();
  vtkTypeMacro(vtkSMComparativeAnimationCueUndoElement, vtkSMUndoElement);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual int Undo();
  virtual int Redo();

  void SetXMLStates(vtkTypeUInt32 id, vtkPVXMLElement* before, vtkPVXMLElement* after);

protected:
  vtkSMComparativeAnimationCueUndoElement();
  ~vtkSMComparativeAnimationCueUndoElement();

  vtkSmartPointer<vtkPVXMLElement> BeforeState;
  vtkSmartPointer<vtkPVXMLElement> AfterState;
  vtkTypeUInt32 ComparativeAnimationCueID;

private:
  vtkSMComparativeAnimationCueUndoElement(const vtkSMComparativeAnimationCueUndoElement&); // Not implemented.
  void operator=(const vtkSMComparativeAnimationCueUndoElement&); // Not implemented.
};

#endif
