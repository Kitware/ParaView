

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPostFilterExecutive.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVPostFilterExecutive
 * @brief   Executive supporting post filters.
 *
 * vtkPVPostFilterExecutive is an executive that supports the creation of
 * post filters for the following uses cases:
 * Provide the ability to automatically use a vector component as a scalar
 * input property.
 *
 * Interpolate cell centered data to point data, and the inverse if needed
 * by the filter.
*/

#ifndef vtkPVPostFilterExecutive_h
#define vtkPVPostFilterExecutive_h

#include "vtkPVCompositeDataPipeline.h"

class vtkInformationInformationVectorKey;
class vtkInformationStringVectorKey;

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkPVPostFilterExecutive : public vtkPVCompositeDataPipeline
{
public:
  static vtkPVPostFilterExecutive* New();
  vtkTypeMacro(vtkPVPostFilterExecutive, vtkPVCompositeDataPipeline);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkInformationInformationVectorKey* POST_ARRAYS_TO_PROCESS();
  static vtkInformationStringVectorKey* POST_ARRAY_COMPONENT_KEY();

  /**
   * Returns the data object stored with the DATA_OBJECT() in the
   * input port
   */
  vtkDataObject* GetCompositeInputData(int port, int index, vtkInformationVector** inInfoVec);

  vtkInformation* GetPostArrayToProcessInformation(int idx);
  void SetPostArrayToProcessInformation(int idx, vtkInformation* inInfo);

protected:
  vtkPVPostFilterExecutive();
  ~vtkPVPostFilterExecutive() override;

  // Overridden to always return true
  int NeedToExecuteData(
    int outputPort, vtkInformationVector** inInfoVec, vtkInformationVector* outInfoVec) override;

  bool MatchingPropertyInformation(vtkInformation* inputArrayInfo, vtkInformation* postArrayInfo);

private:
  vtkPVPostFilterExecutive(const vtkPVPostFilterExecutive&) = delete;
  void operator=(const vtkPVPostFilterExecutive&) = delete;
};

#endif
