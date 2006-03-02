/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPPickFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPPickFilter - This is a pick filter which collects the result
// on the root node.
// .SECTION Description
// This is same as vtkPickFilter except that this class collects the result 
// on the root node on every execute. This is done to ensure similar behaviour
// to that of vtkPProbeFilter.

#ifndef __vtkPPickFilter_h
#define __vtkPPickFilter_h

#include "vtkPickFilter.h"

class VTK_EXPORT vtkPPickFilter : public vtkPickFilter
{
public:
  static vtkPPickFilter* New();
  vtkTypeRevisionMacro(vtkPPickFilter, vtkPickFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPPickFilter();
  ~vtkPPickFilter();

  // We only need to collect for IdExecute, since when picking with
  // world point, the super class already does the collection.
  virtual void IdExecute();
private:
  vtkPPickFilter(const vtkPPickFilter&); // Not implemented.
  void operator=(const vtkPPickFilter&); // Not implemented.
};


#endif

