/*=========================================================================

  Program:   ParaView
  Module:    vtkPVStreamingParallelHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVStreamingParallelHelper.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVSynchronizedRenderWindows.h"

vtkStandardNewMacro(vtkPVStreamingParallelHelper);

//----------------------------------------------------------------------------
vtkPVStreamingParallelHelper::vtkPVStreamingParallelHelper()
{
  this->SynchronizedWindows = NULL;
}

//----------------------------------------------------------------------------
vtkPVStreamingParallelHelper::~vtkPVStreamingParallelHelper()
{
  this->SetSynchronizedWindows(NULL);
}

//----------------------------------------------------------------------------
void vtkPVStreamingParallelHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkPVStreamingParallelHelper::SetSynchronizedWindows
(vtkPVSynchronizedRenderWindows *nv)
{
  if (this->SynchronizedWindows == nv)
    {
    return;
    }
  if (this->SynchronizedWindows)
    {
    this->SynchronizedWindows->Delete();
    }
  this->SynchronizedWindows = nv;
  if (nv)
    {
    nv->Register(this);
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVStreamingParallelHelper::Reduce(bool &flag)
{
  if (!this->SynchronizedWindows)
    {
    return;
    }

  vtkPVSynchronizedRenderWindows::ModeEnum mode =
    this->SynchronizedWindows->GetMode();
  if (mode == vtkPVSynchronizedRenderWindows::INVALID ||
      mode == vtkPVSynchronizedRenderWindows::BUILTIN)
    return;

  vtkMultiProcessController* parallelController =
    this->SynchronizedWindows->GetParallelController();
  if (mode == vtkPVSynchronizedRenderWindows::BATCH &&
      parallelController->GetNumberOfProcesses() <= 1)
    {
    return;
    }
  int value = (int)flag;
  int result = value;
  if (parallelController)
    {
    //server nodes all continue if any needs to
    parallelController->AllReduce(&value, &result, 1,
                                  vtkCommunicator::LOGICAL_OR_OP);
    }
  value = result;

  vtkMultiProcessController* c_s_controller =
    this->SynchronizedWindows->GetClientServerController();
  switch (mode)
    {
    case vtkPVSynchronizedRenderWindows::CLIENT:
      //client just obeys what the server tells it
      c_s_controller->Receive(&value, 1, 1, STREAMING_REDUCE_TAG);
      break;
    default:
      //server tells client what to do
      //TODO: handle split ds/rs/client mode.
      if (c_s_controller)
        {
        c_s_controller->Send(&value, 1, 1, STREAMING_REDUCE_TAG);
        }
    }

  flag = value!=0; //convert back to bool without annoying msvc
}
