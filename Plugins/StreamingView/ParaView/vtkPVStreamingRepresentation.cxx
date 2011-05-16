/*=========================================================================

  Program:   ParaView
  Module:    vtkPVStreamingRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkPVStreamingRepresentation.cxx

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

#include "vtkPVStreamingRepresentation.h"

#include "vtkObjectFactory.h"
#include "vtkPieceCacheFilter.h"
#include "vtkPVStreamingView.h"
#include "vtkStreamingDriver.h"
#include "vtkStreamingHarness.h"

vtkStandardNewMacro(vtkPVStreamingRepresentation);

//----------------------------------------------------------------------------
vtkPVStreamingRepresentation::vtkPVStreamingRepresentation()
{
  this->PieceCache = NULL;
  this->Harness = NULL;
}

//----------------------------------------------------------------------------
vtkPVStreamingRepresentation::~vtkPVStreamingRepresentation()
{
  this->SetPieceCache(NULL);
  this->SetHarness(NULL);
}

//----------------------------------------------------------------------------
void vtkPVStreamingRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVStreamingRepresentation::SetPieceCache(vtkPieceCacheFilter *PCF)
{
  if (this->PieceCache == PCF)
    {
    return;
    }
  this->Modified();
  if (this->PieceCache)
    {
    this->PieceCache->Delete();
    }
  this->PieceCache = PCF;
  if (this->PieceCache)
    {
    this->PieceCache->Register(this);
    }
  if (this->Harness)
    {
    this->Harness->SetCacheFilter(this->PieceCache);
    }
}

//----------------------------------------------------------------------------
void vtkPVStreamingRepresentation::SetHarness(vtkStreamingHarness *harness)
{
  if (this->Harness == harness)
    {
    return;
    }
  this->Modified();
  if (this->Harness)
    {
    this->Harness->Delete();
    }
  this->Harness = harness;
  if (this->Harness)
    {
    this->Harness->Register(this);
    this->Harness->SetCacheFilter(this->PieceCache);
    }
}

//------------------------------------------------------------------------------
bool vtkPVStreamingRepresentation::AddToView(vtkView *view)
{
  vtkPVStreamingView *sView = vtkPVStreamingView::SafeDownCast(view);
  if (!sView)
    {
    return false;
    }
  bool ret = this->Superclass::AddToView(view);
  if (ret)
    {
    vtkStreamingDriver *sd = sView->GetStreamDriver();
    if (sd && this->Harness)
      {
      sd->AddHarness(this->Harness);
      }
    }
  return ret;
}

//------------------------------------------------------------------------------
bool vtkPVStreamingRepresentation::RemoveFromView(vtkView *view)
{
  vtkPVStreamingView *sView = vtkPVStreamingView::SafeDownCast(view);
  if (!sView)
    {
    return false;
    }

  vtkStreamingDriver *sd = sView->GetStreamDriver();
  if (sd && this->Harness)
    {
    sd->RemoveHarness(this->Harness);
    }

  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkPVStreamingRepresentation::MarkModified()
{
  this->Superclass::MarkModified();
  if (this->Harness)
    {
    this->Harness->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkPVStreamingRepresentation::SetVisibility(bool val)
{
  this->Superclass::SetVisibility(val);
  if (this->Harness)
    {
    this->Harness->SetEnabled(val);
    }
}

//----------------------------------------------------------------------------
void vtkPVStreamingRepresentation::SetSelectionVisibility(bool vtkNotUsed(vis))
{
  this->Superclass::SetSelectionVisibility(false);
}
