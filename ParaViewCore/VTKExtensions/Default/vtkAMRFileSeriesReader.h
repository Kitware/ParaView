/*=========================================================================

  Program:   ParaView
  Module:    vtkFileSeriesReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkAMRFileSeriesReader_h
#define __vtkAMRFileSeriesReader_h

#include "vtkFileSeriesReader.h"

class VTK_EXPORT vtkAMRFileSeriesReader : public vtkFileSeriesReader
{
public:
  static vtkAMRFileSeriesReader* New();
  vtkTypeMacro(vtkAMRFileSeriesReader, vtkFileSeriesReader);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);

  virtual int RequestUpdateTime (vtkInformation*,
                                  vtkInformationVector**,
                                 vtkInformationVector*);


  virtual int RequestUpdateTimeDependentInformation (vtkInformation*,
                                                     vtkInformationVector**,
                                                     vtkInformationVector*);


private:
  vtkAMRFileSeriesReader();
  vtkAMRFileSeriesReader(const vtkAMRFileSeriesReader&); // Not implemented.
  void operator=(const vtkAMRFileSeriesReader&); // Not implemented.
};

#endif
