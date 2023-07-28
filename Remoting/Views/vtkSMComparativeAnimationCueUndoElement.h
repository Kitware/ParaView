// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMComparativeAnimationCueUndoElement
 * @brief   UndoElement for ComparativeAnimationCue
 *
 */

#ifndef vtkSMComparativeAnimationCueUndoElement_h
#define vtkSMComparativeAnimationCueUndoElement_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMUndoElement.h"
#include <vtkSmartPointer.h> // needed for vtkSmartPointer.
#include <vtkWeakPointer.h>  // needed for vtkWeakPointer.

class vtkPVXMLElement;

class VTKREMOTINGVIEWS_EXPORT vtkSMComparativeAnimationCueUndoElement : public vtkSMUndoElement
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
