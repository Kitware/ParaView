/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVArrayCalculator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVArrayCalculator
 * @brief   perform mathematical operations on data
 *  in field data arrays
 *
 *
 *  vtkPVArrayCalculator performs operations on vectors or scalars in field
 *  data arrays.
 *  vtkArrayCalculator provides API for users to add scalar/vector fields and
 *  their mapping with the input fields. We extend vtkArrayCalculator to
 *  automatically add scalar/vector fields mapping using the array available in
 *  the input.
 * @sa
 *  vtkArrayCalculator vtkFunctionParser
*/

#ifndef vtkPVArrayCalculator_h
#define vtkPVArrayCalculator_h

#include "vtkArrayCalculator.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class vtkDataObject;
class vtkDataSetAttributes;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVArrayCalculator : public vtkArrayCalculator
{
public:
  vtkTypeMacro(vtkPVArrayCalculator, vtkArrayCalculator);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkPVArrayCalculator* New();

protected:
  vtkPVArrayCalculator();
  ~vtkPVArrayCalculator();

  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  //@{
  /**
   * This function updates the (scalar and vector arrays / variables) names
   * to make them consistent with those of the upstream calculator(s). This
   * addresses the scenarios where the user modifies the name of a calculator
   * whose output is the input of a (some) subsequent calculator(s) or the user
   * changes the input of a downstream calculator. Argument inDataAttrs refers
   * to the attributes of the input dataset. This function should be called by
   * RequestData() only.
   */
  void UpdateArrayAndVariableNames(vtkDataObject* theInputObj, vtkDataSetAttributes* inDataAttrs);

private:
  vtkPVArrayCalculator(const vtkPVArrayCalculator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVArrayCalculator&) VTK_DELETE_FUNCTION;
};
//@}

#endif
