



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
#include "vtkActorCollection.h"
#include "vtkActor.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkVRMLImporter.h"


vtkCxxRevisionMacro(vtkVRMLSource, "1.3");
vtkStandardNewMacro(vtkVRMLSource);

//------------------------------------------------------------------------------
vtkVRMLSource::vtkVRMLSource()
{
  this->FileName = NULL;
  this->Importer = NULL;
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

  if (this->Importer == NULL)
    {
    return;
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
      output = this->GetOutput(idx);
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
      ++idx;
      }
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
}

