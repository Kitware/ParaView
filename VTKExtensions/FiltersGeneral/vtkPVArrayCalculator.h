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
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports

class vtkDataObject;
class vtkDataSetAttributes;

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPVArrayCalculator : public vtkArrayCalculator
{
public:
  vtkTypeMacro(vtkPVArrayCalculator, vtkArrayCalculator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPVArrayCalculator* New();

protected:
  vtkPVArrayCalculator();
  ~vtkPVArrayCalculator() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Get the attribute type.
   */
  int GetAttributeTypeFromInput(vtkDataObject* input);

  /**
   * Clears the array and variable names.
   */
  void ResetArrayAndVariableNames();

  /**
   * Add coordinate variable names.
   */
  void AddCoordinateVariableNames();

  /**
   * This function adds the scalar and vector arrays as variables available
   * to the calculator. It can be called multiple times for the datasets in
   * a vtkCompositeDataSet. Argument inDataAttrs refers to the attributes of
   * the input dataset. This function should be called by RequestData() only.
   * Use ResetArrayAndVariableNames() prior to clear out previously set variable
   * names.
   */
  void AddArrayAndVariableNames(vtkDataObject* theInputObj, vtkDataSetAttributes* inDataAttrs);

private:
  vtkPVArrayCalculator(const vtkPVArrayCalculator&) = delete;
  void operator=(const vtkPVArrayCalculator&) = delete;
};
//@}

#endif
