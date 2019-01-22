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
/**
 * @class   vtkAnnotateGlobalDataFilter
 * @brief   filter for annotating with global data
 * (designed for ExodusII reader).
 *
 * vtkAnnotateGlobalDataFilter provides a simpler API for creating text
 * annotations using vtkPythonAnnotationFilter. Instead of users specifying the
 * annotation expression, this filter determines the expression based on the
 * array selected by limiting the scope of the functionality. This filter only
 * allows the user to annotate using "global-data" aka field data and specify
 * the string prefix to use.
 * If the field array chosen has as many elements as number of timesteps, the
 * array is assumed to be "temporal" and indexed using the current timestep.
*/

#ifndef vtkAnnotateGlobalDataFilter_h
#define vtkAnnotateGlobalDataFilter_h

#include "vtkPVClientServerCorePythonModule.h" //needed for exports
#include "vtkPythonAnnotationFilter.h"

class VTKPVCLIENTSERVERCOREPYTHON_EXPORT vtkAnnotateGlobalDataFilter
  : public vtkPythonAnnotationFilter
{
public:
  static vtkAnnotateGlobalDataFilter* New();
  vtkTypeMacro(vtkAnnotateGlobalDataFilter, vtkPythonAnnotationFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Name of the field to display
   */
  vtkSetStringMacro(FieldArrayName);
  vtkGetStringMacro(FieldArrayName);
  //@}

  //@{
  /**
   * Set the text prefix to display in front of the Field value
   */
  vtkSetStringMacro(Prefix);
  vtkGetStringMacro(Prefix);
  //@}

  //@{
  /**
   * Set the text prefix to display in front of the Field value
   */
  vtkSetStringMacro(Postfix);
  vtkGetStringMacro(Postfix);
  //@}

  //@{
  /**
   * Set the format to use when displaying the field value
   */
  vtkSetStringMacro(Format);
  vtkGetStringMacro(Format);
  //@}

protected:
  vtkAnnotateGlobalDataFilter();
  ~vtkAnnotateGlobalDataFilter() override;

  void EvaluateExpression() override;

  char* Prefix;
  char* Postfix;
  char* FieldArrayName;
  char* Format;

private:
  vtkAnnotateGlobalDataFilter(const vtkAnnotateGlobalDataFilter&) = delete;
  void operator=(const vtkAnnotateGlobalDataFilter&) = delete;
};

#endif
