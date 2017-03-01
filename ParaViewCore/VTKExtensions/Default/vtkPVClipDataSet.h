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

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkTableBasedClipDataSet.h"

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVClipDataSet : public vtkTableBasedClipDataSet
{
public:
  vtkTypeMacro(vtkPVClipDataSet, vtkTableBasedClipDataSet);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkPVClipDataSet* New();

  virtual int ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  //@{
  /**
   * This filter uses vtkAMRDualClip for clipping AMR datasets. Do disable that
   * behavior, turn this flag off.
   */
  vtkSetMacro(UseAMRDualClipForAMR, bool);
  vtkGetMacro(UseAMRDualClipForAMR, bool);
  vtkBooleanMacro(UseAMRDualClipForAMR, bool);
  //@}

protected:
  vtkPVClipDataSet(vtkImplicitFunction* cf = NULL);
  ~vtkPVClipDataSet();

  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  virtual int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  virtual int FillInputPortInformation(int, vtkInformation* info) VTK_OVERRIDE;
  virtual int FillOutputPortInformation(int, vtkInformation* info) VTK_OVERRIDE;

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

private:
  vtkPVClipDataSet(const vtkPVClipDataSet&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVClipDataSet&) VTK_DELETE_FUNCTION;
};

#endif
