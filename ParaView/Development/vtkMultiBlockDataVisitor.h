/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockDataVisitor.h
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
// .NAME vtkMultiBlockDataVisitor -
// .SECTION Description

#ifndef __vtkMultiBlockDataVisitor_h
#define __vtkMultiBlockDataVisitor_h

#include "vtkCompositeDataVisitor.h"

class vtkMultiBlockDataIterator;

class VTK_EXPORT vtkMultiBlockDataVisitor : public vtkCompositeDataVisitor
{
public:
  static vtkMultiBlockDataVisitor *New();

  vtkTypeRevisionMacro(vtkMultiBlockDataVisitor,vtkCompositeDataVisitor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the iterator used to access the items in the input.
  void SetDataIterator(vtkMultiBlockDataIterator* it);
  vtkGetObjectMacro(DataIterator, vtkMultiBlockDataIterator);

  // Description:
  // Apply the command on each object in the collection.
  virtual void Execute();

protected:
  vtkMultiBlockDataVisitor(); 
  virtual ~vtkMultiBlockDataVisitor(); 

  vtkMultiBlockDataIterator* DataIterator;

private:
  vtkMultiBlockDataVisitor(const vtkMultiBlockDataVisitor&);  // Not implemented.
  void operator=(const vtkMultiBlockDataVisitor&);  // Not implemented.
};

#endif

