/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVThresholdFilter.h
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
// .NAME vtkPVThresholdFilter - 
#ifndef __vtkPVThresholdFilter_h
#define __vtkPVThresholdFilter_h

#include "vtkThreshold.h"

class VTK_EXPORT vtkPVThresholdFilter : public vtkThreshold
{
public:
  static vtkPVThresholdFilter *New();
  vtkTypeRevisionMacro(vtkPVThresholdFilter,vtkThreshold);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // If you want to threshold by an arbitrary array, then set its name here.
  // By default this in NULL and the filter will use the active scalar array.
  vtkGetStringMacro(InputScalarsSelection);
  void SelectInputScalars(const char *fieldName) 
    {this->SetInputScalarsSelection(fieldName);}
  
protected:
  vtkPVThresholdFilter();
  ~vtkPVThresholdFilter();

private:
  vtkPVThresholdFilter(const vtkPVThresholdFilter&);  // Not implemented.
  void operator=(const vtkPVThresholdFilter&);  // Not implemented.
};

#endif
