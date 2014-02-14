/*=========================================================================

   Program: ParaView
   Module:    vtkPVRenderViewForAssembly.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVRenderViewForAssembly.h"

#include "vtkDataSetWriter.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVSynchronizedRenderWindows.h"
#include "vtkPVSynchronizedRenderer.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWindowToImageFilter.h"

vtkStandardNewMacro(vtkPVRenderViewForAssembly);
//----------------------------------------------------------------------------
vtkPVRenderViewForAssembly::vtkPVRenderViewForAssembly()
{
  this->InRender = false;
  this->FileName = NULL;
  this->ClippingBounds.Reset();
}

//----------------------------------------------------------------------------
vtkPVRenderViewForAssembly::~vtkPVRenderViewForAssembly()
{
  this->SetFileName(NULL);
}

//----------------------------------------------------------------------------
void vtkPVRenderViewForAssembly::Initialize(unsigned int id)
{
  this->Superclass::Initialize(id);
  this->SynchronizedRenderers->SetUseDepthBuffer(true);
}

//----------------------------------------------------------------------------
void vtkPVRenderViewForAssembly::Render(bool interactive, bool skip_rendering)
{
  if (this->GetMakingSelection() || skip_rendering || interactive || this->InRender)
    {
    this->Superclass::Render(interactive, skip_rendering);
    return;
    }

  this->InRender = true;
  for (int cc=0; cc < 1; cc++)
    {
    this->StillRender();

    if ( this->SynchronizedWindows->GetLocalProcessIsDriver()
         && this->FileName && strlen(this->FileName) > 0)
      {
      // update buffers
      this->GetRGBAData();
      this->GetZBufferData();

      // Combined array into a single ImageData
      vtkNew<vtkUnsignedCharArray> rgbArray;
      rgbArray->SetName("rgb");
      vtkNew<vtkFloatArray> zArray;
      zArray->DeepCopy(this->ZBufferData->GetPointData()->GetScalars());
      zArray->SetName("z");

      // Fill RGB array and set Z to 0 if Alpha == 0
      vtkUnsignedCharArray* rgbaArraySrc =
          vtkUnsignedCharArray::SafeDownCast(
            this->RGBAData->GetPointData()->GetScalars());
      rgbArray->SetNumberOfComponents(3);
      rgbArray->SetNumberOfTuples(rgbaArraySrc->GetNumberOfTuples());
      for(vtkIdType idx = 0, count = rgbaArraySrc->GetNumberOfTuples(); idx < count; ++idx)
        {
        rgbArray->SetValue(idx*3 + 0, rgbaArraySrc->GetValue(idx*4 + 0));
        rgbArray->SetValue(idx*3 + 1, rgbaArraySrc->GetValue(idx*4 + 1));
        rgbArray->SetValue(idx*3 + 2, rgbaArraySrc->GetValue(idx*4 + 2));
        if(rgbaArraySrc->GetValue(idx*4 + 3) == 0)
          {
          zArray->SetValue(idx, 2.0f);
          }
        }

      vtkNew<vtkImageData> combinedData;
      combinedData->SetDimensions(this->RGBAData->GetDimensions());
      combinedData->SetSpacing(this->RGBAData->GetSpacing());
      combinedData->SetOrigin(this->RGBAData->GetOrigin());
      combinedData->GetPointData()->SetScalars(rgbArray.GetPointer());
      combinedData->GetPointData()->AddArray(zArray.GetPointer());

      // Write to disk
      vtkNew<vtkDataSetWriter> writer;
      writer->SetFileTypeToBinary();
      writer->SetFileName(this->FileName);
      writer->SetInputData(combinedData.GetPointer());
      writer->Update();
      }
    }
  this->InRender = false;
}

//----------------------------------------------------------------------------
void vtkPVRenderViewForAssembly::SetClippingBounds( double b1, double b2,
                                                    double b3, double b4,
                                                    double b5, double b6)
{
  double bds[6] = {b1, b2, b3, b4, b5, b6};
  this->SetClippingBounds(bds);
}

//----------------------------------------------------------------------------
void vtkPVRenderViewForAssembly::ResetCameraClippingRange()
{
  if (this->ClippingBounds.IsValid())
    {
    double bounds[6];
    this->ClippingBounds.GetBounds(bounds);
    this->GetRenderer()->ResetCameraClippingRange(bounds);
    this->GetNonCompositedRenderer()->ResetCameraClippingRange(bounds);
    }
  else
    {
    this->Superclass::ResetCameraClippingRange();
    }
}

//----------------------------------------------------------------------------
vtkImageData* vtkPVRenderViewForAssembly::GetRGBAData()
{
  vtkNew<vtkWindowToImageFilter> w2i;
  w2i->SetInput(this->GetRenderWindow());
  w2i->ReadFrontBufferOn();
  w2i->FixBoundaryOff();
  w2i->ShouldRerenderOff();
  w2i->SetMagnification(1);
  w2i->SetInputBufferTypeToRGBA();
  w2i->Update();
  this->RGBAData = w2i->GetOutput();
  return this->RGBAData;
}

//----------------------------------------------------------------------------
vtkImageData* vtkPVRenderViewForAssembly::GetZBufferData()
{
  vtkNew<vtkWindowToImageFilter> w2i;
  w2i->SetInput(this->GetRenderWindow());
  w2i->ReadFrontBufferOn();
  w2i->FixBoundaryOff();
  w2i->ShouldRerenderOff();
  w2i->SetMagnification(1);
  w2i->SetInputBufferTypeToZBuffer();
  w2i->Update();
  this->ZBufferData = w2i->GetOutput();
  return this->ZBufferData;
}

//----------------------------------------------------------------------------
void vtkPVRenderViewForAssembly::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVRenderViewForAssembly::ResetClippingBounds()
{
  this->ClippingBounds.Reset();
}

//----------------------------------------------------------------------------
void vtkPVRenderViewForAssembly::FreezeGeometryBounds()
{
  this->ClippingBounds.Reset();
  this->ClippingBounds.AddBox(this->GeometryBounds);
}
