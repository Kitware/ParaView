/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxApplyFilterCommand.h
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
// .NAME vtkHierarchicalBoxApplyFilterCommand -
// .SECTION Description

#ifndef __vtkHierarchicalBoxApplyFilterCommand_h
#define __vtkHierarchicalBoxApplyFilterCommand_h

#include "vtkApplyFilterCommand.h"
#include "vtkAMRBox.h"

class vtkHierarchicalBoxDataSet;

class VTK_EXPORT vtkHierarchicalBoxApplyFilterCommand : 
  public vtkApplyFilterCommand
{
public:
  static vtkHierarchicalBoxApplyFilterCommand *New(); 

  vtkTypeRevisionMacro(vtkHierarchicalBoxApplyFilterCommand, 
                       vtkApplyFilterCommand);
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
  void SetOutput(vtkHierarchicalBoxDataSet* output);
  vtkGetObjectMacro(Output, vtkHierarchicalBoxDataSet);

  // Description:
  void Initialize();

//BTX
  struct LevelInformation
  {
    unsigned int Level;
    unsigned int DataSetId;
    vtkAMRBox Box;
  };
//ETX
protected:

  vtkHierarchicalBoxDataSet* Output;

  vtkHierarchicalBoxApplyFilterCommand();
  ~vtkHierarchicalBoxApplyFilterCommand();
};



#endif /* __vtkHierarchicalBoxApplyFilterCommand_h */
 
