/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockApplyFilterCommand.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiBlockApplyFilterCommand -
// .SECTION Description

#ifndef __vtkMultiBlockApplyFilterCommand_h
#define __vtkMultiBlockApplyFilterCommand_h

#include "vtkApplyFilterCommand.h"

class vtkMultiBlockDataSet;

class VTK_EXPORT vtkMultiBlockApplyFilterCommand : public vtkApplyFilterCommand
{
public:
  static vtkMultiBlockApplyFilterCommand *New(); 

  vtkTypeRevisionMacro(vtkMultiBlockApplyFilterCommand, vtkApplyFilterCommand);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Satisfy the superclass API for callbacks. Recall that the caller is
  // the instance invoking the event; eid is the event id (see 
  // vtkCommand.h); and calldata is information sent when the callback
  // was invoked
  virtual void Execute(vtkCompositeDataVisitor *caller, 
                       vtkDataObject *input,
                       void* callData);

  // Description:
  void SetOutput(vtkMultiBlockDataSet* output);
  vtkGetObjectMacro(Output, vtkMultiBlockDataSet);

  // Description:
  void Initialize();

protected:

  vtkMultiBlockDataSet* Output;

  vtkMultiBlockApplyFilterCommand();
  ~vtkMultiBlockApplyFilterCommand();

private:
  vtkMultiBlockApplyFilterCommand(
    const vtkMultiBlockApplyFilterCommand&); // Not implemented
  void operator=(const vtkMultiBlockApplyFilterCommand&); // Not implemented
};



#endif /* __vtkMultiBlockApplyFilterCommand_h */
 
