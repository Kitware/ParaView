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
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkVRMLImporter.h"


vtkCxxRevisionMacro(vtkVRMLSource, "1.1");
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
  int idx;  

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
      //mapper->GetInput()->Update();
      vtkPolyData *output = this->GetOutput(idx);
      output->ShallowCopy(mapper->GetInput());
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

