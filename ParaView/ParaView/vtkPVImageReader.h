/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageReader.h
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
// .NAME vtkPVImageReader - Superclass of transformable binary file readers.
// .SECTION Description

#ifndef __vtkPVImageReader_h
#define __vtkPVImageReader_h

#include "vtkImageReader.h"

class VTK_EXPORT vtkPVImageReader : public vtkImageReader
{
public:
  static vtkPVImageReader *New();
  vtkTypeRevisionMacro(vtkPVImageReader,vtkImageReader);
  void PrintSelf(ostream& os, vtkIndent indent);   

protected:
  vtkPVImageReader();
  ~vtkPVImageReader() {};

private:
  vtkPVImageReader(const vtkPVImageReader&);  // Not implemented.
  void operator=(const vtkPVImageReader&);  // Not implemented.
};

#endif
