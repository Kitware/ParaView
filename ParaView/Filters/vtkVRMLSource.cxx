/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVRMLSource.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVRMLSource.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkActorCollection.h"
#include "vtkActor.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkVRMLImporter.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkAppendPolyData.h"
#include "vtkTransform.h"
#include "vtkUnsignedCharArray.h"


vtkCxxRevisionMacro(vtkVRMLSource, "1.6");
vtkStandardNewMacro(vtkVRMLSource);

//------------------------------------------------------------------------------
vtkVRMLSource::vtkVRMLSource()
{
  this->FileName = NULL;
  this->Importer = NULL;
  this->Color = 1;
  this->Append = 1;
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

//----------------------------------------------------------------------------
vtkPolyData* vtkVRMLSource::GetOutput(int idx)
{
  if (this->Importer == NULL)
    {
    this->InitializeImporter();
    }

  return (vtkPolyData *) this->vtkSource::GetOutput(idx); 
}

//----------------------------------------------------------------------------
int vtkVRMLSource::GetNumberOfOutputs()
{
  if (this->Append)
    {
    return 1;
    }

  if (this->Importer == NULL)
    {
    this->InitializeImporter();
    }

  return this->NumberOfOutputs;
}


//------------------------------------------------------------------------------
void vtkVRMLSource::Execute()
{
  if (this->Importer == NULL)
    {
    this->InitializeImporter();
    }
  this->CopyImporterToOutputs();
}

//------------------------------------------------------------------------------
void vtkVRMLSource::InitializeImporter()
{
  vtkRenderer* ren;
  vtkActorCollection* actors;
  vtkActor* actor;
  vtkPolyDataMapper* mapper;
  int idx;  

  if (this->Importer)
    {
    this->Importer->Delete();
    this->Importer = NULL;
    }
  this->Importer = vtkVRMLImporter::New();
  this->Importer->SetFileName(this->FileName);
  this->Importer->Read();
  ren = this->Importer->GetRenderer();
  actors = ren->GetActors();
  actors->InitTraversal();
  idx = 0;
  while ( (actor = actors->GetNextActor()) )
    {
    mapper = vtkPolyDataMapper::SafeDownCast(actor->GetMapper());
    if (mapper)
      {
      //mapper->GetInput()->Update();
      vtkPolyData *newOutput = vtkPolyData::New();
      newOutput->CopyInformation(mapper->GetInput());
      this->SetNthOutput(idx, newOutput);
      ++idx;
      newOutput->Delete();
      newOutput = NULL;
      }
    }
}

//------------------------------------------------------------------------------
void vtkVRMLSource::CopyImporterToOutputs()
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
  while ( (actor = actors->GetNextActor()) )
    {
    mapper = vtkPolyDataMapper::SafeDownCast(actor->GetMapper());
    if (mapper)
      {
      input = mapper->GetInput();
      input->Update();
      if (append)
        {
        output = vtkPolyData::New();
        }
      else
        {
        output = this->GetOutput(idx);
        }
      vtkTransformPolyDataFilter *tf = vtkTransformPolyDataFilter::New();
      vtkTransform *trans = vtkTransform::New();
      tf->SetInput(input);
      tf->SetTransform(trans);
      trans->SetMatrix(actor->GetMatrix());
      input = tf->GetOutput();
      input->Update();
      output->CopyStructure(input);
      // Only copy well formed arrays.
      numPoints = input->GetNumberOfPoints();
      numArrays = input->GetPointData()->GetNumberOfArrays();
      for ( arrayIdx = 0; arrayIdx < numArrays; ++arrayIdx)
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
      for ( arrayIdx = 0; arrayIdx < numArrays; ++arrayIdx)
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
        vtkUnsignedCharArray *colorArray = vtkUnsignedCharArray::New();
        unsigned char r, g, b;
        float* actorColor;
     
        actorColor = actor->GetProperty()->GetColor();
        r = static_cast<unsigned char>(actorColor[0]*255.0);
        g = static_cast<unsigned char>(actorColor[1]*255.0);
        b = static_cast<unsigned char>(actorColor[2]*255.0);
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
        append->AddInput(output);
        output->Delete();
        output = NULL;
        }

      ++idx;
      tf->Delete();
      tf = NULL;
      trans->Delete();
      trans = NULL;
      }
    }
  if (append)
    {
    output = this->GetOutput();
    append->Update();
    output->ShallowCopy(append->GetOutput());
    append->Delete();
    }
}



//------------------------------------------------------------------------------
void vtkVRMLSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->FileName)
    {
    os << indent << "FileName: " << this->FileName << endl;
    }
  os << indent << "Color: " << this->Color << endl;
  os << indent << "Append: " << this->Append << endl;
}

