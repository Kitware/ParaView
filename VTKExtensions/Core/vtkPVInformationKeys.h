/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVInformationKeys.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVInformationKeys
 *
*/

#ifndef vtkPVInformationKeys_h
#define vtkPVInformationKeys_h

#include "vtkPVVTKExtensionsCoreModule.h" // needed for export macro

class vtkInformationStringKey;
class vtkInformationDoubleVectorKey;

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkPVInformationKeys
{
public:
  /**
   * Key to store the label that should be used for labeling the time in the UI
   */
  static vtkInformationStringKey* TIME_LABEL_ANNOTATION();

  //@{
  /**
   * Key to store the bounding box of the entire data set in pipeline
   * information.
   */
  static vtkInformationDoubleVectorKey* WHOLE_BOUNDING_BOX();
};
//@}

#endif // vtkPVInformationKeys_h
// VTK-HeaderTest-Exclude: vtkPVInformationKeys.h
