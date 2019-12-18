/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClipDataSet.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVClipDataSet
 * @brief   Clip filter
 *
 *
 * This is a subclass of vtkTableBasedClipDataSet that allows selection of input
 * scalars.
*/

#ifndef vtkPVClipDataSet_h
#define vtkPVClipDataSet_h

#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports
#include "vtkTableBasedClipDataSet.h"

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPVClipDataSet : public vtkTableBasedClipDataSet
{
public:
  vtkTypeMacro(vtkPVClipDataSet, vtkTableBasedClipDataSet);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPVClipDataSet* New();

  int ProcessRequest(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  //@{
  /**
   * This filter uses vtkAMRDualClip for clipping AMR datasets. To disable that
   * behavior, turn this flag off.
   */
  vtkSetMacro(UseAMRDualClipForAMR, bool);
  vtkGetMacro(UseAMRDualClipForAMR, bool);
  vtkBooleanMacro(UseAMRDualClipForAMR, bool);
  //@}

  //@{
  /**
   * For a vtkPVBlox implicit function we can do an exact clip of the exterior portion of the box.
   */
  vtkSetMacro(ExactBoxClip, bool);
  vtkGetMacro(ExactBoxClip, bool);
  vtkBooleanMacro(ExactBoxClip, bool);
  //@}

protected:
  vtkPVClipDataSet(vtkImplicitFunction* cf = NULL);
  ~vtkPVClipDataSet() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  virtual int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  int FillInputPortInformation(int, vtkInformation* info) override;
  int FillOutputPortInformation(int, vtkInformation* info) override;

  //@{
  /**
   * Uses superclass to clip the input. This also handles composite datasets
   * (since superclass does not handle composite datasets). This method loops
   * over the composite dataset calling superclass repeatedly.
   */
  int ClipUsingSuperclass(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);
  int ClipUsingThreshold(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);
  //@}

  bool UseAMRDualClipForAMR;
  bool ExactBoxClip;

private:
  vtkPVClipDataSet(const vtkPVClipDataSet&) = delete;
  void operator=(const vtkPVClipDataSet&) = delete;
};

#endif
