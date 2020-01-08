/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPGenericIOMultiBlockWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPGenericIOMultiBlockWriter
 *
*/

#ifndef vtkPGenericIOMultiBlockWriter_h
#define vtkPGenericIOMultiBlockWriter_h

#include "vtkPVVTKExtensionsCosmoToolsModule.h" // for export macro
#include "vtkWriter.h"

class vtkMultiProcessController;

class VTKPVVTKEXTENSIONSCOSMOTOOLS_EXPORT vtkPGenericIOMultiBlockWriter : public vtkWriter
{
public:
  static vtkPGenericIOMultiBlockWriter* New();
  vtkTypeMacro(vtkPGenericIOMultiBlockWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkPGenericIOMultiBlockWriter();
  ~vtkPGenericIOMultiBlockWriter();

  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual void WriteData();

private:
  class vtkInternals;
  vtkInternals* Internals;
  char* FileName;
  vtkMultiProcessController* Controller;
  vtkPGenericIOMultiBlockWriter(const vtkPGenericIOMultiBlockWriter&) = delete;
  void operator=(const vtkPGenericIOMultiBlockWriter&) = delete;
};

#endif
