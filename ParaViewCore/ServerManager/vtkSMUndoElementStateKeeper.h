/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUndoElementStateKeeper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMUndoElementStateKeeper - Inactif UndoElement that keep track of
// state of newly created object in a given undoset time range.
// .SECTION Description
// This undo element is used to build a vtkSMStateLocator based on an undoset,
// So underneath undoelement can renew proxy with their correct state.

#ifndef __vtkSMUndoElementStateKeeper_h
#define __vtkSMUndoElementStateKeeper_h

#include "vtkSMUndoElement.h"
#include "vtkSMMessageMinimal.h"

class VTK_EXPORT vtkSMUndoElementStateKeeper : public vtkSMUndoElement
{
public:
  static vtkSMUndoElementStateKeeper* New();
  vtkTypeMacro(vtkSMUndoElementStateKeeper, vtkSMUndoElement);
  void PrintSelf(ostream& os, vtkIndent indent);


  virtual int Undo() {return 1;}
  virtual int Redo() {return 1;}

//BTX
  void KeepCreationState(const vtkSMMessage* state);
  vtkSMMessage* GetCreationState() const;

protected:
  vtkSMUndoElementStateKeeper();
  ~vtkSMUndoElementStateKeeper();

  vtkSMMessage* CreationState;

private:
  vtkSMUndoElementStateKeeper(const vtkSMUndoElementStateKeeper&); // Not implemented.
  void operator=(const vtkSMUndoElementStateKeeper&); // Not implemented.
//ETX
};


#endif
