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

#include "vtkDataRepresentation.h"
#include "vtkDataSetWriter.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkJPEGWriter.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPNGWriter.h"
#include "vtkPointData.h"
#include "vtkPVAxesWidget.h"
#include "vtkPVCenterAxesActor.h"
#include "vtkPVConfig.h" // needed for PARAVIEW_USE_ICE_T
#include "vtkPVDataRepresentation.h"
#include "vtkPVSynchronizedRenderer.h"
#include "vtkPVSynchronizedRenderWindows.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkTIFFWriter.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWeakPointer.h"
#include "vtkWindowToImageFilter.h"

#ifdef PARAVIEW_USE_ICE_T
# include "vtkIceTSynchronizedRenderers.h"
#endif

#include <map>
#include <set>
#include <string>

//----------------------------------------------------------------------------
namespace {
  struct PixelOrder
  {
    float z;
    unsigned char idx;

    PixelOrder(unsigned char cIdx, float zBuffer)
    {
      this->idx = cIdx;
      this->z = zBuffer;
    }

    bool operator< (const PixelOrder& other) const
    {
      return this->z < other.z;
    }
  };
};

//----------------------------------------------------------------------------
struct vtkPVRenderViewForAssembly::vtkInternals {

  vtkInternals(vtkPVRenderViewForAssembly* owner)
  {
    this->Owner = owner;

    this->ZGrabber->SetInput(this->Owner->GetRenderWindow());
    this->ZGrabber->ReadFrontBufferOn();
    this->ZGrabber->FixBoundaryOff();
    this->ZGrabber->ShouldRerenderOff();
    this->ZGrabber->SetMagnification(1);
    this->ZGrabber->SetInputBufferTypeToZBuffer();

    this->ImageGrabber->SetInput(this->Owner->GetRenderWindow());
    this->ImageGrabber->ReadFrontBufferOn();
    this->ImageGrabber->FixBoundaryOff();
    this->ImageGrabber->ShouldRerenderOff();
    this->ImageGrabber->SetMagnification(1);
    this->ImageGrabber->SetInputBufferTypeToRGB();

    this->JPEGWriter->SetInputData(this->ImageStack.GetPointer());
    this->TIFFWriter->SetInputData(this->ImageStack.GetPointer());
    this->PNGWriter->SetInputData(this->ImageStack.GetPointer());
  }

  //----------------------------------------------------------------------------

  void AddRepresentation(vtkPVDataRepresentation* r)
  {
    this->CompositeRepresentations.push_back(r);
  }

  //----------------------------------------------------------------------------

  void RemoveRepresentation(vtkPVDataRepresentation* r)
  {
    std::vector<vtkWeakPointer<vtkPVDataRepresentation> >::iterator iter;
    for(iter = this->CompositeRepresentations.begin(); iter != this->CompositeRepresentations.end(); ++iter)
      {
      if(*iter == r)
        {
        break;
        }
      }
    if(iter != this->CompositeRepresentations.end())
      {
      this->CompositeRepresentations.erase(iter);
      }
  }

  //----------------------------------------------------------------------------

  void StoreVisibilityState()
  {
    int index = 0;
    std::vector<vtkWeakPointer<vtkPVDataRepresentation> >::iterator iter;
    for(iter = this->CompositeRepresentations.begin(); iter != this->CompositeRepresentations.end(); ++iter)
      {
      vtkPVDataRepresentation* rep = vtkPVDataRepresentation::SafeDownCast(*iter);
      this->VisibilityState[index++] = rep ? rep->GetVisibility() : false;
      }
  }

  //----------------------------------------------------------------------------

  void RestoreVisibilityState()
  {
    int index = 0;
    std::vector<vtkWeakPointer<vtkPVDataRepresentation> >::iterator iter;
    for(iter = this->CompositeRepresentations.begin(); iter != this->CompositeRepresentations.end(); ++iter)
      {
      vtkPVDataRepresentation* rep = vtkPVDataRepresentation::SafeDownCast(*iter);
      if(rep)
        {
        rep->SetVisibility(this->VisibilityState[index]);
        }
      index++;
      }
  }

  //----------------------------------------------------------------------------

  void ClearVisibility()
  {
    std::vector<vtkWeakPointer<vtkPVDataRepresentation> >::iterator iter;
    for(iter = this->CompositeRepresentations.begin(); iter != this->CompositeRepresentations.end(); ++iter)
      {
      vtkPVDataRepresentation* rep = vtkPVDataRepresentation::SafeDownCast(*iter);
      if(rep)
        {
        rep->SetVisibility(false);
        }
      }
  }

  //----------------------------------------------------------------------------

  int GetNumberOfRepresentations()
  {
    return static_cast<int>(this->CompositeRepresentations.size());
  }

  //----------------------------------------------------------------------------

  void UpdateVisibleRepresentation(int idx)
  {
    this->ClearVisibility();
    vtkPVDataRepresentation* rep = vtkPVDataRepresentation::SafeDownCast(this->CompositeRepresentations[idx]);
    if(rep)
      {
      rep->SetVisibility(true);
      }
  }

  //----------------------------------------------------------------------------

  vtkFloatArray* CaptureZBuffer()
  {
  this->ArrayHolder = vtkSmartPointer<vtkFloatArray>::New();
#ifdef PARAVIEW_USE_ICE_T
  if (this->IceTSynchronizedRenderers)
    {
    vtkIceTCompositePass* iceTPass = this->IceTSynchronizedRenderers->GetIceTCompositePass();
    if (iceTPass && iceTPass->GetLastRenderedDepths())
      {
      this->ArrayHolder->DeepCopy(iceTPass->GetLastRenderedDepths());
      }
    }
  else
#endif
    {
    this->ZGrabber->Modified();
    this->ZGrabber->Update();
    this->ArrayHolder->DeepCopy(this->ZGrabber->GetOutput()->GetPointData()->GetScalars());
    }

    return this->ArrayHolder;
  }

  //----------------------------------------------------------------------------

  void CaptureImage(int idx)
  {
    if(idx > this->Owner->GetRGBStackSize())
      {
      return;
      }

    // Local vars
    int width = this->Owner->GetSize()[0];
    int height = this->Owner->GetSize()[1];

    // Capture RGB buffer
    this->ImageGrabber->Modified();
    this->ImageGrabber->Update();

    // Ensure buffer stack is the right size
    if(idx == 0)
      {
      int nbImages = this->Owner->GetRGBStackSize();

      this->ImageStack->SetDimensions(width, height * nbImages, 1);
      this->ImageStack->GetPointData()->Reset();
      vtkNew<vtkUnsignedCharArray> rgbStack;
      rgbStack->SetName("RGB");
      rgbStack->SetNumberOfComponents(3);
      rgbStack->SetNumberOfTuples(width * height * nbImages);
      this->ImageStack->GetPointData()->SetScalars(rgbStack.GetPointer());
      this->RGBBuffer = rgbStack.GetPointer();
      }

    // Copy buffer into buffer stack
    vtkUnsignedCharArray* rgb =
        vtkUnsignedCharArray::SafeDownCast(
          this->ImageGrabber->GetOutput()->GetPointData()->GetScalars());
    vtkIdType offset = idx * width * height * 3;
    vtkIdType count = rgb->GetNumberOfTuples();
    vtkIdType position = 0;
    while(count--)
      {
      position = count*3;
      this->RGBBuffer->SetValue(offset + position + 0, rgb->GetValue(position + 0));
      this->RGBBuffer->SetValue(offset + position + 1, rgb->GetValue(position + 1));
      this->RGBBuffer->SetValue(offset + position + 2, rgb->GetValue(position + 2));
      }
  }
  //----------------------------------------------------------------------------

  void WriteImage()
  {
    if(this->ActiveWriter == NULL)
      {
      if(this->Owner->ImageFormatExtension == NULL || !strcmp("jpg", this->Owner->ImageFormatExtension))
        {
        this->Owner->SetImageFormatExtension("jpg");
        this->ActiveWriter = this->JPEGWriter.GetPointer();
        }
      else if(!strcmp("png", this->Owner->ImageFormatExtension))
        {
        this->ActiveWriter = this->PNGWriter.GetPointer();
        }
      else if(!strcmp("tiff", this->Owner->ImageFormatExtension))
        {
        this->ActiveWriter = this->TIFFWriter.GetPointer();
        }
      else
        {
        this->Owner->SetImageFormatExtension("jpg");
        this->ActiveWriter = this->JPEGWriter.GetPointer();
        }
      }

    vtkTimerLog::MarkStartEvent( "WriteRGBImageToDisk" );
    // Write image
    std::stringstream ss;
    ss << this->Owner->GetCompositeDirectory() << "/rgb." << this->Owner->ImageFormatExtension;
    this->ActiveWriter->SetFileName(ss.str().c_str());
    this->ActiveWriter->Modified();
    this->ActiveWriter->Write();
    vtkTimerLog::MarkEndEvent( "WriteRGBImageToDisk" );
  }

  //----------------------------------------------------------------------------

  char GetCodeForIdx(int idx)
  {
    return vtkPVRenderViewForAssembly::vtkInternals::CODING_TABLE[idx];
  }

  //----------------------------------------------------------------------------

  void Encode(char* buffer, int& bufferOffset, std::set<PixelOrder> orderFinder)
  {
    int startIdx = bufferOffset;
    buffer[bufferOffset++] = '+';
    buffer[bufferOffset] = '\0';

    std::set<PixelOrder>::iterator iter;
    for(iter = orderFinder.begin(); iter != orderFinder.end(); ++iter)
      {
      if(iter->idx > 0 && iter->z < 1.0f)
        {
        buffer[bufferOffset - 1] = vtkPVRenderViewForAssembly::vtkInternals::CODING_TABLE[iter->idx];
        buffer[bufferOffset++] = '+';
        buffer[bufferOffset] = '\0';
        }
      else
        {
        this->IncrementPixelOrderCount(&buffer[startIdx]);
        return;
        }
      }
    this->IncrementPixelOrderCount(&buffer[startIdx]);
  }

  //----------------------------------------------------------------------------

  void LineCompressor(char* buffer)
  {
    char* lineStartPointer = buffer;
    char* nextLineStartPointer;
    int count;
    while(lineStartPointer[0] != '\0' && lineStartPointer[1] != '\0' && lineStartPointer[2] != '\0' && lineStartPointer[3] != '\0' && lineStartPointer[4] != '\0')
      {

      // Find number of + next to each others
      count = 0;
      while(lineStartPointer[count] == '+')
        {
        ++count;
        }

      // If enough record their number $count+
      if(count > 4)
        {
        int numberToWrite = count;
        // Need to compress buffer
        int offset = 0;
        lineStartPointer[offset++] = '@';
        while(numberToWrite)
          {
          lineStartPointer[offset++] = vtkPVRenderViewForAssembly::vtkInternals::NUMBER_TABLE[numberToWrite%10];
          numberToWrite /= 10;
          }
        this->ReverseNumber(&lineStartPointer[offset - 1], offset - 2);

        lineStartPointer[offset++] = '+';
        nextLineStartPointer = lineStartPointer + count;
        lineStartPointer += offset;
        ShiftBuffer(lineStartPointer, nextLineStartPointer);
        }
      else
        {
        // Move to the next location that has at least 2 + in a row
        while(lineStartPointer[0] != '\0' && lineStartPointer[0] != '+')
          {
          lineStartPointer++;
          }
        if(lineStartPointer[0] != '\0')
          {
          lineStartPointer++;
          }
        }
      }
  }

  //----------------------------------------------------------------------------

  void ReverseNumber(char* buffer, int size)
  {
    char* left = buffer - size;
    char* right = buffer;
    char tmp;
    while(left < right)
      {
      tmp = left[0];
      left[0] = right[0];
      right[0] = tmp;
      right--;
      left++;
      }
  }

  //----------------------------------------------------------------------------

  void ShiftBuffer(char* left, char* right)
  {
    while(right[0] != '\0')
      {
      left[0] = right[0];
      left++;
      right++;
      }
    left[0] = '\0';
  }

  //----------------------------------------------------------------------------

  int GetRepresentationIdx(vtkPVDataRepresentation* r)
  {
    int index = 0;
    std::vector<vtkWeakPointer<vtkPVDataRepresentation> >::iterator iter;
    for(iter = this->CompositeRepresentations.begin(); iter != this->CompositeRepresentations.end(); ++iter)
      {
      vtkPVDataRepresentation* rep = vtkPVDataRepresentation::SafeDownCast(*iter);
      if(rep == r)
        {
        return index;
        }
      index++;
      }
    return -1;
  }

  //----------------------------------------------------------------------------

  void ResetPixelOrderCount()
  {
    this->PixelOrderCount.clear();
  }

  //----------------------------------------------------------------------------

  void IncrementPixelOrderCount(const char* order)
  {
    std::map<std::string, int>::iterator findEntry =
        this->PixelOrderCount.find(order);

    if(findEntry == this->PixelOrderCount.end())
      {
      this->PixelOrderCount[order] = 1;
      }
    else
      {
      ++this->PixelOrderCount[order];
      }
  }

  //----------------------------------------------------------------------------

  void WriteOrderMap(ostream& out)
  {
    vtkTimerLog::MarkStartEvent( "WriteOrderMapToDisk" );
    std::map<std::string, int>::iterator entry;
    for( entry = this->PixelOrderCount.begin();
         entry != this->PixelOrderCount.end();
         entry++)
      {
      if(entry == this->PixelOrderCount.begin())
        {
        out << "\n\"";
        }
      else
        {
        out << ",\n\"";
        }
      out << entry->first.c_str() << "\" : " << entry->second;
      }
    vtkTimerLog::MarkEndEvent("WriteOrderMapToDisk" );
  }

  //----------------------------------------------------------------------------
#ifdef PARAVIEW_USE_ICE_T
  vtkWeakPointer<vtkIceTSynchronizedRenderers> IceTSynchronizedRenderers;
#endif

  vtkNew<vtkJPEGWriter> JPEGWriter;
  vtkNew<vtkPNGWriter> PNGWriter;
  vtkNew<vtkTIFFWriter> TIFFWriter;
  vtkNew<vtkWindowToImageFilter> ImageGrabber;
  vtkNew<vtkImageData> ImageStack;
  // --
  vtkSmartPointer<vtkFloatArray> ArrayHolder;
  vtkNew<vtkWindowToImageFilter> ZGrabber;
  vtkWeakPointer<vtkPVRenderViewForAssembly> Owner;
  vtkWeakPointer<vtkUnsignedCharArray> RGBBuffer;
  vtkWeakPointer<vtkImageWriter> ActiveWriter;
  bool VisibilityState[255];
  std::vector<vtkWeakPointer<vtkPVDataRepresentation> > CompositeRepresentations;
  static const char* CODING_TABLE;
  static const char* NUMBER_TABLE;
  std::map<std::string, int> PixelOrderCount;
};

const char* vtkPVRenderViewForAssembly::vtkInternals::CODING_TABLE = "_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
const char* vtkPVRenderViewForAssembly::vtkInternals::NUMBER_TABLE = "0123456789";
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVRenderViewForAssembly);
//----------------------------------------------------------------------------
vtkPVRenderViewForAssembly::vtkPVRenderViewForAssembly()
{
  this->InRender = false;
  this->ClippingBounds.Reset();

  this->InsideComputeZOrdering = false;
  this->OrderingBufferSize = -1;
  this->OrderingBuffer = NULL;
  this->CompositeDirectory = NULL;
  this->InsideRGBDump = false;
  this->RepresentationToRender = -1;
  this->ActiveStack = 0;
  this->RGBStackSize = -1;
  this->ImageFormatExtension = NULL;
  this->SetImageFormatExtension("jpg");

  this->Internal = new vtkInternals(this);
}

//----------------------------------------------------------------------------
vtkPVRenderViewForAssembly::~vtkPVRenderViewForAssembly()
{
  this->SetCompositeDirectory(NULL);
  if(this->OrderingBuffer)
    {
    delete[] this->OrderingBuffer;
    this->OrderingBuffer = NULL;
    this->OrderingBufferSize = -1;
    }
  delete this->Internal;
  this->SetImageFormatExtension(NULL);
}

//----------------------------------------------------------------------------
void vtkPVRenderViewForAssembly::Initialize(unsigned int id)
{
  this->Superclass::Initialize(id);

#ifdef PARAVIEW_USE_ICE_T
  // If vtkIceTSynchronizedRenderers is used, we'll use that to access frame buffers.
  this->Internal->IceTSynchronizedRenderers =
    vtkIceTSynchronizedRenderers::SafeDownCast(
      this->SynchronizedRenderers->GetParallelSynchronizer());
#endif
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

  if(this->InsideComputeZOrdering)
    {
    vtkTimerLog::MarkStartEvent("CaptureZBuffer" );
    this->Internal->StoreVisibilityState();
    bool orientationVisibilityOrigin = (this->OrientationWidget->GetEnabled() != 0);
    bool centerOfRotationOrigin = (this->CenterAxes->GetVisibility() != 0);

    // Init tmp vars
    std::vector<vtkSmartPointer<vtkFloatArray> > zBuffers;

    // Disable all representation and start rendering each one independantly
    // to capture the Z Buffer and compute the ordering.
    int nbReps = this->Internal->GetNumberOfRepresentations();
    zBuffers.resize(nbReps+1, NULL);

    // Get Background Z buffer
    this->Internal->ClearVisibility();
    this->Superclass::Render(interactive, skip_rendering);
    if(this->SynchronizedWindows->GetLocalProcessIsDriver())
      {
      zBuffers[0] = this->Internal->CaptureZBuffer();
      }

    // Change orientation + center visibility
    this->SetCenterAxesVisibility(false);
    this->SetOrientationAxesVisibility(false);

    // Update visibility to match only the active one
    for(int i=0; i < nbReps; ++i)
      {
      this->Internal->UpdateVisibleRepresentation(i);

      // Lets render
      this->Superclass::Render(interactive, skip_rendering);

      // Grab Z-buffer
      if(this->SynchronizedWindows->GetLocalProcessIsDriver())
        {
        zBuffers[i+1] = this->Internal->CaptureZBuffer();
        }
      }
    vtkTimerLog::MarkEndEvent("CaptureZBuffer" );
    vtkTimerLog::MarkStartEvent("ComputeZOrdering" );

    // Do the ordering computation only on ROOT node
    if(this->SynchronizedWindows->GetLocalProcessIsDriver())
      {
      // Make sure the ordered String buffer is big enough
      int sizeX = this->GetRenderWindow()->GetSize()[0];
      int sizeY = this->GetRenderWindow()->GetSize()[1];
      int maxBufferSize = sizeX * sizeY * (1+nbReps);
      if(maxBufferSize > this->OrderingBufferSize)
        {
        if(this->OrderingBuffer != NULL)
          {
          delete[] this->OrderingBuffer;
          }
        this->OrderingBufferSize = maxBufferSize;
        this->OrderingBuffer = new char[this->OrderingBufferSize + 1];
        }

      // Lets encode the representation order for each pixel while inverting the Y
      int pixelIdx = -1;
      int bufferOffset = 0;
      std::set<PixelOrder> orderFinder;
      this->Internal->ResetPixelOrderCount();
      for(int lineIdx = sizeY; lineIdx; --lineIdx)
        {
        for(int colIdx = 0; colIdx < sizeX; ++colIdx)
          {
          pixelIdx = ((lineIdx-1)*sizeX) + colIdx;

          // Compute odering
          orderFinder.clear();
          for(unsigned char repIdx = 0; repIdx <= nbReps; ++repIdx)
            {
            if(zBuffers[repIdx])
              {
              orderFinder.insert(PixelOrder(repIdx, zBuffers[repIdx]->GetValue(pixelIdx)));
              }
            }

          // Write ordering to buffer
          this->Internal->Encode(this->OrderingBuffer, bufferOffset, orderFinder);
          }
        }
      }

    // Reset to previous state representation visibility
    this->Internal->RestoreVisibilityState();
    this->SetCenterAxesVisibility(centerOfRotationOrigin);
    this->SetOrientationAxesVisibility(orientationVisibilityOrigin);

    vtkTimerLog::MarkEndEvent("ComputeZOrdering" );
    }
  else if(this->InsideRGBDump)
    {
    vtkTimerLog::MarkStartEvent("CaptureRGB" );
    this->Internal->StoreVisibilityState();
    bool orientationVisibilityOrigin = (this->OrientationWidget->GetEnabled() != 0);
    bool centerOfRotationOrigin = (this->CenterAxes->GetVisibility() != 0);

    // Capture Background
    this->Internal->ClearVisibility();

    this->Superclass::Render(interactive, skip_rendering);
    if(this->SynchronizedWindows->GetLocalProcessIsDriver() && this->ActiveStack == 0)
      {
      this->Internal->CaptureImage(0);
      this->ActiveStack++;
      }

    // Change orientation + center visibility
    this->SetCenterAxesVisibility(false);
    this->SetOrientationAxesVisibility(false);

    // Disable all representation and start rendering each one independantly
    // to capture the RGB Buffer and save a JPEG file.
    int nbReps = this->Internal->GetNumberOfRepresentations();
    if(this->RepresentationToRender == -1)
      {
      for(int idx = 0; idx < nbReps; ++idx)
        {
        this->Internal->UpdateVisibleRepresentation(idx);
        this->Superclass::Render(interactive, skip_rendering);
        if(this->SynchronizedWindows->GetLocalProcessIsDriver())
          {
          this->Internal->CaptureImage(this->ActiveStack++);
          }
        }
      }
    else
      {
      this->Internal->UpdateVisibleRepresentation(this->RepresentationToRender);
      this->Superclass::Render(interactive, skip_rendering);
      if(this->SynchronizedWindows->GetLocalProcessIsDriver())
        {
        this->Internal->CaptureImage(this->ActiveStack++);
        }
      }

    // Reset to previous state representation visibility
    this->Internal->RestoreVisibilityState();
    this->SetCenterAxesVisibility(centerOfRotationOrigin);
    this->SetOrientationAxesVisibility(orientationVisibilityOrigin);
    vtkTimerLog::MarkEndEvent("CaptureRGB" );
    }
  else
    {
    this->Superclass::Render(interactive, skip_rendering);
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

//----------------------------------------------------------------------------
void vtkPVRenderViewForAssembly::ComputeZOrdering()
{
  this->InsideComputeZOrdering = true;
  this->Render(false, false);
  this->InsideComputeZOrdering = false;
}

//----------------------------------------------------------------------------
const char* vtkPVRenderViewForAssembly::GetZOrdering()
{
  return this->OrderingBuffer;
}

//----------------------------------------------------------------------------
const char* vtkPVRenderViewForAssembly::GetRepresentationCodes()
{
  return vtkPVRenderViewForAssembly::vtkInternals::CODING_TABLE;
}

//----------------------------------------------------------------------------
void vtkPVRenderViewForAssembly::WriteImage()
{
  if(this->CompositeDirectory == NULL || !this->SynchronizedWindows->GetLocalProcessIsDriver())
    {
    return;
    }
  this->Internal->WriteImage();
}

//----------------------------------------------------------------------------
void vtkPVRenderViewForAssembly::WriteComposite()
{
  if(this->CompositeDirectory == NULL || !this->SynchronizedWindows->GetLocalProcessIsDriver())
    {
    return;
    }

  // Write order as info.json
  vtkTimerLog::MarkStartEvent("WriteJSONData");
  std::stringstream jsonFileName;
  jsonFileName << this->CompositeDirectory << "/composite.json";
  ofstream file(jsonFileName.str().c_str(), ios::out);
  if (file.fail())
    {
    vtkErrorMacro("RecursiveWrite: Could not open file " <<
                  jsonFileName.str().c_str());
    return;
    }

  int xSize = this->GetRenderWindow()->GetSize()[0];
  int ySize = this->GetRenderWindow()->GetSize()[1];
  file << "{"
       << "\n\"dimensions\": [" << xSize << ", " << ySize << "]"
       << ",\n\"pixel-order\": \"";

  vtkTimerLog::MarkEndEvent("WriteJSONData");
  vtkTimerLog::MarkStartEvent("CompressCompositeJSON");
  this->Internal->LineCompressor(this->OrderingBuffer);
  vtkTimerLog::MarkEndEvent("CompressCompositeJSON");
  vtkTimerLog::MarkStartEvent("WriteJSONData");
  file << this->GetZOrdering();

  // Close file
  file << "\"\n}" << endl;
  file.close();
  vtkTimerLog::MarkEndEvent("WriteJSONData");


  // Write collapsed order for image query
  vtkTimerLog::MarkStartEvent("WriteJSONData");
  std::stringstream queryFileName;
  queryFileName << this->CompositeDirectory << "/query.json";
  ofstream queryFile(queryFileName.str().c_str(), ios::out);
  if (queryFile.fail())
    {
    vtkErrorMacro("RecursiveWrite: Could not open file " <<
                  queryFileName.str().c_str());
    return;
    }

  queryFile << "{"
       << "\n\"dimensions\": [" << xSize << ", " << ySize << "]"
       << ",\n\"counts\": {";

  // Write order counts
  this->Internal->WriteOrderMap(queryFile);

  // Close file
  queryFile << "\n}\n}" << endl;
  queryFile.close();
  vtkTimerLog::MarkEndEvent("WriteJSONData");
}

//----------------------------------------------------------------------------
void vtkPVRenderViewForAssembly::AddRepresentationForComposite(vtkPVDataRepresentation* r)
{
  this->AddRepresentation(r);
  this->Internal->AddRepresentation(r);
}

//----------------------------------------------------------------------------
void vtkPVRenderViewForAssembly::RemoveRepresentationForComposite(vtkPVDataRepresentation* r)
{
  this->RemoveRepresentation(r);
  this->Internal->RemoveRepresentation(r);
}

//----------------------------------------------------------------------------
void vtkPVRenderViewForAssembly::SetActiveRepresentationForComposite(vtkPVDataRepresentation* r)
{
  if(r == NULL)
    {
    this->RepresentationToRender = -1;
    }
  else
    {
    this->RepresentationToRender = this->Internal->GetRepresentationIdx(r);
    }
}
//----------------------------------------------------------------------------
void vtkPVRenderViewForAssembly::ResetActiveImageStack()
{
  this->ActiveStack = 0;
}

//----------------------------------------------------------------------------
void vtkPVRenderViewForAssembly::CaptureActiveRepresentation()
{
  if(this->CompositeDirectory == NULL)
    {
    return;
    }

  this->InsideRGBDump = true;
  this->Render(false, false);
  this->InsideRGBDump = false;
}
