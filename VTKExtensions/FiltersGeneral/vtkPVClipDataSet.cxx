// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVClipDataSet.h"

#include "vtkCellData.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkDataSetTriangleFilter.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkGeometryFilter.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridAxisClip.h"
#include "vtkInformation.h"
#include "vtkInformationStringVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkMassProperties.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkNonMergingPointLocator.h"
#include "vtkObjectFactory.h"
#include "vtkPVBox.h"
#include "vtkPVCylinder.h"
#include "vtkPVThreshold.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkQuadric.h"
#include "vtkSmartPointer.h"
#include "vtkSphere.h"
#include "vtkTransform.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkPVClipDataSet);

//----------------------------------------------------------------------------
vtkPVClipDataSet::vtkPVClipDataSet(vtkImplicitFunction* vtkNotUsed(cf))
{
  // setting NumberOfOutputPorts to 1 because ParaView does not allow you to
  // generate the clipped output
  this->SetNumberOfOutputPorts(1);

  this->UseAMRDualClipForAMR = true;
  this->ExactBoxClip = false;
}

//----------------------------------------------------------------------------
vtkPVClipDataSet::~vtkPVClipDataSet() = default;

//----------------------------------------------------------------------------
void vtkPVClipDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseAMRDualClipForAMR: " << this->UseAMRDualClipForAMR << endl;
}

//----------------------------------------------------------------------------
int vtkPVClipDataSet::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  if (!inInfo)
  {
    vtkErrorMacro(<< "Failed to get input information.");
    return 0;
  }

  vtkDataObject* inDataObj = inInfo->Get(vtkDataObject::DATA_OBJECT());

  if (!inDataObj)
  {
    vtkErrorMacro(<< "Failed to get input data object.");
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    vtkErrorMacro(<< "Failed to get output information.");
  }

  vtkDataObject* outDataObj = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!outDataObj)
  {
    vtkErrorMacro(<< "Failed to get output data object.");
  }

  if (vtkDataSet::SafeDownCast(inDataObj)) // For vtkDataSet.
  {
    if (this->GetClipFunction())
    {
      return this->ClipUsingSuperclass(request, inputVector, outputVector);
    }

    vtkDataSet* ds = vtkDataSet::SafeDownCast(inDataObj);
    if (!ds)
    {
      vtkErrorMacro("Failed to get vtkDataSet.");
      return 1;
    }

    int association = this->GetInputArrayAssociation(0, ds);
    // If using point scalars.
    if (association == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
      return this->ClipUsingSuperclass(request, inputVector, outputVector);
    } // End if using point scalars.
    else if (association == vtkDataObject::FIELD_ASSOCIATION_CELLS)
    {
      // Use vtkPVThreshold here.
      return this->ClipUsingThreshold(request, inputVector, outputVector);
    }
    else
    {
      vtkErrorMacro("Unhandled association: " << association);
    }
  } // End for vtkDataSet.
  else if (vtkHyperTreeGrid::SafeDownCast(inDataObj))
  {
    //  Using Scalar
    if (!this->ClipFunction)
    {
      return this->ClipUsingThreshold(request, inputVector, outputVector);
    }
    vtkPlane* plane = vtkPlane::SafeDownCast(this->ClipFunction);
    vtkPVBox* box = vtkPVBox::SafeDownCast(this->ClipFunction);
    vtkSphere* sphere = vtkSphere::SafeDownCast(this->ClipFunction);
    vtkQuadric* quadric = vtkQuadric::SafeDownCast(this->ClipFunction);
    vtkPVCylinder* cylinder = vtkPVCylinder::SafeDownCast(this->ClipFunction);
    vtkNew<vtkHyperTreeGridAxisClip> htgClip;
    if (plane)
    {
      double* normal = plane->GetNormal();
      if (!vtkMathUtilities::NearlyEqual(normal[0], 1.0) &&
        !vtkMathUtilities::NearlyEqual(normal[1], 1.0) &&
        !vtkMathUtilities::NearlyEqual(normal[2], 1.0))
      {
        htgClip->SetClipTypeToQuadric();
        htgClip->SetQuadricCoefficients(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, normal[0], normal[1],
          normal[2], plane->EvaluateFunction(0.0, 0.0, 0.0));
      }
      else
      {
        htgClip->SetClipTypeToPlane();
        htgClip->SetPlanePosition(-plane->EvaluateFunction(0.0, 0.0, 0.0));
        int planeNormalAxis = 0;
        if (normal[1] > normal[0])
        {
          planeNormalAxis = 1;
        }
        if (normal[2] > normal[0])
        {
          planeNormalAxis = 2;
        }
        htgClip->SetPlanePosition(-plane->EvaluateFunction(0.0, 0.0, 0.0));
        htgClip->SetPlaneNormalAxis(planeNormalAxis);
      }
    }
    else if (box)
    {
      htgClip->SetClipTypeToBox();
      double position[3], scale[3];
      box->GetPosition(position);
      box->GetScale(scale);
      htgClip->SetBounds(box->GetBounds());
      htgClip->SetBounds(position[0], position[0] + scale[0], position[1], position[1] + scale[1],
        position[2], position[2] + scale[2]);
    }
    else if (sphere)
    {
      htgClip->SetClipTypeToQuadric();
      double center[3], radius = sphere->GetRadius();
      sphere->GetCenter(center);
      htgClip->SetQuadricCoefficients(1.0, 1.0, 1.0, 0.0, 0.0, 0.0, -2.0 * center[0],
        -2.0 * center[1], -2.0 * center[2],
        -radius * radius + center[0] * center[0] + center[1] * center[1] + center[2] * center[2]);
    }
    else if (cylinder)
    {
      htgClip->SetClipTypeToQuadric();
      double* axis = cylinder->GetOrientedAxis();
      double radius = cylinder->GetRadius();
      double* center = cylinder->GetCenter();
      htgClip->SetQuadricCoefficients(axis[1] * axis[1] + axis[2] * axis[2],
        axis[0] * axis[0] + axis[2] * axis[2], axis[0] * axis[0] + axis[1] * axis[1],
        -2.0 * axis[0] * axis[1], -2.0 * axis[1] * axis[2], -2.0 * axis[0] * axis[2],
        -2.0 * center[0] * (axis[1] * axis[1] + axis[2] * axis[2]) +
          2 * center[1] * axis[0] * axis[1] + 2 * center[2] * axis[0] * axis[2],
        -2.0 * center[1] * (axis[0] * axis[0] + axis[2] * axis[2]) +
          2 * center[0] * axis[0] * axis[1] + 2 * center[2] * axis[1] * axis[2],
        -2.0 * center[2] * (axis[0] * axis[0] + axis[1] * axis[1]) +
          2 * center[1] * axis[2] * axis[1] + 2 * center[0] * axis[0] * axis[2],
        -radius * radius + center[0] * center[0] * (axis[1] * axis[1] + axis[2] * axis[2]) +
          center[1] * center[1] * (axis[0] * axis[0] + axis[2] * axis[2]) +
          center[2] * center[2] * (axis[0] * axis[0] + axis[1] * axis[1]) -
          2.0 *
            (axis[0] * axis[1] * center[0] * center[1] + axis[1] * axis[2] * center[1] * center[2] +
              axis[0] * axis[2] * center[0] * center[2]));
    }
    else if (quadric)
    {
      htgClip->SetClipTypeToQuadric();
      htgClip->SetQuadric(quadric);
    }
    else
    {
      vtkErrorMacro(<< "Clipping function not supported");
      return 0;
    }
    htgClip->SetInsideOut(this->GetInsideOut() != 0);
    htgClip->SetInputData(0, vtkHyperTreeGrid::SafeDownCast(inDataObj));
    htgClip->Update();
    outDataObj->ShallowCopy(htgClip->GetOutput(0));
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVClipDataSet::ClipUsingThreshold(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);

  auto threshold = vtkSmartPointer<vtkPVThreshold>::New();

  vtkCompositeDataPipeline* executive = vtkCompositeDataPipeline::New();
  threshold->SetExecutive(executive);
  executive->FastDelete();

  vtkDataObject* inputClone = inputDO->NewInstance();
  inputClone->ShallowCopy(inputDO);
  threshold->SetInputData(inputClone);
  inputClone->FastDelete();
  threshold->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));

  if (this->GetInsideOut())
  {
    threshold->SetThresholdFunction(vtkThreshold::THRESHOLD_LOWER);
    threshold->SetLowerThreshold(this->GetValue());
  }
  else
  {
    threshold->SetThresholdFunction(vtkThreshold::THRESHOLD_UPPER);
    threshold->SetUpperThreshold(this->GetValue());
  }

  threshold->Update();
  outputDO->ShallowCopy(threshold->GetOutputDataObject(0));
  return 1;
}

namespace
{
//----------------------------------------------------------------------------
struct ClipPlaneInfo
{
  vtkSmartPointer<vtkPlane> Plane;
  double bounds[6];
  double Volume;
};

//----------------------------------------------------------------------------
void ReorderPlanesBasedOnBounds(double bounds[6], std::vector<vtkSmartPointer<vtkPlane>>& boxPlanes)
{
  // create a copy of the box planes
  std::vector<vtkSmartPointer<vtkPlane>> boxPlanesQueue(boxPlanes.size());
  std::copy(boxPlanes.begin(), boxPlanes.end(), boxPlanesQueue.begin());
  boxPlanes.clear();

  while (!boxPlanesQueue.empty())
  {
    // First try to remove planes that don't need to be checked.
    // If a plane doesn't intersect with the box, then the box is either in the direction of plane
    // normal or the opposite one. If the box is in same direction, then the plane is not needed.
    for (auto it = boxPlanesQueue.begin(); it != boxPlanesQueue.end();)
    {
      auto plane = *it;
      bool remove = false;
      auto intersect = vtkBox::IntersectWithPlane(bounds, plane->GetOrigin(), plane->GetNormal());
      if (intersect == 0)
      {
        // cite https://math.stackexchange.com/questions/1330210/
        double minPoint[3] = { bounds[0], bounds[2], bounds[4] };
        double directionVector[3];
        vtkMath::Subtract(minPoint, plane->GetOrigin(), directionVector);
        double dotProd = vtkMath::Dot(plane->GetNormal(), directionVector);
        // We check for negative dot product because the plane's normal is pointing outwards
        // (InsideOut is on).
        if (dotProd < 0)
        {
          it = boxPlanesQueue.erase(it);
          remove = true;
        }
      }
      if (!remove)
      {
        ++it;
      }
    }

    // It is beneficial to reorder planes based on which one creates the smallest clip result with
    // regard to the bounding box.
    if (boxPlanesQueue.size() > 1)
    {
      // create a cube
      vtkNew<vtkUnstructuredGrid> cube;
      vtkNew<vtkPoints> points;
      points->SetDataTypeToDouble();
      points->InsertNextPoint(bounds[0], bounds[2], bounds[4]);
      points->InsertNextPoint(bounds[1], bounds[2], bounds[4]);
      points->InsertNextPoint(bounds[1], bounds[3], bounds[4]);
      points->InsertNextPoint(bounds[0], bounds[3], bounds[4]);
      points->InsertNextPoint(bounds[0], bounds[2], bounds[5]);
      points->InsertNextPoint(bounds[1], bounds[2], bounds[5]);
      points->InsertNextPoint(bounds[1], bounds[3], bounds[5]);
      points->InsertNextPoint(bounds[0], bounds[3], bounds[5]);
      cube->SetPoints(points);
      vtkNew<vtkIdList> ids;
      ids->InsertNextId(0);
      ids->InsertNextId(1);
      ids->InsertNextId(2);
      ids->InsertNextId(3);
      ids->InsertNextId(4);
      ids->InsertNextId(5);
      ids->InsertNextId(6);
      ids->InsertNextId(7);
      cube->InsertNextCell(VTK_HEXAHEDRON, ids);

      std::vector<ClipPlaneInfo> clipPlaneInfo(boxPlanesQueue.size());
      for (size_t i = 0; i < boxPlanesQueue.size(); ++i)
      {
        auto& plane = boxPlanesQueue[i];
        // clip the cube with the plane
        vtkNew<vtkTableBasedClipDataSet> clip;
        clip->SetInputData(cube);
        clip->InsideOutOn();
        clip->SetClipFunction(plane);
        // triangulate it because vtkMassProperties only works with triangles
        vtkNew<vtkDataSetTriangleFilter> tri;
        tri->SetInputConnection(clip->GetOutputPort());
        // extract surface
        vtkNew<vtkGeometryFilter> geom;
        geom->SetInputConnection(tri->GetOutputPort());
        // Compute Volume. Note: GetVolume calls Update()
        vtkNew<vtkMassProperties> mass;
        mass->SetInputConnection(geom->GetOutputPort());
        clipPlaneInfo[i].Volume = mass->GetVolume();
        clipPlaneInfo[i].Plane = plane;
        geom->GetOutput()->GetBounds(clipPlaneInfo[i].bounds);
      }
      // sort planes based on volume
      std::sort(clipPlaneInfo.begin(), clipPlaneInfo.end(),
        [](const ClipPlaneInfo& a, const ClipPlaneInfo& b) { return a.Volume < b.Volume; });
      // put the best plane in the output
      boxPlanes.push_back(clipPlaneInfo[0].Plane);
      // update the bounds for the next iteration
      std::copy(clipPlaneInfo[0].bounds, clipPlaneInfo[0].bounds + 6, bounds);
      // put the remaining planes in the queue
      boxPlanesQueue.resize(boxPlanesQueue.size() - 1);
      for (size_t i = 1; i < clipPlaneInfo.size(); ++i)
      {
        boxPlanesQueue[i - 1] = clipPlaneInfo[i].Plane;
      }
    }
    else if (boxPlanesQueue.size() == 1)
    {
      // just add the plane
      boxPlanes.push_back(boxPlanesQueue[0]);
      boxPlanesQueue.clear();
    }
  }
}
constexpr vtkIdType VTK_MINIMUM_CELLS_TO_REORDER_PLANES = 1000000;
}

//----------------------------------------------------------------------------
int vtkPVClipDataSet::ClipUsingSuperclass(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (vtkPVBox* pvBox = vtkPVBox::SafeDownCast(this->GetClipFunction()))
  {
    if (this->ExactBoxClip && this->GetInsideOut())
    {
      int retVal = 1;
      // create 6 implicit functions planes representing the faces of the boc
      vtkAbstractTransform* transform = pvBox->GetTransform();
      double bounds[6];
      pvBox->GetBounds(bounds);
      vtkSmartPointer<vtkDataSet> currentInputDO = vtkDataSet::GetData(inputVector[0], 0);
      // get the bounds of the dataset
      double dataBounds[6];
      currentInputDO->GetBounds(dataBounds);
      std::vector<vtkSmartPointer<vtkPlane>> boxPlanes;
      boxPlanes.reserve(6);
      for (int i = 0; i < 3; i++)
      {
        for (int j = 0; j < 2; j++)
        {
          auto plane = vtkSmartPointer<vtkPlane>::New();
          double origin[3] = { 0, 0, 0 };
          double normal[3] = { 0, 0, 0 };
          if (j == 0)
          {
            normal[i] = -1;
            origin[0] = bounds[0];
            origin[1] = bounds[2];
            origin[2] = bounds[4];
          }
          else
          {
            normal[i] = 1;
            origin[0] = bounds[1];
            origin[1] = bounds[3];
            origin[2] = bounds[5];
          }
          // it's more efficient to transform the origin and normal once rather than
          // transform all the points of the input.
          if (transform)
          {
            auto inverse = transform->GetInverse();
            inverse->TransformNormalAtPoint(origin, normal, normal);
            inverse->TransformPoint(origin, origin);
          }
          plane->SetOrigin(origin);
          plane->SetNormal(normal);

          boxPlanes.push_back(plane);
        }
      }
      // This optimization has a small but constant overhead that depends on the number of the
      // preserved planes. It should be used only if the dataset is big enough.
      if (currentInputDO->GetNumberOfCells() >= VTK_MINIMUM_CELLS_TO_REORDER_PLANES)
      {
        ::ReorderPlanesBasedOnBounds(dataBounds, boxPlanes);
      }
      for (const auto& plane : boxPlanes)
      {
        this->ClipFunction = plane; // temporarily set it without changing MTime of the filter

        // Creating new input information.
        auto newInInfoVec = vtkSmartPointer<vtkInformationVector>::New();
        vtkSmartPointer<vtkInformation> newInInfo = vtkSmartPointer<vtkInformation>::New();
        newInInfo->Set(vtkDataObject::DATA_OBJECT(), currentInputDO);
        newInInfoVec->SetInformationObject(0, newInInfo);

        // Creating new output information.
        auto usGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
        auto newOutInfoVec = vtkSmartPointer<vtkInformationVector>::New();
        auto newOutInfo = vtkSmartPointer<vtkInformation>::New();
        newOutInfo->Set(vtkDataObject::DATA_OBJECT(), usGrid);
        newOutInfoVec->SetInformationObject(0, newOutInfo);

        vtkInformationVector* newInInfoVecPtr = newInInfoVec.GetPointer();
        retVal = retVal && this->ClipUsingSuperclass(request, &newInInfoVecPtr, newOutInfoVec);
        currentInputDO = usGrid;
      }
      vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);
      outputDO->ShallowCopy(currentInputDO);
      this->ClipFunction = pvBox; // set back to original clip function
      return retVal;
    }
  }

  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);

  vtkNew<Superclass> instance;
  instance->SetInsideOut(this->GetInsideOut());
  instance->SetValue(this->GetValue());
  instance->SetUseValueAsOffset(this->GetUseValueAsOffset());
  instance->SetClipFunction(this->GetClipFunction());
  instance->SetGenerateClipScalars(this->GetGenerateClipScalars());
  instance->SetMergeTolerance(this->GetMergeTolerance());
  instance->SetGenerateClippedOutput(this->GetGenerateClippedOutput());
  instance->SetOutputPointsPrecision(this->GetOutputPointsPrecision());
  instance->SetBatchSize(this->GetBatchSize());

  // Dataset with "normals" will use non merging point locator instead of vtkMergePoints to avoid
  // potential artefacts
  vtkDataSet* inputDS = vtkDataSet::SafeDownCast(inputDO);
  vtkNew<vtkNonMergingPointLocator> locator;
  if (inputDS->GetPointData()->GetNormals() || inputDS->GetCellData()->GetNormals())
  {
    instance->SetLocator(locator);
  }

  instance->SetInputDataObject(inputDO);
  instance->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
  if (instance->GetExecutive()->Update())
  {
    outputDO->ShallowCopy(instance->GetOutput());
    if (instance->GetClippedOutput())
    {
      this->GetClippedOutput()->ShallowCopy(instance->GetClippedOutput());
    }
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
int vtkPVClipDataSet::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  auto inputDO = vtkDataObject::GetData(inInfo);
  if (!inInfo)
  {
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (vtkHyperTreeGrid::SafeDownCast(inputDO))
  {
    vtkHyperTreeGrid* output = vtkHyperTreeGrid::GetData(outInfo);
    if (!output)
    {
      output = vtkHyperTreeGrid::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      output->FastDelete();
    }
    return 1;
  }
  else
  {
    vtkDataSet* output = vtkDataSet::GetData(outInfo);
    if (!output)
    {
      output = vtkUnstructuredGrid::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      output->FastDelete();
    }
    return 1;
  }
}

//----------------------------------------------------------------------------
int vtkPVClipDataSet::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // create the output
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVClipDataSet::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVClipDataSet::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}
