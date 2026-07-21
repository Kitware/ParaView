// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkVRMLSource.h"

#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkAppendPolyData.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkStringFormatter.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVRMLImporter.h"

#include "vtksys/SystemTools.hxx"

#include <iostream>

vtkStandardNewMacro(vtkVRMLSource);

//------------------------------------------------------------------------------
vtkVRMLSource::vtkVRMLSource()
{
  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
vtkVRMLSource::~vtkVRMLSource()
{
  this->SetFileName(nullptr);
}

//-----------------------------------------------------------------------------
int vtkVRMLSource::CanReadFile(const char* filename)
{
  FILE* fd = vtksys::SystemTools::Fopen(filename, "r");
  if (!fd)
  {
    return 0;
  }

  char header[128];
  if (fgets(header, 128, fd) == nullptr)
  {
    fclose(fd);
    return 0;
  }

  // Technically, the header should start with "#VRML V2.0 utf8", but who's
  // to say that new versions will not be forward compatible.  Let's not be
  // prescriptive yet.  If some future version of VRML is incompatible, we
  // can make this test more strict.
  int valid = (strncmp(header, "#VRML ", 6) == 0);

  fclose(fd);
  return valid;
}

//------------------------------------------------------------------------------
int vtkVRMLSource::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);

  vtkDataObject* doOutput = info->Get(vtkDataObject::DATA_OBJECT());
  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::SafeDownCast(doOutput);
  if (!output)
  {
    return 0;
  }

  this->InitializeImporter();
  this->CopyImporterToOutputs(output);

  return 1;
}

//------------------------------------------------------------------------------
void vtkVRMLSource::InitializeImporter()
{
  this->Importer->SetFileName(this->FileName);
  this->Importer->Update();
}

//------------------------------------------------------------------------------
void vtkVRMLSource::CopyImporterToOutputs(vtkMultiBlockDataSet* mbOutput)
{
  vtkAppendPolyData* append = nullptr;

  if (this->Append)
  {
    append = vtkAppendPolyData::New();
  }

  vtkRenderer* ren = this->Importer->GetRenderer();
  vtkActorCollection* actors = ren->GetActors();
  actors->InitTraversal();
  int idx = 0;
  int arrayCount = 0;
  vtkActor* actor = nullptr;
  while ((actor = actors->GetNextActor()))
  {
    vtkPolyDataMapper* mapper = vtkPolyDataMapper::SafeDownCast(actor->GetMapper());
    if (mapper)
    {
      mapper->Update();
      vtkPolyData* input = mapper->GetInput();
      vtkPolyData* output = vtkPolyData::New();

      if (!append)
      {
        mbOutput->SetBlock(idx, output);
      }

      vtkNew<vtkTransformFilter> tf;
      vtkNew<vtkTransform> trans;
      tf->SetInputData(input);
      tf->SetTransform(trans);
      tf->Update();
      trans->SetMatrix(actor->GetMatrix());
      input = tf->GetPolyDataOutput();

      output->CopyStructure(input);
      // Only copy well formed arrays.
      vtkIdType numPoints = input->GetNumberOfPoints();
      int numArrays = input->GetPointData()->GetNumberOfArrays();
      for (int arrayIdx = 0; arrayIdx < numArrays; ++arrayIdx)
      {
        vtkDataArray* array = input->GetPointData()->GetArray(arrayIdx);
        if (array->GetNumberOfTuples() == numPoints)
        {
          if (array->GetName() == nullptr)
          {
            char name[256];
            auto result = vtk::format_to_n(name, sizeof(name), "VRMLArray{:d}", ++arrayCount);
            *result.out = '\0';
            array->SetName(name);
          }
          output->GetPointData()->AddArray(array);
        }
      }
      // Only copy well formed arrays.
      vtkIdType numCells = input->GetNumberOfCells();
      numArrays = input->GetCellData()->GetNumberOfArrays();
      for (int arrayIdx = 0; arrayIdx < numArrays; ++arrayIdx)
      {
        vtkDataArray* array = input->GetCellData()->GetArray(arrayIdx);
        if (array->GetNumberOfTuples() == numCells)
        {
          if (array->GetName() == nullptr)
          {
            char name[256];
            auto result = vtk::format_to_n(name, sizeof(name), "VRMLArray{:d}", ++arrayCount);
            *result.out = '\0';
            array->SetName(name);
          }
          output->GetCellData()->AddArray(array);
        }
      }
      if (this->Color)
      {
        // Create an array that stores per-vertex colors for the VRML file.
        vtkNew<vtkUnsignedCharArray> colorArray;
        colorArray->SetName("VRMLColor");
        colorArray->SetNumberOfComponents(4);
        colorArray->SetNumberOfTuples(numPoints);

        // The lookup table from the vtkVRMLImport has the scalar range for mapping,
        // not the mapper, so use it for mapping.
        mapper->UseLookupTableScalarRangeOn();

        // MapScalars() manages the pointer it returns (if any), so we don't need to delete it.
        vtkUnsignedCharArray* rgbaArray = mapper->MapScalars(1.0);
        if (rgbaArray)
        {
          // vtkVRMLImporter may add an extra tuple to the active scalar array, so we copy up
          // to the number of points in the polydata rather than simply DeepCopy the entire array.
          colorArray->InsertTuples(0, numPoints, 0, rgbaArray);
        }
        else
        {
          // Mapping didn't succeed so copy the solid color of the actor to the VRMLColor array.
          double* actorColor = actor->GetProperty()->GetColor();
          unsigned char rgba[4];
          rgba[0] = static_cast<unsigned char>(actorColor[0] * 255.0);
          rgba[1] = static_cast<unsigned char>(actorColor[1] * 255.0);
          rgba[2] = static_cast<unsigned char>(actorColor[2] * 255.0);
          rgba[3] = 255;

          colorArray->SetName("VRMLColor");
          colorArray->SetNumberOfComponents(4);
          for (vtkIdType ptIdx = 0; ptIdx < numPoints; ++ptIdx)
          {
            colorArray->SetTypedTuple(ptIdx, rgba);
          }
        }
        output->GetPointData()->SetScalars(colorArray);
      }

      if (append)
      {
        append->AddInputData(output);
      }
      output->Delete();
      output = nullptr;

      ++idx;
    }
  }

  if (append)
  {
    append->Update();
    vtkPolyData* newOutput = vtkPolyData::New();
    newOutput->ShallowCopy(append->GetOutput());
    mbOutput->SetBlock(0, newOutput);
    newOutput->Delete();
    append->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkVRMLSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->FileName)
  {
    os << indent << "FileName: " << this->FileName << endl;
  }
  os << indent << "Color: " << this->Color << endl;
  os << indent << "Append: " << this->Append << endl;
}
