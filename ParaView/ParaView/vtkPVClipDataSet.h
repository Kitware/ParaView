/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVClipDataSet.h
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
// .NAME vtkPVClipDataSet -

#ifndef __vtkPVClipDataSet_h
#define __vtkPVClipDataSet_h

#include "vtkClipDataSet.h"

class VTK_EXPORT vtkPVClipDataSet : public vtkClipDataSet
{
public:
  vtkTypeRevisionMacro(vtkPVClipDataSet,vtkClipDataSet);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  static vtkPVClipDataSet *New();

  // Description:
  // If you want to clip by an arbitrary array, then set its name here.
  // By default this in NULL and the filter will use the active scalar array.
  vtkGetStringMacro(InputScalarsSelection);
  void SelectInputScalars(const char *fieldName) 
    {this->SetInputScalarsSelection(fieldName);}

protected:
  vtkPVClipDataSet(vtkImplicitFunction *cf=NULL);
  ~vtkPVClipDataSet();

private:
  vtkPVClipDataSet(const vtkPVClipDataSet&);  // Not implemented.
  void operator=(const vtkPVClipDataSet&);  // Not implemented.
};

#endif
