/*=========================================================================

  Program:   ParaView
  Module:    vtkVolumeRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVolumeRepresentation.h"

#include "vtkColorTransferFunction.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkOutlineSource.h"
#include "vtkPVLODVolume.h"
#include "vtkSMPTools.h"
#include "vtkVolumeProperty.h"

//----------------------------------------------------------------------------
vtkVolumeRepresentation::vtkVolumeRepresentation()
  : Actor(vtkSmartPointer<vtkPVLODVolume>::New())
{
  this->Actor->SetProperty(this->Property);

  vtkMath::UninitializeBounds(this->DataBounds);
}

//----------------------------------------------------------------------------
vtkVolumeRepresentation::~vtkVolumeRepresentation()
{
}

//***************************************************************************
// Forwarded to Actor.

//----------------------------------------------------------------------------
void vtkVolumeRepresentation::SetVisibility(bool val)
{
  this->Superclass::SetVisibility(val);
  this->Actor->SetVisibility(val ? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkVolumeRepresentation::SetOrientation(double x, double y, double z)
{
  this->Actor->SetOrientation(x, y, z);
}

//----------------------------------------------------------------------------
void vtkVolumeRepresentation::SetOrigin(double x, double y, double z)
{
  this->Actor->SetOrigin(x, y, z);
}

//----------------------------------------------------------------------------
void vtkVolumeRepresentation::SetPickable(int val)
{
  this->Actor->SetPickable(val);
}
//----------------------------------------------------------------------------
void vtkVolumeRepresentation::SetPosition(double x, double y, double z)
{
  this->Actor->SetPosition(x, y, z);
}
//----------------------------------------------------------------------------
void vtkVolumeRepresentation::SetScale(double x, double y, double z)
{
  this->Actor->SetScale(x, y, z);
}

//***************************************************************************
// Forwarded to vtkVolumeProperty.
//----------------------------------------------------------------------------
void vtkVolumeRepresentation::SetInterpolationType(int val)
{
  this->Property->SetInterpolationType(val);
}

//----------------------------------------------------------------------------
void vtkVolumeRepresentation::SetColor(vtkColorTransferFunction* lut)
{
  this->Property->SetColor(lut);
}

//----------------------------------------------------------------------------
void vtkVolumeRepresentation::SetScalarOpacity(vtkPiecewiseFunction* pwf)
{
  this->Property->SetScalarOpacity(pwf);
}

//----------------------------------------------------------------------------
void vtkVolumeRepresentation::SetScalarOpacityUnitDistance(double val)
{
  this->Property->SetScalarOpacityUnitDistance(val);
}

//----------------------------------------------------------------------------
void vtkVolumeRepresentation::SetMapScalars(bool val)
{
  this->MapScalars = val;
  // the value is passed on to the vtkVolumeProperty in UpdateMapperParameters
  // since SetMapScalars and SetMultiComponentsMapping both control the same
  // vtkVolumeProperty ivar i.e. IndependentComponents.
}

//----------------------------------------------------------------------------
void vtkVolumeRepresentation::SetMultiComponentsMapping(bool val)
{
  this->MultiComponentsMapping = val;
  // the value is passed on to the vtkVolumeProperty in UpdateMapperParameters
  // since SetMapScalars and SetMultiComponentsMapping both control the same
  // vtkVolumeProperty ivar i.e. IndependentComponents.
}

//----------------------------------------------------------------------------
void vtkVolumeRepresentation::SetUseSeparateOpacityArray(bool value)
{
  if (this->UseSeparateOpacityArray != value)
  {
    this->UseSeparateOpacityArray = value;
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
void vtkVolumeRepresentation::SelectOpacityArray(
  int, int, int, int fieldAssociation, const char* name)
{
  std::string newName;
  if (name)
  {
    newName = std::string(name);
  }

  if (this->OpacityArrayName != newName)
  {
    this->OpacityArrayName = newName;
    this->MarkModified();
  }

  if (this->OpacityArrayFieldAssociation != fieldAssociation)
  {
    this->OpacityArrayFieldAssociation = fieldAssociation;
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
void vtkVolumeRepresentation::SelectOpacityArrayComponent(int component)
{
  if (this->OpacityArrayComponent != component)
  {
    this->OpacityArrayComponent = component;
    this->MarkModified();
  }
}

namespace
{

/**
 * Copies a component from one array to another. If the source component is >= the number of
 * components in the source array, the source array is treated as a vector and its magnitude
 * is computed and set in the destination component.
 */
void CopyComponent(vtkDataArray* dst, int dstComponent, vtkDataArray* src, int srcComponent)
{
  if (srcComponent < src->GetNumberOfComponents())
  {
    dst->CopyComponent(dstComponent, src, srcComponent);
  }
  else
  {
    vtkSMPTools::For(0, src->GetNumberOfTuples(), [&](vtkIdType start, vtkIdType end) {
      // Compute vector value
      auto inTuples = vtk::DataArrayTupleRange(src, start, end);
      auto outValues = vtk::DataArrayTupleRange(dst, start, end);
      auto outIter = outValues.begin();
      for (auto tuple : inTuples)
      {
        double magnitude = 0.0;
        for (auto component : tuple)
        {
          magnitude += static_cast<double>(component) * static_cast<double>(component);
        }
        (*outIter)[dstComponent] = std::sqrt(magnitude);
        ++outIter;
      }
    });
  }
}

} // end namespace

//----------------------------------------------------------------------------
bool vtkVolumeRepresentation::AppendOpacityComponent(vtkDataSet* dataset)
{
  // Get opacity array
  if (!dataset)
  {
    vtkErrorMacro("No input");
    return false;
  }

  vtkDataArray* opacityArray = dataset->GetAttributes(this->OpacityArrayFieldAssociation)
                                 ->GetArray(this->OpacityArrayName.c_str());
  if (!opacityArray)
  {
    vtkErrorMacro("No opacity array named '" << this->OpacityArrayName << "' in input.");
    return false;
  }

  const char* colorArrayName = nullptr;
  int fieldAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;
  vtkInformation* info = this->GetInputArrayInformation(0);
  if (info && info->Has(vtkDataObject::FIELD_ASSOCIATION()) &&
    info->Has(vtkDataObject::FIELD_NAME()))
  {
    colorArrayName = info->Get(vtkDataObject::FIELD_NAME());
    fieldAssociation = info->Get(vtkDataObject::FIELD_ASSOCIATION());

    if (colorArrayName)
    {
      vtkDataArray* colorArray = this->GetInputArrayToProcess(0, dataset);
      // Create a copy of the array
      std::string combinedName(colorArrayName);
      combinedName += "_and_opacity";
      vtkSmartPointer<vtkDataArray> combinedArray;
      combinedArray.TakeReference(colorArray->NewInstance());
      combinedArray->SetName(combinedName.c_str());
      int numComponents = colorArray->GetNumberOfComponents();
      if (numComponents != 1 && numComponents != 3)
      {
        vtkErrorMacro("Cannot use a separate opacity array name when color array has "
          << numComponents << " components.");
        return false;
      }
      // We will always create a two-component array, the first for color and the second for
      // opacity.
      combinedArray->SetNumberOfComponents(2);
      const int colorComponent = 0;
      const int opacityComponent = 1;
      combinedArray->SetNumberOfTuples(colorArray->GetNumberOfTuples());

      // Copy initial color component(s)
      vtkColorTransferFunction* ctf = this->Property->GetRGBTransferFunction(0);
      int sourceComponent = ctf->GetVectorMode() == vtkScalarsToColors::COMPONENT
        ? ctf->GetVectorComponent()
        : colorArray->GetNumberOfComponents();

      // TODO - force update when component changes
      CopyComponent(combinedArray, colorComponent, colorArray, sourceComponent);

      // Copy the opacity component
      CopyComponent(combinedArray, opacityComponent, opacityArray, this->OpacityArrayComponent);

      dataset->GetAttributes(fieldAssociation)->AddArray(combinedArray);
    }
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkVolumeRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  auto printObject = [&](vtkObject* object, const std::string& name) {
    if (object)
    {
      os << indent << name << ":\n";
      object->PrintSelf(os, indent.GetNextIndent());
    }
    else
    {
      os << indent << name << ": (none)\n";
    }
  };

  printObject(this->Property, "Property");
  printObject(this->Actor, "Actor");
  printObject(this->OutlineSource, "OutlineSource");

  os << indent << "MapScalars: " << this->MapScalars << endl;
  os << indent << "MultiComponentsMapping: " << this->MultiComponentsMapping << endl;
  os << indent << "UseSeparateOpacityArray: " << this->UseSeparateOpacityArray << endl;
  os << indent << "OpacityArrayName: " << this->OpacityArrayName << endl;
  os << indent << "OpacityArrayFieldAssociation: " << this->OpacityArrayFieldAssociation << endl;
  os << indent << "OpacityArrayComponent: " << this->OpacityArrayComponent << endl;
}
