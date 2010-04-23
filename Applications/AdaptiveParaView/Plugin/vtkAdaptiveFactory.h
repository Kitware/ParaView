/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAdaptiveFactory.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAdaptiveFactory - 
// .SECTION Description

#ifndef __vtkAdaptiveFactory_h
#define __vtkAdaptiveFactory_h

#include "vtkObjectFactory.h"

class VTK_EXPORT vtkAdaptiveFactory : public vtkObjectFactory
{
public: 

  virtual const char* GetVTKSourceVersion();
  virtual const char* GetDescription();

  // Methods from vtkObject
  vtkTypeMacro(vtkAdaptiveFactory, vtkObjectFactory);
  static vtkAdaptiveFactory *New();
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkAdaptiveFactory();
  ~vtkAdaptiveFactory();

private:
  vtkAdaptiveFactory(const vtkAdaptiveFactory&);  // Not implemented.
  void operator=(const vtkAdaptiveFactory&);  // Not implemented.
};


#endif
