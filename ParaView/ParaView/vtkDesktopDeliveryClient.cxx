// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDesktopDeliveryClient.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

=========================================================================*/

#include "vtkDesktopDeliveryClient.h"
#include "vtkDesktopDeliveryServer.h"

#include <vtkObjectFactory.h>
#include <vtkRenderWindow.h>
#include <vtkCallbackCommand.h>
#include <vtkCubeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkRendererCollection.h>
#include <vtkCamera.h>
#include <vtkLight.h>
#include <vtkTimerLog.h>
#include <vtkLightCollection.h>
#include <vtkFloatArray.h>
#include <vtkUnsignedCharArray.h>
#include "vtkMultiProcessController.h"

//#include <vtkRef.h>

vtkCxxRevisionMacro(vtkDesktopDeliveryClient, "1.1.2.2");
vtkStandardNewMacro(vtkDesktopDeliveryClient);

vtkDesktopDeliveryClient::vtkDesktopDeliveryClient()
{
  this->ReplaceActors = 1;
  this->Squirt = 0;
  this->SquirtCompressionLevel = 5;
  this->SquirtBuffer = vtkUnsignedCharArray::New();
}

vtkDesktopDeliveryClient::~vtkDesktopDeliveryClient()
{
  this->SquirtBuffer->Delete();
}

void
vtkDesktopDeliveryClient::SetController(vtkMultiProcessController *controller)
{
  vtkDebugMacro("SetController");

  if (controller && (controller->GetNumberOfProcesses() != 2))
    {
    vtkErrorMacro("vtkDesktopDelivery needs controller with 2 processes");
    return;
    }

  this->Superclass::SetController(controller);

  if (this->Controller)
    {
    this->RootProcessId = this->Controller->GetLocalProcessId();
    this->ServerProcessId = 1 - this->RootProcessId;
    }
}

void vtkDesktopDeliveryClient::SetRenderWindow(vtkRenderWindow *renWin)
{
  //Make sure the renWin has at least one renderer
  if (renWin)
    {
    vtkRendererCollection *rens = renWin->GetRenderers();
    if (rens->GetNumberOfItems() < 1)
      {
      vtkRenderer *ren = vtkRenderer::New();
      renWin->AddRenderer(ren);
      ren->Delete();
      }
    }

  this->Superclass::SetRenderWindow(renWin);
}

void vtkDesktopDeliveryClient::SendWindowInformation()
{
  vtkDesktopDeliveryServer::SquirtOptions squirt_options;
  squirt_options.Enabled = this->Squirt;
  squirt_options.CompressLevel = this->SquirtCompressionLevel;

  this->Controller->Send((int *)(&squirt_options),
             vtkDesktopDeliveryServer::SQUIRT_OPTIONS_SIZE,
             this->ServerProcessId,
             vtkDesktopDeliveryServer::SQUIRT_OPTIONS_TAG);
}

void vtkDesktopDeliveryClient::PreRenderProcessing()
{
  // Get remote display flag
  this->Controller->Receive(&this->RemoteDisplay, 1, this->ServerProcessId,
                vtkDesktopDeliveryServer::REMOTE_DISPLAY_TAG);

  if (this->RemoteDisplay)
    {
    // Turn swap buffers off so we can put the final image into the render
    // window before it is shown to the viewer.
    this->RenderWindow->SwapBuffersOff();
    }
  else
    {
    if (this->ImageReductionFactor > 1)
      {
      // Since we're not replacing the image, restore the renderer viewports.
      vtkRendererCollection *rens = this->RenderWindow->GetRenderers();
      vtkRenderer *ren;
      int i;
      for (rens->InitTraversal(), i = 0; ren = rens->GetNextItem(); i++)
    {
    ren->SetViewport(this->Viewports->GetTuple(i));
    }
      }
    }
}

#include "vtkImageData.h"
#include "vtkDataSetWriter.h"
#include "vtkPointData.h"

void vtkDesktopDeliveryClient::PostRenderProcessing()
{
  // Adjust render time for actual render on server.
  this->Timer->StartTimer();
  this->Controller->Barrier();
  this->Timer->StopTimer();
  this->RenderTime += this->Timer->GetElapsedTime();

  vtkDesktopDeliveryServer::ImageParams ip;
  this->Controller->Receive((int *)(&ip),
                vtkDesktopDeliveryServer::IMAGE_PARAMS_SIZE,
                this->ServerProcessId,
                vtkDesktopDeliveryServer::IMAGE_PARAMS_TAG);

  if (ip.RemoteDisplay)
    {
    // Receive image.
    this->Timer->StartTimer();
    this->ReducedImage->SetNumberOfComponents(ip.NumberOfComponents);
    if (this->ImageReductionFactor == 1)
      {
      this->FullImage->SetNumberOfComponents(ip.NumberOfComponents);
      this->FullImage->SetNumberOfTuples(  this->FullImageSize[0]
                     * this->FullImageSize[1]);
      this->FullImageUpToDate = true;
      this->ReducedImage->SetArray(this->FullImage->GetPointer(0),
                   this->FullImage->GetSize(), 1);
      }
    this->ReducedImage->SetNumberOfTuples(  this->ReducedImageSize[0]
                      * this->ReducedImageSize[1]);

    if (ip.SquirtCompressed)
      {
      this->SquirtBuffer->SetNumberOfComponents(ip.NumberOfComponents);
      this->SquirtBuffer->SetNumberOfTuples(ip.Size/ip.NumberOfComponents);
      this->Controller->Receive(this->SquirtBuffer->GetPointer(0), ip.Size,
                this->ServerProcessId,
                vtkDesktopDeliveryServer::IMAGE_TAG);
      this->SquirtDecompress(this->SquirtBuffer, this->ReducedImage);
      }
    else
      {
      this->Controller->Receive(this->ReducedImage->GetPointer(0), ip.Size,
                this->ServerProcessId,
                vtkDesktopDeliveryServer::IMAGE_TAG);
      }
    this->ReducedImageUpToDate = true;

    this->Timer->StopTimer();
    this->TransferTime = this->Timer->GetElapsedTime();

    if (this->WriteBackImages)
      {
      if (this->MagnifyImages)
          {
          this->MagnifyReducedImage();
        /*
        if (this->ImageReductionFactor > 2)
          {
          vtkImageData* image = vtkImageData::New();
          image->SetDimensions(this->FullImageSize[0], this->FullImageSize[1], 1);
          image->SetNumberOfScalarComponents(4);
          image->SetScalarType(VTK_UNSIGNED_CHAR);
          image->GetPointData()->SetScalars(this->FullImage);
          vtkDataSetWriter* writer= vtkDataSetWriter::New();
          writer->SetFileTypeToBinary();
          writer->SetFileName("junkBug.vtk");
          writer->SetInput(image);
          writer->Write();
          writer->Delete();
          image->Delete();
          }
        */
        // Having problem with only small part of window set properly.
        // Mabe the view port ....
        //if (this->ImageReductionFactor > 1)
        //  {
        //  vtkRendererCollection *rens = this->RenderWindow->GetRenderers();
        //  vtkRenderer *ren;
        //  int i;
        //  for (rens->InitTraversal(), i = 0; ren = rens->GetNextItem(); i++)
        //    { // hard coded to full window.
         //   ren->SetViewport(0,0,1,1);
         //   }
         // }

          this->SetRenderWindowPixelData(this->FullImage, this->FullImageSize);
        }
      else
    {
    this->ReadReducedImage();
    this->SetRenderWindowPixelData(this->ReducedImage,
                       this->ReducedImageSize);
    }
      this->RenderWindowImageUpToDate = true;
      }

    // Force swap buffers here.
    this->RenderWindow->SwapBuffersOn();
    this->RenderWindow->Frame();
    }
  else
    {
    // No remote display means no transfer time.
    this->TransferTime = 0.0;

    // Leave the image in the window alone.
    this->RenderWindowImageUpToDate = true;
    }

  vtkDesktopDeliveryServer::TimingMetrics tm;
  this->Controller->Receive((double *)(&tm),
                vtkDesktopDeliveryServer::TIMING_METRICS_SIZE,
                this->ServerProcessId,
                vtkDesktopDeliveryServer::TIMING_METRICS_TAG);
  this->RemoteImageProcessingTime = tm.ImageProcessingTime;
}

void vtkDesktopDeliveryClient::ComputeVisiblePropBounds(vtkRenderer *ren,
                            float bounds[6])
{
  this->Superclass::ComputeVisiblePropBounds(ren, bounds);

  if (this->ReplaceActors)
    {
    vtkDebugMacro("Replacing actors.");

    ren->GetActors()->RemoveAllItems();

    vtkCubeSource* source = vtkCubeSource::New();
    source->SetBounds(bounds);

    vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
    mapper->SetInput(source->GetOutput());

    vtkActor* actor = vtkActor::New();
    actor->SetMapper(mapper);

    ren->AddActor(actor);
    source->Delete();
    mapper->Delete();
    actor->Delete();
    }
}

void vtkDesktopDeliveryClient
    ::SetImageReductionFactorForUpdateRate(float DesiredUpdateRate)
{
  this->Superclass::SetImageReductionFactorForUpdateRate(DesiredUpdateRate);
  if (this->Squirt)
    {
    if (this->ImageReductionFactor == 1)
      {
      this->SetSquirtCompressionLevel(0);
      }
    else
      {
      this->SetSquirtCompressionLevel(5);
      }
    }
}

void vtkDesktopDeliveryClient::SquirtDecompress(vtkUnsignedCharArray *in,
                        vtkUnsignedCharArray *out)
{
  int count=0;
  int index=0;
  unsigned int current_color;
  unsigned int *_rawColorBuffer;
  unsigned int *_rawCompressedBuffer;

  // Get compressed buffer size
  int CompSize = in->GetNumberOfTuples();

  // Access raw arrays directly
  _rawColorBuffer = (unsigned int *)out->GetPointer(0);
  _rawCompressedBuffer = (unsigned int *)in->GetPointer(0);

  // Go through compress buffer and extract RLE format into color buffer
  for(int i=0; i<CompSize; i++)
    {
    // Get color and count
    current_color = _rawCompressedBuffer[i];

    // Get run length count;
    count = ((unsigned char *)(&current_color))[3];

    // Fixed Alpha
    ((unsigned char *)(&current_color))[3] = 0xFF;

    // Set color
    _rawColorBuffer[index++] = current_color;

    // Blast color into color buffer
    for(int j=0; j < count; j++)
      {
      _rawColorBuffer[index++] = current_color;
      }
    }

  // Save out compression stats.
  vtkTimerLog::FormatAndMarkEvent("Squirt ratio: %f",
                  (float)CompSize/(float)index);
}

void vtkDesktopDeliveryClient::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ServerProcessId: " << this->ServerProcessId << endl;

  os << indent << "ReplaceActors: "
     << (this->ReplaceActors ? "on" : "off") << endl;
  os << indent << "RemoteDisplay: "
     << (this->RemoteDisplay ? "on" : "off") << endl;

  os << indent << "RemoteImageProcessingTime: "
     << this->RemoteImageProcessingTime << endl;
  os << indent << "TransferTime: " << this->TransferTime << endl;
}
