/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVKitwareContourFilter.h
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
// .NAME vtkPVKitwareContourFilter -
#ifndef __vtkPVKitwareContourFilter_h
#define __vtkPVKitwareContourFilter_h

#include "vtkKitwareContourFilter.h"

class VTK_EXPORT vtkPVKitwareContourFilter : public vtkKitwareContourFilter
{
public:
  vtkTypeRevisionMacro(vtkPVKitwareContourFilter,vtkKitwareContourFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  static vtkPVKitwareContourFilter *New();

  // Description:
  // If you want to contour by an arbitrary array, then set its name here.
  // By default this in NULL and the filter will use the active scalar array.
  vtkGetStringMacro(InputScalarsSelection);
  void SelectInputScalars(const char *fieldName) 
    {this->SetInputScalarsSelection(fieldName);}
  
protected:
  vtkPVKitwareContourFilter();
  ~vtkPVKitwareContourFilter();

private:
  vtkPVKitwareContourFilter(const vtkPVKitwareContourFilter&);  // Not implemented.
  void operator=(const vtkPVKitwareContourFilter&);  // Not implemented.
};


#endif


