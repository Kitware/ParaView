/*=========================================================================

  Program:   ParaView
  Module:    vtkCSVReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCSVReader - reads CSV files into a one-dimensional 
// vtkRectilinearGrid.
// .SECTION Description:
// This extends vtkDelimitedTextReader to convert numerical columns to double
// values. This makes it easy for ParaView to deal with numerical data
// correctly.
// .SECTION See Also
// vtkDelimitedTextReader

#ifndef __vtkCSVReader_h
#define __vtkCSVReader_h

#include "vtkDelimitedTextReader.h"

class VTK_EXPORT vtkCSVReader : public vtkDelimitedTextReader
{
public:
  static vtkCSVReader* New();
  vtkTypeRevisionMacro(vtkCSVReader, vtkDelimitedTextReader);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkCSVReader();
  ~vtkCSVReader();
  virtual int RequestData(vtkInformation*, vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkCSVReader(const vtkCSVReader&); // Not implemented.
  void operator=(const vtkCSVReader&); // Not implemented.
};

#endif

