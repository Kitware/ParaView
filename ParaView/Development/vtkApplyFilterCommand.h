/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkApplyFilterCommand.h
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
// .NAME vtkApplyFilterCommand -
// .SECTION Description

#ifndef __vtkApplyFilterCommand_h
#define __vtkApplyFilterCommand_h

#include "vtkCompositeDataVisitorCommand.h"

class vtkApplyFilterCommandInternal;
class vtkMultiBlockDataSet;
class vtkSource;
class vtkDataObject;

class VTK_EXPORT vtkApplyFilterCommand : public vtkCompositeDataVisitorCommand
{
public:
  vtkTypeRevisionMacro(vtkApplyFilterCommand, vtkCompositeDataVisitorCommand);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  void SetFilter(vtkSource* filter);
  vtkGetObjectMacro(Filter, vtkSource)

protected:

  vtkApplyFilterCommandInternal* Internal;

  vtkSource* Filter;

  int CheckFilterInputMatch(vtkDataObject* inp);
  void SetFilterInput(vtkSource* source, vtkDataObject* input);
  
  vtkApplyFilterCommand();
  ~vtkApplyFilterCommand();

private:
  vtkApplyFilterCommand(
    const vtkApplyFilterCommand&); // Not implemented
  void operator=(const vtkApplyFilterCommand&); // Not implemented
};



#endif /* __vtkApplyFilterCommand_h */
 
