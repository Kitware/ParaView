/*=========================================================================

  Program:   ParaView
  Module:    vtkPrismGeometryConverter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPrismGeometryConverter.h"

#include "vtkArrayCalculator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

// clang-format off
#include <vtk_fmt.h> // needed for `fmt`
#include VTK_FMT(fmt/core.h)
// clang-format on

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkPrismGeometryConverter);

//------------------------------------------------------------------------------
vtkPrismGeometryConverter::vtkPrismGeometryConverter()
{
  this->Calculator->SetAttributeTypeToPointData();
  this->Calculator->CoordinateResultsOn();
  this->Calculator->AddCoordinateScalarVariable("coordsX", 0);
  this->Calculator->AddCoordinateScalarVariable("coordsY", 1);
  this->Calculator->AddCoordinateScalarVariable("coordsZ", 2);
  this->Calculator->ReplaceInvalidValuesOn();
  this->Calculator->SetReplacementValue(0.0);
}

//------------------------------------------------------------------------------
vtkPrismGeometryConverter::~vtkPrismGeometryConverter() = default;

//------------------------------------------------------------------------------
void vtkPrismGeometryConverter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PrismBounds: " << this->PrismBounds[0] << ", " << this->PrismBounds[1] << ", "
     << this->PrismBounds[2] << ", " << this->PrismBounds[3] << ", " << this->PrismBounds[4] << ", "
     << this->PrismBounds[5] << endl;
  os << indent << "Log ScaleX: " << (this->LogScaleX ? "On" : "Off") << endl;
  os << indent << "Log ScaleY: " << (this->LogScaleY ? "On" : "Off") << endl;
  os << indent << "Log ScaleZ: " << (this->LogScaleZ ? "On" : "Off") << endl;
  os << indent << "Aspect Ratio: " << this->AspectRatio[0] << ", " << this->AspectRatio[1] << ", "
     << this->AspectRatio[2] << endl;
}

//------------------------------------------------------------------------------
int vtkPrismGeometryConverter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // transform points to be in the cubic space and apply log scale if needed
  double* aspectRatio = this->AspectRatio;
  double prismBounds[6];
  std::copy_n(this->PrismBounds, 6, prismBounds);
  if (this->LogScaleX)
  {
    prismBounds[0] = prismBounds[0] > 0 ? std::log(prismBounds[0]) : 0;
    prismBounds[1] = prismBounds[1] > 0 ? std::log(prismBounds[1]) : 0;
  }
  if (this->LogScaleY)
  {
    prismBounds[2] = prismBounds[2] > 0 ? std::log(prismBounds[2]) : 0;
    prismBounds[3] = prismBounds[3] > 0 ? std::log(prismBounds[3]) : 0;
  }
  if (this->LogScaleZ)
  {
    prismBounds[4] = prismBounds[4] > 0 ? std::log(prismBounds[4]) : 0;
    prismBounds[5] = prismBounds[5] > 0 ? std::log(prismBounds[5]) : 0;
  }

  std::string coordsX = "coordsX";
  if (this->LogScaleX)
  {
    coordsX = "(" + coordsX + " > 0 ? log(" + coordsX + ") : 0)";
  }
  const auto functionXPart = "(" + coordsX + "-" + fmt::format("{}", prismBounds[0]) + ")/(" +
    fmt::format("{}", prismBounds[1] - prismBounds[0]) + ")*" + fmt::format("{}", aspectRatio[0]);
  std::string coordsY = "coordsY";
  if (this->LogScaleY)
  {
    coordsY = "(" + coordsY + " > 0 ? log(" + coordsY + ") : 0)";
  }
  const auto functionYPart = "(" + coordsY + "-" + fmt::format("{}", prismBounds[2]) + ")/(" +
    fmt::format("{}", prismBounds[3] - prismBounds[2]) + ")*" + fmt::format("{}", aspectRatio[1]);
  std::string coordsZ = "coordsZ";
  if (this->LogScaleZ)
  {
    coordsZ = "(" + coordsZ + " > 0 ? log(" + coordsZ + ") : 0)";
  }
  const auto functionZPart = "(" + coordsZ + "-" + fmt::format("{}", prismBounds[4]) + ")/(" +
    fmt::format("{}", prismBounds[5] - prismBounds[4]) + ")*" + fmt::format("{}", aspectRatio[2]);
  const auto function =
    "iHat*(" + functionXPart + ")+jHat*(" + functionYPart + ")+kHat*(" + functionZPart + ")";
  this->Calculator->SetFunction(function.c_str());
  this->Calculator->ProcessRequest(request, inputVector, outputVector);

  return 1;
}
