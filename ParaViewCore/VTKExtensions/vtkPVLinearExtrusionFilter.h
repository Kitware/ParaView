/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLinearExtrusionFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVLinearExtrusionFilter - change a default value
// .SECTION Description
// vtkPVLinearExtrusionFilter is a subclass of vtkPLinearExtrusionFilter.
// The only difference is changing the default extrusion type to vector
// extrusion

#ifndef __vtkPVLinearExtrusionFilter_h
#define __vtkPVLinearExtrusionFilter_h

#include "vtkPLinearExtrusionFilter.h"

class VTK_EXPORT vtkPVLinearExtrusionFilter : public vtkPLinearExtrusionFilter
{
public:
  static vtkPVLinearExtrusionFilter *New();
  vtkTypeMacro(vtkPVLinearExtrusionFilter, vtkPLinearExtrusionFilter);
  void PrintSelf(ostream &os, vtkIndent indent);
  
protected:
  vtkPVLinearExtrusionFilter();
  ~vtkPVLinearExtrusionFilter() {};
  
private:
  vtkPVLinearExtrusionFilter(const vtkPVLinearExtrusionFilter&); // Not implemented.
  void operator=(const vtkPVLinearExtrusionFilter&); // Not implemented.
};

#endif
