/*=========================================================================
  
  Program:   ParaView
  Module:    vtkPVBatchOptions.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVBatchOptions - ParaView options storage
// .SECTION Description
// An object of this class represents a storage for ParaView options
// 
// These options can be retrieved during run-time, set using configuration file
// or using Command Line Arguments.

#ifndef __vtkPVBatchOptions_h
#define __vtkPVBatchOptions_h

#include "vtkPVOptions.h"

class VTK_EXPORT vtkPVBatchOptions : public vtkPVOptions
{
public:
  static vtkPVBatchOptions* New();
  vtkTypeMacro(vtkPVBatchOptions,vtkPVOptions);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkGetStringMacro(BatchScriptName);

protected:
  // Description:
  // Default constructor.
  vtkPVBatchOptions();

  // Description:
  // Destructor.
  virtual ~vtkPVBatchOptions();

  // Description:
  // Initialize arguments.
  virtual void Initialize();

  // Description:
  // After parsing, process extra option dependencies.
  virtual int PostProcess(int argc, const char* const* argv);

  // Description:
  // This method is called when wrong argument is found. If it returns 0, then
  // the parsing will fail.
  virtual int WrongArgument(const char* argument);

  // Options:
  vtkSetStringMacro(BatchScriptName);
  char* BatchScriptName;

  int RequireBatchScript;

private:
  vtkPVBatchOptions(const vtkPVBatchOptions&); // Not implemented
  void operator=(const vtkPVBatchOptions&); // Not implemented
};

#endif // #ifndef __vtkPVBatchOptions_h


