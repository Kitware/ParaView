// -*- c++ -*-

#include "vtkCompositeRenderManager.h"

#include <vtkObjectFactory.h>
#include <vtkCompressCompositer.h>
#include <vtkUnsignedCharArray.h>
#include <vtkFloatArray.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRendererCollection.h>
#include <vtkTimerLog.h>

vtkCxxRevisionMacro(vtkCompositeRenderManager, "1.2.2.3");
vtkStandardNewMacro(vtkCompositeRenderManager);
vtkCxxSetObjectMacro(vtkCompositeRenderManager,Compositer,vtkCompositer);

vtkCompositeRenderManager::vtkCompositeRenderManager()
{
  this->Compositer = vtkCompressCompositer::New();
  this->Compositer->Register(this);
  this->Compositer->Delete();

  this->DepthData = vtkFloatArray::New();
  this->TmpPixelData = vtkUnsignedCharArray::New();
  this->TmpDepthData = vtkFloatArray::New();

  this->DepthData->SetNumberOfComponents(1);
  this->TmpPixelData->SetNumberOfComponents(4);
  this->TmpDepthData->SetNumberOfComponents(1);
}

vtkCompositeRenderManager::~vtkCompositeRenderManager()
{
  this->SetCompositer(NULL);
  this->DepthData->Delete();
  this->TmpPixelData->Delete();
  this->TmpDepthData->Delete();
}

void vtkCompositeRenderManager::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Compositer: " << endl;
  this->Compositer->PrintSelf(os, indent.GetNextIndent());
}

void vtkCompositeRenderManager::PreRenderProcessing()
{
  // Turn swap buffers off before the render so the end render method has a
  // chance to add the back buffer.
  this->RenderWindow->SwapBuffersOff();
}

void vtkCompositeRenderManager::PostRenderProcessing()
{
  if (this->Controller->GetNumberOfProcesses() > 1)
    {
    // Read in data.
    this->ReadReducedImage();
    this->Timer->StartTimer();
    this->RenderWindow->GetZbufferData(0, 0, this->ReducedImageSize[0]-1,
                       this->ReducedImageSize[1]-1,
                       this->DepthData);

    // Set up temporary buffers.
    this->TmpPixelData->SetNumberOfComponents
      (this->ReducedImage->GetNumberOfComponents());
    this->TmpPixelData->SetNumberOfTuples
      (this->ReducedImage->GetNumberOfTuples());
    this->TmpDepthData->SetNumberOfComponents
      (this->DepthData->GetNumberOfComponents());
    this->TmpDepthData->SetNumberOfTuples(this->DepthData->GetNumberOfTuples());

    // Do composite
    this->Compositer->SetController(this->Controller);
    this->Compositer->CompositeBuffer(this->ReducedImage, this->DepthData,
                      this->TmpPixelData, this->TmpDepthData);

    this->Timer->StopTimer();
    this->ImageProcessingTime = this->Timer->GetElapsedTime();
    }

  this->WriteFullImage();

  // Swap buffers here
  this->RenderWindow->SwapBuffersOn();
  this->RenderWindow->Frame();
}


