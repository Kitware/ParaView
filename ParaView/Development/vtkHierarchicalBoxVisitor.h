/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxVisitor.h
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
// .NAME vtkHierarchicalBoxVisitor -
// .SECTION Description

#ifndef __vtkHierarchicalBoxVisitor_h
#define __vtkHierarchicalBoxVisitor_h

#include "vtkCompositeDataVisitor.h"

class vtkHierarchicalBoxDataSet;

class VTK_EXPORT vtkHierarchicalBoxVisitor : public vtkCompositeDataVisitor
{
public:
  static vtkHierarchicalBoxVisitor *New();

  vtkTypeRevisionMacro(vtkHierarchicalBoxVisitor,vtkCompositeDataVisitor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Apply the command on each object in the collection.
  virtual void Execute();

  // Description:
  // Set the data object to iterator over.
  void SetDataSet(vtkHierarchicalBoxDataSet* dataset);
  vtkGetObjectMacro(DataSet, vtkHierarchicalBoxDataSet);

protected:
  vtkHierarchicalBoxVisitor(); 
  virtual ~vtkHierarchicalBoxVisitor(); 

  vtkHierarchicalBoxDataSet* DataSet;

private:
  vtkHierarchicalBoxVisitor(const vtkHierarchicalBoxVisitor&);  // Not implemented.
  void operator=(const vtkHierarchicalBoxVisitor&);  // Not implemented.
};

#endif

