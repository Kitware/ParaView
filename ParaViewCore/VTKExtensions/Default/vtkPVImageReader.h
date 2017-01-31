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
/**
 * @class   vtkPVImageReader
 * @brief   ImageReader that automatically switch between
 * vtkMPIImageReader or vtkImageReader based on the build setup.
*/

#ifndef vtkPVImageReader_h
#define vtkPVImageReader_h

#include "vtkPVConfig.h"                     // for PARAVIEW_USE_MPI
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

#ifdef PARAVIEW_USE_MPI
#include "vtkMPIImageReader.h" // For MPI-enabled builds
#else
#include "vtkImageReader.h" // For non-MPI builds
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

  virtual int CanReadFile(const char*) VTK_OVERRIDE { return 1; }

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkPVImageReader();
  ~vtkPVImageReader();

private:
  vtkPVImageReader(const vtkPVImageReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVImageReader&) VTK_DELETE_FUNCTION;
};

#endif

// VTK-HeaderTest-Exclude: vtkPVImageReader.h
