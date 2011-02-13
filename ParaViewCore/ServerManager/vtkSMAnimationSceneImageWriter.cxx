/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAnimationSceneImageWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAnimationSceneImageWriter.h"

#include "vtkErrorCode.h"
#include "vtkGenericMovieWriter.h"
#include "vtkImageData.h"
#include "vtkImageIterator.h"
#include "vtkImageWriter.h"
#include "vtkJPEGWriter.h"
#include "vtkObjectFactory.h"
#include "vtkObjectFactory.h"
#include "vtkPNGWriter.h"
#include "vtkSMAnimationScene.h"
#include "vtkSmartPointer.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkTIFFWriter.h"
#include "vtkToolkits.h"

#ifdef VTK_USE_MPEG2_ENCODER
# include "vtkMPEG2Writer.h"
#endif

#include <vtkstd/algorithm>
#include <vtkstd/string>
#include <vtksys/SystemTools.hxx>

#ifdef _WIN32
# include "vtkAVIWriter.h"
#else
# ifdef VTK_USE_FFMPEG_ENCODER
#   include "vtkFFMPEGWriter.h"
# endif
#endif

#ifdef VTK_USE_OGGTHEORA_ENCODER
#  include "vtkOggTheoraWriter.h"
#  include "vtkPVConfig.h"
#endif

vtkStandardNewMacro(vtkSMAnimationSceneImageWriter);
vtkCxxSetObjectMacro(vtkSMAnimationSceneImageWriter,
  ImageWriter, vtkImageWriter);
vtkCxxSetObjectMacro(vtkSMAnimationSceneImageWriter,
  MovieWriter, vtkGenericMovieWriter);
//-----------------------------------------------------------------------------
vtkSMAnimationSceneImageWriter::vtkSMAnimationSceneImageWriter()
{
  this->Magnification = 1;
  this->ErrorCode = 0;
  this->Quality = 2; // 0 = low, 1 = medium, 2 = high
#ifdef PARAVIEW_OGGTHEORA_USE_SUBSAMPLING
  this->Subsampling = 1;
#else
  this->Subsampling = 0;
#endif
  this->ActualSize[0] = this->ActualSize[1] = 0;

  this->MovieWriter = 0;
  this->ImageWriter = 0;
  this->FileCount = 0;

  this->Prefix = 0;
  this->Suffix = 0;
  this->FrameRate = 1.0;

  this->BackgroundColor[0] = this->BackgroundColor[1] =
    this->BackgroundColor[2] = 0.0;
}

//-----------------------------------------------------------------------------
vtkSMAnimationSceneImageWriter::~vtkSMAnimationSceneImageWriter()
{
  this->SetMovieWriter(0);
  this->SetImageWriter(0);

  this->SetPrefix(0);
  this->SetSuffix(0);
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneImageWriter::SaveInitialize()
{
  // Create writers.
  if (!this->CreateWriter())
    {
    return false;
    }

  this->UpdateImageSize();

  if (this->MovieWriter)
    {
    this->MovieWriter->SetFileName(this->FileName);
    vtkImageData* emptyImage = this->NewFrame();
    this->MovieWriter->SetInput(emptyImage);
    emptyImage->Delete();

    this->MovieWriter->Start();
    }

  this->AnimationScene->SetOverrideStillRender(1);

  this->FileCount = 0;

#if !defined(__APPLE__)
  // Iterate over all views and enable offscreen rendering. This avoid toggling
  // of the offscreen rendering flag on every frame.
  unsigned int num_modules = this->AnimationScene->GetNumberOfViewProxies();
  for (unsigned int cc=0; cc < num_modules; cc++)
    {
    vtkSMRenderViewProxy* rmview = vtkSMRenderViewProxy::SafeDownCast(
      this->AnimationScene->GetViewProxy(cc));
    if (rmview)
      {
      if (vtkSMPropertyHelper(rmview,
          "UseOffscreenRenderingForScreenshots").GetAsInt() == 1)
        {
        vtkSMPropertyHelper(rmview, "UseOffscreenRendering").Set(1);
        rmview->UpdateProperty("UseOffscreenRendering");
        }
      }
    }
#endif

  return true;
}

//-----------------------------------------------------------------------------
vtkImageData* vtkSMAnimationSceneImageWriter::NewFrame()
{
  vtkImageData* image = vtkImageData::New();
  image->SetDimensions(this->ActualSize[0], this->ActualSize[1], 1);
  image->SetScalarTypeToUnsignedChar();
  image->SetNumberOfScalarComponents(3);
  image->AllocateScalars();

  unsigned char rgb[3];
  rgb[0] = 0x0ff & static_cast<int>(this->BackgroundColor[0]*0x0ff);
  rgb[1] = 0x0ff & static_cast<int>(this->BackgroundColor[1]*0x0ff);
  rgb[2] = 0x0ff & static_cast<int>(this->BackgroundColor[2]*0x0ff);
  vtkImageIterator<unsigned char> it(image, image->GetExtent());
  while (!it.IsAtEnd())
    {
    unsigned char* span = it.BeginSpan();
    unsigned char* spanEnd = it.EndSpan();
    while (spanEnd != span)
      {
      *span = rgb[0]; span++;
      *span = rgb[1]; span++;
      *span = rgb[2]; span++;
      }
    it.NextSpan();
    }
  return image;
}

//-----------------------------------------------------------------------------
vtkImageData* vtkSMAnimationSceneImageWriter::CaptureViewImage(
  vtkSMViewProxy* view, int magnification)
{
  if (view)
    {
    return view->CaptureWindow(magnification);
    }

  return NULL;
}

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneImageWriter::Merge(vtkImageData* dest, vtkImageData* src)
{
  if (!src || !dest)
    {
    return;
    }

  vtkImageIterator<unsigned char> inIt(src, src->GetExtent());
  int outextent[6];
  src->GetExtent(outextent);

  // we need to flip Y.
  outextent[2] = dest->GetExtent()[3] - outextent[2];
  outextent[3] = dest->GetExtent()[3] - outextent[3];
  int temp = outextent[2];
  outextent[2] = outextent[3];
  outextent[3] = temp;
  // snap extents to what is available.
  outextent[0] = vtkstd::max(outextent[0], dest->GetExtent()[0]);
  outextent[1] = vtkstd::min(outextent[1], dest->GetExtent()[1]);
  outextent[2] = vtkstd::max(outextent[2], dest->GetExtent()[2]);
  outextent[3] = vtkstd::min(outextent[3], dest->GetExtent()[3]);
  vtkImageIterator<unsigned char> outIt(dest, outextent);

  while (!outIt.IsAtEnd() && !inIt.IsAtEnd())
    {
    unsigned char* spanOut = outIt.BeginSpan();
    unsigned char* spanIn = inIt.BeginSpan();
    unsigned char* outSpanEnd = outIt.EndSpan();
    unsigned char* inSpanEnd = inIt.EndSpan();
    if (outSpanEnd != spanOut && inSpanEnd != spanIn)
      {
      size_t minO = outSpanEnd - spanOut;
      size_t minI = inSpanEnd - spanIn;
      memcpy(spanOut, spanIn, (minO < minI)? minO : minI);
      }
    inIt.NextSpan();
    outIt.NextSpan();
    }
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneImageWriter::SaveFrame(double vtkNotUsed(time))
{
  vtkSmartPointer<vtkImageData> combinedImage;
  unsigned int num_modules = this->AnimationScene->GetNumberOfViewProxies();
  if (num_modules > 1)
    {
    combinedImage.TakeReference(this->NewFrame());
    for (unsigned int cc=0; cc < num_modules; cc++)
      {
      vtkSMViewProxy* view = this->AnimationScene->GetViewProxy(cc);
      vtkImageData* capture = this->CaptureViewImage(view, this->Magnification);
      if (capture)
        {
        this->Merge(combinedImage, capture);
        capture->Delete();
        }
      }
    }
  else if (num_modules == 1)
    {
    // If only one view, we speed things up slightly by using the
    // captured image directly.
    vtkSMViewProxy* view = this->AnimationScene->GetViewProxy(0);
    vtkImageData* capture = this->CaptureViewImage(view, this->Magnification);
    if (!capture)
      {
      return false;
      }
    combinedImage.TakeReference(capture);
    }

  int errcode = 0;
  if (this->ImageWriter)
    {
    char number[1024];
    sprintf(number, ".%04d", this->FileCount);
    vtkstd::string filename = this->Prefix;
    filename = filename + number + this->Suffix;
    this->ImageWriter->SetInput(combinedImage);
    this->ImageWriter->SetFileName(filename.c_str());
    this->ImageWriter->Write();
    this->ImageWriter->SetInput(0);

    errcode = this->ImageWriter->GetErrorCode();
    this->FileCount = (!errcode)? this->FileCount + 1 : this->FileCount;

    }
  else if (this->MovieWriter)
    {
    this->MovieWriter->SetInput(combinedImage);
    this->MovieWriter->Write();
    this->MovieWriter->SetInput(0);

    int alg_error = this->MovieWriter->GetErrorCode();
    int movie_error = this->MovieWriter->GetError();

    if (movie_error && !alg_error)
      {
      //An error that the moviewriter caught, without setting any error code.
      //vtkGenericMovieWriter::GetStringFromErrorCode will result in
      //Unassigned Error. If this happens the Writer should be changed to set
      //a meaningful error code.

      errcode = vtkErrorCode::UserError;
      }
    else
      {
      //if 0, then everything went well

      //< userError, means a vtkAlgorithm error (see vtkErrorCode.h)
      //= userError, means an unknown Error (Unassigned error)
      //> userError, means a vtkGenericMovieWriter error

      errcode = alg_error;
      }
    }
  combinedImage = 0;

  if (errcode)
    {
    this->ErrorCode = errcode;
    return false;
    }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneImageWriter::SaveFinalize()
{
  this->AnimationScene->SetOverrideStillRender(0);

  // TODO: If save failed, we must remove the partially
  // written files.
  if (this->MovieWriter)
    {
    this->MovieWriter->End();
    this->SetMovieWriter(0);
    }
  this->SetImageWriter(0);

#if !defined(__APPLE__)
  // restore offscreen rendering state.
  unsigned int num_modules = this->AnimationScene->GetNumberOfViewProxies();
  for (unsigned int cc=0; cc < num_modules; cc++)
    {
    vtkSMRenderViewProxy* rmview = vtkSMRenderViewProxy::SafeDownCast(
      this->AnimationScene->GetViewProxy(cc));
    if (rmview)
      {
      vtkSMPropertyHelper(rmview, "UseOffscreenRendering").Set(0);
      rmview->UpdateProperty("UseOffscreenRendering");
      }
    }
#endif
  return true;
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneImageWriter::CreateWriter()
{
  this->SetMovieWriter(0);
  this->SetImageWriter(0);

  vtkImageWriter* iwriter =0;
  vtkGenericMovieWriter* mwriter = 0;

  vtkstd::string extension = vtksys::SystemTools::GetFilenameLastExtension(
    this->FileName);
  if (extension == ".jpg" || extension == ".jpeg")
    {
    iwriter = vtkJPEGWriter::New();
    }
  else if (extension == ".tif" || extension == ".tiff")
    {
    iwriter = vtkTIFFWriter::New();
    }
  else if (extension == ".png")
    {
    iwriter = vtkPNGWriter::New();
    }
#ifdef VTK_USE_MPEG2_ENCODER
  else if (extension == ".mpeg" || extension == ".mpg")
    {
    mwriter = vtkMPEG2Writer::New();
    }
#endif
#ifdef _WIN32
  else if (extension == ".avi")
    {
    vtkAVIWriter* avi = vtkAVIWriter::New();
    avi->SetQuality(this->Quality);
    avi->SetRate(
      static_cast<int>(this->GetFrameRate()));
    mwriter = avi;
    }
#else
# ifdef VTK_USE_FFMPEG_ENCODER
  else if (extension == ".avi")
    {
    vtkFFMPEGWriter *aviwriter = vtkFFMPEGWriter::New();
    aviwriter->SetQuality(this->Quality);
    aviwriter->SetRate(
      static_cast<int>(this->GetFrameRate()));
    mwriter = aviwriter;
    }
# endif
#endif
#ifdef VTK_USE_OGGTHEORA_ENCODER
  else if (extension == ".ogv" || extension == ".ogg")
    {
    vtkOggTheoraWriter *ogvwriter = vtkOggTheoraWriter::New();
    ogvwriter->SetQuality(this->Quality);
    ogvwriter->SetRate(
      static_cast<int>(this->GetFrameRate()));
    ogvwriter->SetSubsampling(this->GetSubsampling());
    mwriter = ogvwriter;
    }
#endif
  else
    {
    vtkErrorMacro("Unknown extension " << extension.c_str());
    return false;
    }

  if (iwriter)
    {
    this->SetImageWriter(iwriter);
    iwriter->Delete();

    vtkstd::string filename = this->FileName;
    vtkstd::string::size_type dot_pos = filename.rfind(".");
    if(dot_pos != vtkstd::string::npos)
      {
      this->SetPrefix(filename.substr(0, dot_pos).c_str());
      this->SetSuffix(filename.substr(dot_pos).c_str());
      }
    else
      {
      this->SetPrefix(this->FileName);
      this->SetSuffix("");
      }
    }
  if (mwriter)
    {
    this->SetMovieWriter(mwriter);
    mwriter->Delete();
    }
  return true;
}


//-----------------------------------------------------------------------------
void vtkSMAnimationSceneImageWriter::UpdateImageSize()
{
  unsigned int num_modules = this->AnimationScene->GetNumberOfViewProxies();
  int gui_size[2] = {1, 1};
  for (unsigned int cc=0; cc < num_modules; cc++)
    {
    vtkSMViewProxy* view = this->AnimationScene->GetViewProxy(cc);
    if (!view)
      {
      continue;
      }
    vtkSMPropertyHelper size(view, "ViewSize");
    vtkSMPropertyHelper position(view, "ViewPosition");
    if (gui_size[0] < size.GetAsInt(0) + position.GetAsInt(0))
      {
      gui_size[0] = size.GetAsInt(0) + position.GetAsInt(0);
      }
    if (gui_size[1] < size.GetAsInt(1) + position.GetAsInt(1))
      {
      gui_size[1] = size.GetAsInt(1) + position.GetAsInt(1);
      }
    }
  if (num_modules==0)
    {
    vtkErrorMacro("AnimationScene has no view modules added to it.");
    }
  gui_size[0] *= this->Magnification;
  gui_size[1] *= this->Magnification;
  this->SetActualSize(gui_size);
}

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneImageWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Quality: " << this->Quality << endl;
  os << indent << "Magnification: " << this->Magnification << endl;
  os << indent << "Subsampling: " << this->Subsampling << endl;
  os << indent << "ErrorCode: " << this->ErrorCode << endl;
  os << indent << "FrameRate: " << this->FrameRate << endl;
  os << indent << "BackgroundColor: " << this->BackgroundColor[0]
    << ", " << this->BackgroundColor[1] << ", " << this->BackgroundColor[2]
    << endl;
}
