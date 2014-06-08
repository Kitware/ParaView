/*=========================================================================

  Program:   ParaView
  Module:    vtkAnnotateGlobalDataFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAnnotateGlobalDataFilter
// .SECTION Description
// This filter is a helper to vtkPythonAnnotationFilter by providing a simpler
// API and allowing the user to simply display FieldData without wondering
// of the Python syntax.

#ifndef __vtkAnnotateGlobalDataFilter_h
#define __vtkAnnotateGlobalDataFilter_h

#include "vtkPVClientServerCoreDefaultModule.h" //needed for exports
#include "vtkPythonAnnotationFilter.h"

class VTKPVCLIENTSERVERCOREDEFAULT_EXPORT vtkAnnotateGlobalDataFilter : public vtkPythonAnnotationFilter
{
public:
  static vtkAnnotateGlobalDataFilter* New();
  vtkTypeMacro(vtkAnnotateGlobalDataFilter, vtkPythonAnnotationFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Name of the field to display
  vtkSetStringMacro(FieldArrayName);
  vtkGetStringMacro(FieldArrayName);


  // Description:
  // Set the text prefix to display in front of the Field value
  vtkSetStringMacro(Prefix);
  vtkGetStringMacro(Prefix);
//BTX
protected:
  vtkAnnotateGlobalDataFilter();
  ~vtkAnnotateGlobalDataFilter();

  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  char* Prefix;
  char* FieldArrayName;

private:
  vtkAnnotateGlobalDataFilter(const vtkAnnotateGlobalDataFilter&); // Not implemented
  void operator=(const vtkAnnotateGlobalDataFilter&); // Not implemented

//ETX
};

#endif
