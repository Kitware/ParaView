// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVChangeOfBasisHelper
 *
 * vtkPVChangeOfBasisHelper is designed for ORNL-SNS use-cases where we needed
 * to add support for different basis.
 */

#ifndef vtkPVChangeOfBasisHelper_h
#define vtkPVChangeOfBasisHelper_h

#include "vtkPVVTKExtensionsMiscModule.h" // needed for export macro

#include "vtkMatrix4x4.h"    // for vtkMatrix4x4
#include "vtkSmartPointer.h" // for vtkSmartPointer
#include "vtkVector.h"       // for vtkVector

class vtkDataObject;

class VTKPVVTKEXTENSIONSMISC_EXPORT vtkPVChangeOfBasisHelper
{
public:
  /**
   * Given a set of basis vectors, returns the change-of-basis matrix.
   */
  static vtkSmartPointer<vtkMatrix4x4> GetChangeOfBasisMatrix(
    const vtkVector3d& u, const vtkVector3d& v, const vtkVector3d& w);

  static bool GetBasisVectors(vtkMatrix4x4* matrix, vtkVector3d& u, vtkVector3d& v, vtkVector3d& w);

  static bool AddChangeOfBasisMatrixToFieldData(vtkDataObject* dataObject, vtkMatrix4x4* matrix);

  static vtkSmartPointer<vtkMatrix4x4> GetChangeOfBasisMatrix(vtkDataObject* dataObject);

  ///@{
  /**
   * Add basis titles to field data.
   */
  static bool AddBasisNames(
    vtkDataObject* dataObject, const char* utitle, const char* vtitle, const char* wtitle);
  ///@}

  static void GetBasisName(
    vtkDataObject* dataObject, const char*& utitle, const char*& vtitle, const char*& wtitle);

  ///@{
  /**
   * Add bounding box in model space.
   */
  static bool AddBoundingBoxInBasis(vtkDataObject* dataObject, const double bbox[6]);
  ///@}

  static bool GetBoundingBoxInBasis(vtkDataObject* dataObject, double bbox[6]);
};

#endif
//****************************************************************************
// VTK-HeaderTest-Exclude: vtkPVChangeOfBasisHelper.h
