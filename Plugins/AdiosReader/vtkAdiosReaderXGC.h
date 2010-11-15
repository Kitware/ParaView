/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAdiosReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAdiosReader - Base class Reader for ADIOS file format.
// .SECTION Description
//

#ifndef __vtkAdiosReader_h
#define __vtkAdiosReader_h

#include "vtkCompositeDataSetAlgorithm.h"

class vtkDataSetAttributes;
class vtkInformationVector;
class vtkInformation;

class VTK_IO_EXPORT vtkAdiosReader : public vtkCompositeDataSetAlgorithm
{
public:
  static vtkAdiosReader *New();
  vtkTypeMacro(vtkAdiosReader,vtkCompositeDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the name of the input file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Decription:
  // Set Read method
  void SetReadMethodToBP();
  void SetReadMethodToDART();
  void SetReadMethod(int methodEnum);

  // Description:
  // Set the stage application ID
  void SetAdiosApplicationId(int id);

  // Description:
  // Modified the reader to request a new read (from stage), so you can
  // move forward in time.
  void PollForNewTimeSteps();

  // Description:
  // Test whether the file with the given name can be read by this
  // reader.
  virtual int CanReadFile(const char* name);

protected:
  vtkAdiosReader();
  ~vtkAdiosReader();

  // call 1
  virtual int RequestDataObject(vtkInformation *,
                                vtkInformationVector** vtkNotUsed(inputVector),
                                vtkInformationVector* outputVector);

  // call 2
  virtual int RequestInformation(vtkInformation *,
                                 vtkInformationVector **,
                                 vtkInformationVector *outputVector);

  // call 3
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  // call 4
  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *outputVector);

//  virtual int FillOutputPortInformation(int, vtkInformation *);

  int ReadOutputType();

  // The input file's name.
  char* FileName;

//BTX

private:
  vtkAdiosReader(const vtkAdiosReader&);  // Not implemented.
  void operator=(const vtkAdiosReader&);  // Not implemented.

  class Internals;
  Internals* Internal;
//ETX
};

#endif
