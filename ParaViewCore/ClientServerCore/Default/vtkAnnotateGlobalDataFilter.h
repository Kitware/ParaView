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
// .NAME vtkAnnotateGlobalDataFilter - filter for annotating with global data
// (designed for ExodusII reader).
// .SECTION Description
// vtkAnnotateGlobalDataFilter provides a simpler API for creating text
// annotations using vtkPythonAnnotationFilter. Instead of users specifying the
// annotation expression, this filter determines the expression based on the
// array selected by limiting the scope of the functionality. This filter only
// allows the user to annotate using "global-data" aka field data and specify
// the string prefix to use.
// If the field array chosen has as many elements as number of timesteps, the
// array is assumed to be "temporal" and indexed using the current timestep.

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

protected:
  vtkAnnotateGlobalDataFilter();
  ~vtkAnnotateGlobalDataFilter();

  virtual void EvaluateExpression();

  char* Prefix;
  char* FieldArrayName;
private:
  vtkAnnotateGlobalDataFilter(const vtkAnnotateGlobalDataFilter&); // Not implemented
  void operator=(const vtkAnnotateGlobalDataFilter&); // Not implemented
};

#endif
