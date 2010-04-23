/*=========================================================================

  Program:   ParaView
  Module:    vtkCPNodalFieldBuilder.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCPNodalFieldBuilder - Class for specifying nodal fields over grids.
// .SECTION Description
// Class for specifying nodal fields over grids for a test driver.  

#ifndef __vtkCPNodalFieldBuilder_h
#define __vtkCPNodalFieldBuilder_h

#include "vtkCPFieldBuilder.h"

class VTK_EXPORT vtkCPNodalFieldBuilder : public vtkCPFieldBuilder
{
public:
  static vtkCPNodalFieldBuilder * New();
  vtkTypeMacro(vtkCPNodalFieldBuilder, vtkCPFieldBuilder);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return a field on Grid. 
  virtual void BuildField(unsigned long timeStep, double time,
                          vtkDataSet* grid);

protected:
  vtkCPNodalFieldBuilder();
  ~vtkCPNodalFieldBuilder();

private:
  vtkCPNodalFieldBuilder(const vtkCPNodalFieldBuilder&); // Not implemented
  void operator=(const vtkCPNodalFieldBuilder&); // Not implemented
};

#endif
