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
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkPVOptions.h"
#include "vtkPVServerInformation.h"
#include "vtkRemoteConnection.h"
#include "vtkSocketController.h"

#ifdef PARAVIEW_USE_ICE_T
# include "vtkIceTSynchronizedRenderers.h"
#endif

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

  bool in_tile_display_mode = false;
  int tile_dims[2] = {0, 0};
  vtkPVServerInformation* server_info = NULL;
  if (pm->GetActiveRemoteConnection())
    {
    vtkIdType connectionID = pm->GetConnectionID(
      pm->GetActiveRemoteConnection());
    server_info = pm->GetServerInformation(connectionID);
    }
  else
    {
    server_info = pm->GetServerInformation(0);
    }
  tile_dims[0] = server_info->GetTileDimensions()[0];
  tile_dims[1] = server_info->GetTileDimensions()[1];
  in_tile_display_mode = (tile_dims[0] > 1 || tile_dims[1] > 1);
  cout << "in_tile_display_mode: " << in_tile_display_mode << endl;
  cout << "tile_dims: " << tile_dims[0] << ", " << tile_dims[1] << endl;

  switch (this->Mode)
    {
  case BUILTIN:
    break;

  case CLIENT:
      {
      if (in_tile_display_mode)
        {
        this->CSSynchronizer = vtkSynchronizedRenderers::New();
        this->CSSynchronizer->WriteBackImagesOff();
        }
      else
        {
        this->CSSynchronizer = vtkClientServerSynchronizedRenderers::New();
        this->CSSynchronizer->WriteBackImagesOn();
        }
      this->CSSynchronizer->SetRootProcessId(0);
      this->CSSynchronizer->SetParallelController(
        pm->GetActiveRenderServerSocketController());
      }
    break;


  case SERVER:
      {
      if (in_tile_display_mode)
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
#ifdef PARAVIEW_USE_ICE_T
      this->ParallelSynchronizer = vtkIceTSynchronizedRenderers::New();
      static_cast<vtkIceTSynchronizedRenderers*>(this->ParallelSynchronizer)->SetTileDimensions(
        tile_dims[0], tile_dims[1]);
#else
      // FIXME: need to add support for compositing.
      this->ParallelSynchronizer = vtkSynchronizedRenderers::New();
#endif
      this->ParallelSynchronizer->SetParallelController(
        vtkMultiProcessController::GetGlobalController());
      if ( (pm->GetPartitionId() == 0 && this->Mode == BATCH) ||
            in_tile_display_mode)
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

  // This ensures that CSSynchronizer simply fetches the captured buffer from
  // iceT without requiring icet to render to screen on the root node.
  if (this->ParallelSynchronizer && this->CSSynchronizer)
    {
    this->CSSynchronizer->SetCaptureDelegate(
      this->ParallelSynchronizer);
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
