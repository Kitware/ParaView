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
/**
 * @class   vtkSMComparativeAnimationCueUndoElement
 * @brief   UndoElement for ComparativeAnimationCue
 *
*/

#ifndef vtkSMComparativeAnimationCueUndoElement_h
#define vtkSMComparativeAnimationCueUndoElement_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMUndoElement.h"
#include <vtkSmartPointer.h> // needed for vtkSmartPointer.
#include <vtkWeakPointer.h>  // needed for vtkWeakPointer.

class vtkPVXMLElement;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMComparativeAnimationCueUndoElement
  : public vtkSMUndoElement
{
public:
  static vtkSMComparativeAnimationCueUndoElement* New();
  vtkTypeMacro(vtkSMComparativeAnimationCueUndoElement, vtkSMUndoElement);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int Undo() override;
  int Redo() override;

  void SetXMLStates(vtkTypeUInt32 id, vtkPVXMLElement* before, vtkPVXMLElement* after);

protected:
  vtkSMComparativeAnimationCueUndoElement();
  ~vtkSMComparativeAnimationCueUndoElement() override;

  vtkSmartPointer<vtkPVXMLElement> BeforeState;
  vtkSmartPointer<vtkPVXMLElement> AfterState;
  vtkTypeUInt32 ComparativeAnimationCueID;

private:
  vtkSMComparativeAnimationCueUndoElement(const vtkSMComparativeAnimationCueUndoElement&) = delete;
  void operator=(const vtkSMComparativeAnimationCueUndoElement&) = delete;
};

#endif
