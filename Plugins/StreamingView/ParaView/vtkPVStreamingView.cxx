/*=========================================================================

  Program:   ParaView
  Module:    vtkPVStreamingView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkPVStreamingView.cxx

Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC.
This software was produced under U.S. Government contract DE-AC52-06NA25396
for Los Alamos National Laboratory (LANL), which is operated by
Los Alamos National Security, LLC for the U.S. Department of Energy.
The U.S. Government has rights to use, reproduce, and distribute this software.
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
If software is modified to produce derivative works, such modified software
should be clearly marked, so as not to confuse it with the version available
from LANL.

Additionally, redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions
are met:
-   Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkPVStreamingView.h"

#include "vtkMath.h"
#include "vtkMultiResolutionStreamer.h"
#include "vtkObjectFactory.h"
#include "vtkPVStreamingParallelHelper.h"
#include "vtkPVStreamingRepresentation.h"
#include "vtkPVSynchronizedRenderWindows.h"
#include "vtkRenderer.h"
#include "vtkStreamingDriver.h"

vtkStandardNewMacro(vtkPVStreamingView);

void vtkPVStreamingViewRenderLaterFunction(void *instance)
{
  vtkPVStreamingView *self = static_cast<vtkPVStreamingView*>(instance);
  self->RenderSchedule();
}

//----------------------------------------------------------------------------
vtkPVStreamingView::vtkPVStreamingView()
{
  this->StreamDriver = NULL;
  this->IsDisplayDone = 1;
  vtkMath::UninitializeBounds(this->RunningBounds);
  this->LastRenderWasStill = false;
}

//----------------------------------------------------------------------------
vtkPVStreamingView::~vtkPVStreamingView()
{
  this->SetStreamDriver(NULL);
}

//----------------------------------------------------------------------------
void vtkPVStreamingView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVStreamingView::SetStreamDriver(vtkStreamingDriver *nd)
{
  if (this->StreamDriver == nd)
    {
    return;
    }
  this->Modified();
  if (this->StreamDriver)
    {
    this->StreamDriver->Delete();
    }
  this->StreamDriver = nd;
  if (this->StreamDriver)
    {
    this->StreamDriver->Register(this);
    this->StreamDriver->SetManualStart(true);
    this->StreamDriver->SetManualFinish(true);
    this->StreamDriver->SetRenderWindow(this->GetRenderWindow());
    this->StreamDriver->SetRenderer(this->GetRenderer());
    this->StreamDriver->AssignRenderLaterFunction
      (vtkPVStreamingViewRenderLaterFunction, this);

    vtkPVStreamingParallelHelper *helper = vtkPVStreamingParallelHelper::New();
    helper->SetSynchronizedWindows(this->SynchronizedWindows);
    this->StreamDriver->SetParallelHelper(helper);
    helper->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkPVStreamingView::Render(bool interactive, bool skip_rendering)
{
  bool changed = this->LastRenderWasStill != interactive;
  this->LastRenderWasStill = interactive;

  //set flag that gui watches to schedule more renders,
  //assume we are done, and correct later if needed
  this->IsDisplayDone = 1;

  bool render_event_propagation =
    this->SynchronizedWindows->GetRenderEventPropagation();
  this->SynchronizedWindows->RenderEventPropagationOff();

  if (this->StreamDriver)
    {
    if (changed)
      {
      //prevent refinement while dragging mouse to make it more responsive
      vtkMultiResolutionStreamer *msr = vtkMultiResolutionStreamer::SafeDownCast
        (this->StreamDriver);
      if (msr)
        {
        if (interactive)
          {
          msr->SetInteracting(true);
          }
        else
          {
          msr->SetInteracting(false);
          }
        }

      //when mode changes, be sure not to skip anything
      this->StreamDriver->RestartStreaming();
      }

    //figure out what piece to show now
    this->StreamDriver->StartRenderEvent();

    //be sure to update pipeline far enough to get it
    int num_reprs = this->GetNumberOfRepresentations();
    for (int cc=0; cc < num_reprs; cc++)
      {
      vtkDataRepresentation* repr = this->GetRepresentation(cc);
      vtkPVStreamingRepresentation* pvrepr =
        vtkPVStreamingRepresentation::SafeDownCast(repr);
      if (pvrepr)
        {
        pvrepr->MarkModified();
        }
      }

    // Since the vtkPVStreamingView modifies representations, we need to update
    // them explicitly.
    this->Update();
    }

  this->Superclass::Render(interactive, skip_rendering);

  if (this->StreamDriver)
    {
    //figure out what to do next
    this->StreamDriver->EndRenderEvent();
    }

  this->SynchronizedWindows->SetRenderEventPropagation
    (render_event_propagation);
}

//----------------------------------------------------------------------------
void vtkPVStreamingView::RenderSchedule()
{
  //let GUI know that we are not done yet
  this->IsDisplayDone = 0;
}

//----------------------------------------------------------------------------
void vtkPVStreamingView::ResetCameraClippingRange()
{
  //extend the bounds we use whenever we find a piece that is outside of what
  //we've used before.
  int i;
  for (i = 0; i < 6; i+=2)
    {
    if (this->LastComputedBounds[i] < this->RunningBounds[i])
      {
      this->RunningBounds[i] = this->LastComputedBounds[i];
      }
    }
  for (i = 1; i < 6; i+=2)
    {
    if (this->LastComputedBounds[i] > this->RunningBounds[i])
      {
      this->RunningBounds[i] = this->LastComputedBounds[i];
      }
    }
  for (i = 0; i<6; i++)
    {
    this->LastComputedBounds[i] = this->RunningBounds[i];
    }

  this->GetRenderer()->ResetCameraClippingRange(this->LastComputedBounds);
  this->GetNonCompositedRenderer()->ResetCameraClippingRange
    (this->LastComputedBounds);
}
