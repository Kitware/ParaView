/*=========================================================================

  Program:   ParaView
  Module:    vtkPVChangeOfBasisHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVChangeOfBasisHelper.h"

#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"

//----------------------------------------------------------------------------
vtkSmartPointer<vtkMatrix4x4> vtkPVChangeOfBasisHelper::GetChangeOfBasisMatrix(
  const vtkVector3d& u, const vtkVector3d& v, const vtkVector3d& w)
{
  vtkSmartPointer<vtkMatrix4x4> cobMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  cobMatrix->Identity();
  std::copy(u.GetData(), u.GetData() + 3, cobMatrix->Element[0]);
  std::copy(v.GetData(), v.GetData() + 3, cobMatrix->Element[1]);
  std::copy(w.GetData(), w.GetData() + 3, cobMatrix->Element[2]);
  cobMatrix->Transpose();
  return cobMatrix;
}

//----------------------------------------------------------------------------
bool vtkPVChangeOfBasisHelper::GetBasisVectors(
  vtkMatrix4x4* matrix, vtkVector3d& u, vtkVector3d& v, vtkVector3d& w)
{
  if (matrix)
  {
    double xaxis[4] = { 1, 0, 0, 1 };
    double yaxis[4] = { 0, 1, 0, 1 };
    double zaxis[4] = { 0, 0, 1, 1 };
    matrix->MultiplyPoint(xaxis, xaxis);
    matrix->MultiplyPoint(yaxis, yaxis);
    matrix->MultiplyPoint(zaxis, zaxis);
    for (int cc = 0; cc < 3; cc++)
    {
      xaxis[cc] /= xaxis[3];
      yaxis[cc] /= yaxis[3];
      zaxis[cc] /= zaxis[3];
    }
    u = vtkVector3d(xaxis);
    v = vtkVector3d(yaxis);
    w = vtkVector3d(zaxis);
    return true;
  }
  else
  {
    u = vtkVector3d(1, 0, 0);
    v = vtkVector3d(0, 1, 0);
    w = vtkVector3d(0, 0, 1);
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkPVChangeOfBasisHelper::AddChangeOfBasisMatrixToFieldData(
  vtkDataObject* dataObject, vtkMatrix4x4* matrix)
{
  if (dataObject && matrix)
  {
    vtkNew<vtkDoubleArray> cobArray;
    cobArray->SetName("ChangeOfBasisMatrix");
    cobArray->SetNumberOfComponents(16);
    cobArray->SetNumberOfTuples(1);
    std::copy(&matrix->Element[0][0], (&matrix->Element[0][0]) + 16, cobArray->GetPointer(0));
    dataObject->GetFieldData()->AddArray(cobArray.GetPointer());
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkMatrix4x4> vtkPVChangeOfBasisHelper::GetChangeOfBasisMatrix(
  vtkDataObject* dataObject)
{
  if (dataObject && dataObject->GetFieldData() &&
    dataObject->GetFieldData()->GetArray("ChangeOfBasisMatrix"))
  {
    vtkDataArray* array = dataObject->GetFieldData()->GetArray("ChangeOfBasisMatrix");
    if (array && array->GetNumberOfComponents() == 16 && array->GetNumberOfTuples() == 1)
    {
      vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
      array->GetTuple(0, &mat->Element[0][0]);
      return mat;
    }
  }
  return vtkSmartPointer<vtkMatrix4x4>();
}

//----------------------------------------------------------------------------
bool vtkPVChangeOfBasisHelper::AddBasisNames(
  vtkDataObject* dataObject, const char* utitle, const char* vtitle, const char* wtitle)
{
  if (!dataObject)
  {
    return false;
  }
  if (utitle)
  {
    vtkNew<vtkStringArray> uAxisTitle;
    uAxisTitle->SetName("AxisTitleForX");
    uAxisTitle->SetNumberOfComponents(1);
    uAxisTitle->SetNumberOfTuples(1);
    uAxisTitle->SetValue(0, utitle);
    uAxisTitle->SetComponentName(0, utitle);
    dataObject->GetFieldData()->AddArray(uAxisTitle.GetPointer());
  }
  if (vtitle)
  {
    vtkNew<vtkStringArray> vAxisTitle;
    vAxisTitle->SetName("AxisTitleForY");
    vAxisTitle->SetNumberOfComponents(1);
    vAxisTitle->SetNumberOfTuples(1);
    vAxisTitle->SetValue(0, vtitle);
    vAxisTitle->SetComponentName(0, vtitle);
    dataObject->GetFieldData()->AddArray(vAxisTitle.GetPointer());
  }
  if (wtitle)
  {
    vtkNew<vtkStringArray> wAxisTitle;
    wAxisTitle->SetName("AxisTitleForZ");
    wAxisTitle->SetNumberOfComponents(1);
    wAxisTitle->SetNumberOfTuples(1);
    wAxisTitle->SetValue(0, wtitle);
    wAxisTitle->SetComponentName(0, wtitle);
    dataObject->GetFieldData()->AddArray(wAxisTitle.GetPointer());
  }
  return (utitle || vtitle || wtitle);
}

//----------------------------------------------------------------------------
void vtkPVChangeOfBasisHelper::GetBasisName(
  vtkDataObject* dataObject, const char*& utitle, const char*& vtitle, const char*& wtitle)
{
  utitle = vtitle = wtitle = NULL;
  if (!dataObject || !dataObject->GetFieldData())
  {
    return;
  }
  if (vtkStringArray* uarray =
        vtkStringArray::SafeDownCast(dataObject->GetFieldData()->GetAbstractArray("AxisTitleForX")))
  {
    if (uarray->GetNumberOfValues() == 1)
    {
      utitle = uarray->GetValue(0).c_str();
    }
  }
  if (vtkStringArray* varray =
        vtkStringArray::SafeDownCast(dataObject->GetFieldData()->GetAbstractArray("AxisTitleForY")))
  {
    if (varray->GetNumberOfValues() == 1)
    {
      vtitle = varray->GetValue(0).c_str();
    }
  }
  if (vtkStringArray* warray =
        vtkStringArray::SafeDownCast(dataObject->GetFieldData()->GetAbstractArray("AxisTitleForZ")))
  {
    if (warray->GetNumberOfValues() == 1)
    {
      wtitle = warray->GetValue(0).c_str();
    }
  }
}

//----------------------------------------------------------------------------
bool vtkPVChangeOfBasisHelper::AddBoundingBoxInBasis(
  vtkDataObject* dataObject, const double bbox[6])
{
  if (dataObject)
  {
    vtkNew<vtkDoubleArray> bounds;
    bounds->SetName("BoundingBoxInModelCoordinates");
    bounds->SetNumberOfComponents(6);
    bounds->SetNumberOfTuples(1);
    std::copy(bbox, bbox + 6, bounds->GetPointer(0));
    dataObject->GetFieldData()->AddArray(bounds.GetPointer());
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkPVChangeOfBasisHelper::GetBoundingBoxInBasis(vtkDataObject* dataObject, double bbox[6])
{
  if (dataObject && dataObject->GetFieldData() &&
    dataObject->GetFieldData()->GetArray("BoundingBoxInModelCoordinates"))
  {
    vtkDataArray* array = dataObject->GetFieldData()->GetArray("BoundingBoxInModelCoordinates");
    if (array && array->GetNumberOfTuples() == 1 && array->GetNumberOfComponents() == 6)
    {
      array->GetTuple(0, bbox);
      return true;
    }
  }
  return false;
}
