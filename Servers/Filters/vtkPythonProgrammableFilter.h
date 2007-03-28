/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonProgrammableFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPythonProgrammableFilter - Executes a user supplied python script 
// on its input dataset to produce an output dataset.
// .SECTION Description
// This filter will execute a python script to produce an output dataset.
// The filter keeps a copy of the python script in Script, and creates 
// Interpretor, a python interpretor to run the script in upon the first 
// execution. Use SetOutputDataSetType to choose and output data type instead
// of using the default of using the same type as is on the input.

#ifndef __vtkPythonProgrammableFilter_h
#define __vtkPythonProgrammableFilter_h

#include "vtkProgrammableFilter.h"

class vtkPVPythonInterpretor;

class VTK_EXPORT vtkPythonProgrammableFilter : public vtkProgrammableFilter
{
public:
  vtkTypeRevisionMacro(vtkPythonProgrammableFilter,vtkProgrammableFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkPythonProgrammableFilter *New();

  //Description: Set the text of the python script to execute.
  void SetScript(const char *script);

  //Description: 
  //For internal use only.
  static void ExecuteScript(void *);

  //Description:
  //For internal use only.
  void Exec();

  // Description:
  // Changes the output data set type.
  // Allowable values are defined in vtkType.h
  vtkSetMacro(OutputDataSetType, int);
  vtkGetMacro(OutputDataSetType, int);

protected:
  vtkPythonProgrammableFilter();
  ~vtkPythonProgrammableFilter();

  //overridden to allow multiple inputs to port 0
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  // Description:
  // Creates whatever output data set type is selected.
  virtual int RequestDataObject(vtkInformation* request, 
                                vtkInformationVector** inputVector, 
                                vtkInformationVector* outputVector);
  

  char *Script;
  vtkPVPythonInterpretor* Interpretor;
  int OutputDataSetType;

private:
  vtkPythonProgrammableFilter(const vtkPythonProgrammableFilter&);  // Not implemented.
  void operator=(const vtkPythonProgrammableFilter&);  // Not implemented.
};

#endif
