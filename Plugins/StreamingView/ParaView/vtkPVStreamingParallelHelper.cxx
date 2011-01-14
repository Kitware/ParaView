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

//#include <unistd.h>

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
//  static int cnt = 0;
//  cerr << getpid() << " " << this << " " << cnt
//       << " SReduce < " << (flag?"TRUE":"FALSE") << endl;
  if (this->SynchronizedWindows)
    {
    unsigned int value = (unsigned int)flag;
    int mode = this->SynchronizedWindows->GetMode();
    vtkMultiProcessController* c_ds_controller = NULL;
    switch (mode)
      {
      case vtkPVSynchronizedRenderWindows::INVALID:
      case vtkPVSynchronizedRenderWindows::BUILTIN:
        return;
      case vtkPVSynchronizedRenderWindows::CLIENT:
        c_ds_controller =
          this->SynchronizedWindows->GetClientServerController();
        c_ds_controller->Receive(&value, 1, 0, 99999);
        break;
      default:
        c_ds_controller =
          this->SynchronizedWindows->GetClientServerController();
        c_ds_controller->Send(&value, 1, 1, 99999);
      }
    flag = (bool)value;
    }
//  cerr << getpid() << " " << this << " " << cnt
//       << " EReduce > " << (flag?"TRUE":"FALSE") << endl;
//  cnt++;
}
