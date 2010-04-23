/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamingFactory.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStreamingFactory - 
// .SECTION Description

#ifndef __vtkStreamingFactory_h
#define __vtkStreamingFactory_h

#include "vtkObjectFactory.h"

class VTK_EXPORT vtkStreamingFactory : public vtkObjectFactory
{
public: 

  virtual const char* GetVTKSourceVersion();
  virtual const char* GetDescription();

  // Methods from vtkObject
  vtkTypeMacro(vtkStreamingFactory, vtkObjectFactory);
  static vtkStreamingFactory *New();
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkStreamingFactory();
  ~vtkStreamingFactory();

private:
  vtkStreamingFactory(const vtkStreamingFactory&);  // Not implemented.
  void operator=(const vtkStreamingFactory&);  // Not implemented.
};


#endif
