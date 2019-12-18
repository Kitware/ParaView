/*=========================================================================

  Program:   ParaView
  Module:    vtkPVChangeOfBasisHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVChangeOfBasisHelper
 *
 * vtkPVChangeOfBasisHelper is designed for ORNL-SNS use-cases where we needed
 * to add support for different basis.
*/

#ifndef vtkPVChangeOfBasisHelper_h
#define vtkPVChangeOfBasisHelper_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsMiscModule.h" // needed for export macro

#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkVector.h"

#include <algorithm>

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

  //@{
  /**
   * Add basis titles to field data.
   */
  static bool AddBasisNames(
    vtkDataObject* dataObject, const char* utitle, const char* vtitle, const char* wtitle);
  //@}

  static void GetBasisName(
    vtkDataObject* dataObject, const char*& utitle, const char*& vtitle, const char*& wtitle);

  //@{
  /**
   * Add bounding box in model space.
   */
  static bool AddBoundingBoxInBasis(vtkDataObject* dataObject, const double bbox[6]);
  //@}

  static bool GetBoundingBoxInBasis(vtkDataObject* dataObject, double bbox[6]);
};

#endif
//****************************************************************************
// VTK-HeaderTest-Exclude: vtkPVChangeOfBasisHelper.h
