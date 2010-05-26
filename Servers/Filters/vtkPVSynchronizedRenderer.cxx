/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSynchronizedRenderer.h"

#include "vtkClientServerSynchronizedRenderers.h"
#include "vtkIceTSynchronizedRenderers.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkPVServerInformation.h"
#include "vtkRemoteConnection.h"
#include "vtkSocketController.h"

vtkStandardNewMacro(vtkPVSynchronizedRenderer);
vtkCxxRevisionMacro(vtkPVSynchronizedRenderer, "$Revision$");
//----------------------------------------------------------------------------
vtkPVSynchronizedRenderer::vtkPVSynchronizedRenderer()
{
  this->Enabled = true;
  this->ImageReductionFactor = 1;

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm)
    {
    vtkErrorMacro(
      "vtkPVSynchronizedRenderWindows cannot be used in the current\n"
      "setup. Aborting for debugging purposes.");
    abort();
    }

  if (pm->GetActiveRemoteConnection() == NULL)
    {
    this->Mode = BUILTIN;
    if (pm->GetNumberOfLocalPartitions() > 1)
      {
      this->Mode = BATCH;
      }
    }
  else if (pm->GetActiveRemoteConnection()->IsA("vtkClientConnection"))
    {
    this->Mode = SERVER;
    }
  else if (pm->GetActiveRemoteConnection()->IsA("vtkServerConnection"))
    {
    this->Mode = CLIENT;
    }


  this->CSSynchronizer = 0;
  this->ParallelSynchronizer = 0;

  switch (this->Mode)
    {
  case BUILTIN:
    break;

  case CLIENT:
      {
      vtkIdType connectionID = pm->GetConnectionID(
        pm->GetActiveRemoteConnection());
      vtkPVServerInformation* server_info = pm->GetServerInformation(
        connectionID);
      if (server_info->GetTileDimensions()[0])
        {
        this->CSSynchronizer = vtkSynchronizedRenderers::New();
        this->CSSynchronizer->WriteBackImagesOff();
        }
      else
        {
        this->CSSynchronizer = vtkClientServerSynchronizedRenderers::New();
        this->CSSynchronizer->WriteBackImagesOff();
        }
      this->CSSynchronizer->SetRootProcessId(0);
      this->CSSynchronizer->SetParallelController(
        pm->GetActiveRenderServerSocketController());
      }
    break;


  case SERVER:
      {
      vtkPVOptions* options = pm->GetOptions();
      if (options->GetTileDimensions()[0])
        {
        this->CSSynchronizer = vtkSynchronizedRenderers::New();
        }
      else
        {
        this->CSSynchronizer = vtkClientServerSynchronizedRenderers::New();
        }
      this->CSSynchronizer->WriteBackImagesOff();
      this->CSSynchronizer->SetRootProcessId(1);
      this->CSSynchronizer->SetParallelController(
        pm->GetActiveRenderServerSocketController());
      }

    // DONT BREAK
    // break;

  case BATCH:
    if (pm->GetNumberOfLocalPartitions() > 1)
      {
      this->ParallelSynchronizer = vtkIceTSynchronizedRenderers::New();
      this->ParallelSynchronizer->SetParallelController(
        vtkMultiProcessController::GetGlobalController());
      if (pm->GetPartitionId() == 0 && this->Mode == BATCH)
        {
        this->ParallelSynchronizer->WriteBackImagesOn();
        }
      else
        {
        this->ParallelSynchronizer->WriteBackImagesOff();
        }
      this->ParallelSynchronizer->SetRootProcessId(0);
      }
    break;


  default: abort();
    }
}

//----------------------------------------------------------------------------
vtkPVSynchronizedRenderer::~vtkPVSynchronizedRenderer()
{
  this->SetRenderer(0);
  if (this->ParallelSynchronizer)
    {
    this->ParallelSynchronizer->Delete();
    this->ParallelSynchronizer = 0;
    }
  if (this->CSSynchronizer)
    {
    this->CSSynchronizer->Delete();
    this->CSSynchronizer = 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetRenderer(vtkRenderer* ren)
{
  if (this->ParallelSynchronizer)
    {
    this->ParallelSynchronizer->SetRenderer(ren);
    }
  if (this->CSSynchronizer)
    {
    this->CSSynchronizer->SetRenderer(ren);
    }
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
