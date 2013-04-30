/*=========================================================================

  Program:   ParaView
  Module:    vtkPVImageReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVImageReader - ImageReader that automatically switch between
// vtkMPIImageReader or vtkImageReader based on the build setup.
#ifndef __vtkPVImageReader_h
#define __vtkPVImageReader_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

#ifdef PARAVIEW_USE_MPI
#include "vtkMPIImageReader.h"
#else
#include "vtkImageReader.h"
#endif

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVImageReader : public
#ifdef PARAVIEW_USE_MPI
vtkMPIImageReader
#else
vtkImageReader
#endif
{
public:
  static vtkPVImageReader* New();

#ifdef PARAVIEW_USE_MPI
  vtkTypeMacro(vtkPVImageReader, vtkMPIImageReader);
#else
  vtkTypeMacro(vtkPVImageReader, vtkImageReader);
#endif

  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPVImageReader();
  ~vtkPVImageReader();

private:
  vtkPVImageReader(const vtkPVImageReader&); // Not implemented.
  void operator=(const vtkPVImageReader&); // Not implemented.
//ETX
};

#endif
