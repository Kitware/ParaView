/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUndoElement.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMUndoElement - abstract superclass for Server Manager undo 
// elements.
// .SECTION Description
// Abstract superclass for Server Manager undo elements. 
// This class keeps a ConnectionID, since every server manager undo/redo element
// has to have a connection id.

#ifndef __vtkSMUndoElement_h
#define __vtkSMUndoElement_h

#include "vtkUndoElement.h"

#include "vtkConnectionID.h" // needed for vtkConnectionID.

class VTK_EXPORT vtkSMUndoElement : public vtkUndoElement
{
public:
  vtkTypeRevisionMacro(vtkSMUndoElement, vtkUndoElement);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  void SetConnectionID(vtkConnectionID id) 
    { this->ConnectionID = id; }
  vtkConnectionID GetConnectionID()  { return this->ConnectionID; }
//ETX
protected:
  vtkSMUndoElement();
  ~vtkSMUndoElement();
//BTX
  vtkConnectionID ConnectionID;
//ETX
private:
  vtkSMUndoElement(const vtkSMUndoElement&); // Not implemented.
  void operator=(const vtkSMUndoElement&); // Not implemented.
};


#endif

