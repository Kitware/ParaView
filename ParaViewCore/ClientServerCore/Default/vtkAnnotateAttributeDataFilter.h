/*=========================================================================

  Program:   ParaView
  Module:    vtkAnnotateAttributeDataFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAnnotateAttributeDataFilter - specialization of
// vtkPythonAnnotationFilter to work with element data.
// .SECTION Description
// vtkAnnotateAttributeDataFilter is a specialization of
// vtkPythonAnnotationFilter which makes it easier to annotate using data values
// from any input dataset.

#ifndef vtkAnnotateAttributeDataFilter_h
#define vtkAnnotateAttributeDataFilter_h

#include "vtkPythonAnnotationFilter.h"
#include "vtkPVClientServerCoreDefaultModule.h" //needed for exports

class VTKPVCLIENTSERVERCOREDEFAULT_EXPORT vtkAnnotateAttributeDataFilter : public vtkPythonAnnotationFilter
{
public:
  static vtkAnnotateAttributeDataFilter* New();
  vtkTypeMacro(vtkAnnotateAttributeDataFilter, vtkPythonAnnotationFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the attribute array name to annotate with.
  vtkSetStringMacro(ArrayName);
  vtkGetStringMacro(ArrayName);

  // Description:
  // Set the element number to annotate with.
  vtkSetMacro(ElementId, vtkIdType);
  vtkGetMacro(ElementId, vtkIdType);

  // Description:
  // Set the rank to extract the data from.
  // Default is 0.
  vtkSetMacro(ProcessId, int);
  vtkGetMacro(ProcessId, int);

  // Description:
  // Set the text prefix to display in front of the Field value
  vtkSetStringMacro(Prefix);
  vtkGetStringMacro(Prefix);
//BTX
protected:
  vtkAnnotateAttributeDataFilter();
  ~vtkAnnotateAttributeDataFilter();

  virtual void EvaluateExpression();

  char* ArrayName;
  char* Prefix;
  int ProcessId;
  vtkIdType ElementId;

private:
  vtkAnnotateAttributeDataFilter(const vtkAnnotateAttributeDataFilter&); // Not implemented
  void operator=(const vtkAnnotateAttributeDataFilter&); // Not implemented
//ETX
};

#endif
