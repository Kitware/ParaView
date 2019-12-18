/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPLYWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPLYWriter.h"

#include "vtkDataSetAttributes.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPLYWriter.h"
#include "vtkPolyData.h"
#include "vtkScalarsToColors.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>

vtkStandardNewMacro(vtkPVPLYWriter);
//----------------------------------------------------------------------------
vtkPVPLYWriter::vtkPVPLYWriter()
  : EnableColoring(false)
  , EnableAlpha(false)
{
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, static_cast<const char*>(NULL));
}

//----------------------------------------------------------------------------
vtkPVPLYWriter::~vtkPVPLYWriter()
{
}

//----------------------------------------------------------------------------
int vtkPVPLYWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVPLYWriter::SetDataByteOrder(int dbo)
{
  this->Writer->SetDataByteOrder(dbo);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVPLYWriter::SetFileType(int ftype)
{
  this->Writer->SetFileType(ftype);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVPLYWriter::SetFileName(const char* fname)
{
  this->Writer->SetFileName(fname);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVPLYWriter::SetLookupTable(vtkScalarsToColors* lut)
{
  if (this->LookupTable != lut)
  {
    this->LookupTable = lut;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVPLYWriter::WriteData()
{
  int fieldAssociation = 0;
  vtkPolyData* input = vtkPolyData::SafeDownCast(this->GetInputDataObject(0, 0));
  if (vtkAbstractArray* scalars = (this->EnableColoring && this->LookupTable != NULL)
      ? this->GetInputAbstractArrayToProcess(0, input, fieldAssociation)
      : NULL)
  {
    this->Writer->SetColorModeToDefault();
    this->Writer->SetArrayName("vtkPVPLYWriterColors");
    vtkDiscretizableColorTransferFunction* dctf =
      vtkDiscretizableColorTransferFunction::SafeDownCast(this->LookupTable);
    bool enableOpacityMapping = dctf->GetEnableOpacityMapping();
    dctf->SetEnableOpacityMapping(this->EnableAlpha);
    vtkUnsignedCharArray* rgba =
      this->LookupTable->MapScalars(scalars, VTK_COLOR_MODE_MAP_SCALARS, -1);
    rgba->SetName("vtkPVPLYWriterColors");

    vtkSmartPointer<vtkPolyData> clone;
    clone.TakeReference(input->NewInstance());
    clone->ShallowCopy(input);
    clone->GetAttributes(fieldAssociation)->AddArray(rgba);
    rgba->FastDelete();

    this->Writer->SetEnableAlpha(this->EnableAlpha);
    this->Writer->SetInputDataObject(0, clone.GetPointer());
    dctf->SetEnableOpacityMapping(enableOpacityMapping);
  }
  else
  {
    this->Writer->SetInputDataObject(0, input);
    this->Writer->SetColorModeToOff();
    this->Writer->EnableAlphaOff();
  }
  this->Writer->Write();
  this->Writer->SetInputDataObject(0, NULL);
}

//----------------------------------------------------------------------------
void vtkPVPLYWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
