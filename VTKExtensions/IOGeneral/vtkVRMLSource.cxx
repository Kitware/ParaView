/*=========================================================================

  Program:   ParaView
  Module:    vtkVRMLSource.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVRMLSource.h"

#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkAppendPolyData.h"
#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVRMLImporter.h"

vtkStandardNewMacro(vtkVRMLSource);

//------------------------------------------------------------------------------
vtkVRMLSource::vtkVRMLSource()
{
  this->FileName = NULL;
  this->Importer = NULL;
  this->Color = 1;
  this->Append = 1;

  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
vtkVRMLSource::~vtkVRMLSource()
{
  this->SetFileName(NULL);
  if (this->Importer)
  {
    this->Importer->Delete();
    this->Importer = NULL;
  }
}

//-----------------------------------------------------------------------------
int vtkVRMLSource::CanReadFile(const char* filename)
{
  FILE* fd = fopen(filename, "r");
  if (!fd)
    return 0;

  char header[128];
  if (fgets(header, 128, fd) == NULL)
  {
    return 0;
  }

  // Technically, the header should start with "#VRML V2.0 utf8", but who's
  // to say that new versions will not be forward compatible.  Let's not be
  // perscriptive yet.  If some future version of VRML is incompatible, we
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

  if (this->Importer == NULL)
  {
    this->InitializeImporter();
  }
  this->CopyImporterToOutputs(output);

  return 1;
}

//------------------------------------------------------------------------------
void vtkVRMLSource::InitializeImporter()
{
  if (this->Importer)
  {
    this->Importer->Delete();
    this->Importer = NULL;
  }
  this->Importer = vtkVRMLImporter::New();
  this->Importer->SetFileName(this->FileName);
  this->Importer->Read();
}

//------------------------------------------------------------------------------
void vtkVRMLSource::CopyImporterToOutputs(vtkMultiBlockDataSet* mbOutput)
{
  vtkRenderer* ren;
  vtkActorCollection* actors;
  vtkActor* actor;
  vtkPolyDataMapper* mapper;
  vtkPolyData* input;
  vtkPolyData* output;
  int idx;
  int numArrays, arrayIdx, numPoints, numCells;
  vtkDataArray* array;
  int arrayCount = 0;
  char name[256];
  int ptIdx;
  vtkAppendPolyData* append = NULL;

  if (this->Importer == NULL)
  {
    return;
  }

  if (this->Append)
  {
    append = vtkAppendPolyData::New();
  }

  ren = this->Importer->GetRenderer();
  actors = ren->GetActors();
  actors->InitTraversal();
  idx = 0;
  while ((actor = actors->GetNextActor()))
  {
    mapper = vtkPolyDataMapper::SafeDownCast(actor->GetMapper());
    if (mapper)
    {
      mapper->Update();
      input = mapper->GetInput();
      output = vtkPolyData::New();

      if (!append)
      {
        mbOutput->SetBlock(idx, output);
      }

      vtkTransformPolyDataFilter* tf = vtkTransformPolyDataFilter::New();
      vtkTransform* trans = vtkTransform::New();
      tf->SetInputData(input);
      tf->SetTransform(trans);
      tf->Update();
      trans->SetMatrix(actor->GetMatrix());
      input = tf->GetOutput();

      output->CopyStructure(input);
      // Only copy well formed arrays.
      numPoints = input->GetNumberOfPoints();
      numArrays = input->GetPointData()->GetNumberOfArrays();
      for (arrayIdx = 0; arrayIdx < numArrays; ++arrayIdx)
      {
        array = input->GetPointData()->GetArray(arrayIdx);
        if (array->GetNumberOfTuples() == numPoints)
        {
          if (array->GetName() == NULL)
          {
            sprintf(name, "VRMLArray%d", ++arrayCount);
            array->SetName(name);
          }
          output->GetPointData()->AddArray(array);
        }
      }
      // Only copy well formed arrays.
      numCells = input->GetNumberOfCells();
      numArrays = input->GetCellData()->GetNumberOfArrays();
      for (arrayIdx = 0; arrayIdx < numArrays; ++arrayIdx)
      {
        array = input->GetCellData()->GetArray(arrayIdx);
        if (array->GetNumberOfTuples() == numCells)
        {
          if (array->GetName() == NULL)
          {
            sprintf(name, "VRMLArray%d", ++arrayCount);
            array->SetName(name);
          }
          output->GetCellData()->AddArray(array);
        }
      }
      if (this->Color)
      {
        vtkUnsignedCharArray* colorArray = vtkUnsignedCharArray::New();
        unsigned char r, g, b;
        double* actorColor;

        actorColor = actor->GetProperty()->GetColor();
        r = static_cast<unsigned char>(actorColor[0] * 255.0);
        g = static_cast<unsigned char>(actorColor[1] * 255.0);
        b = static_cast<unsigned char>(actorColor[2] * 255.0);
        colorArray->SetName("VRMLColor");
        colorArray->SetNumberOfComponents(3);
        for (ptIdx = 0; ptIdx < numPoints; ++ptIdx)
        {
          colorArray->InsertNextValue(r);
          colorArray->InsertNextValue(g);
          colorArray->InsertNextValue(b);
        }
        output->GetPointData()->SetScalars(colorArray);
        colorArray->Delete();
        colorArray = NULL;
      }
      if (append)
      {
        append->AddInputData(output);
      }
      output->Delete();
      output = NULL;

      ++idx;
      tf->Delete();
      tf = NULL;
      trans->Delete();
      trans = NULL;
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
