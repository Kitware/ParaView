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
#include "vtkImageWriter.h"
#include "vtkJPEGWriter.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPNGWriter.h"
#include "vtkPVConfig.h"
#include "vtkSMAnimationScene.h"
#include "vtkTIFFWriter.h"
#include "vtkToolkits.h"

#ifdef VTK_USE_MPEG2_ENCODER
#include "vtkMPEG2Writer.h"
#endif

#include <algorithm>
#include <string>
#include <vtksys/SystemTools.hxx>

#ifdef _WIN32
#include "vtkAVIWriter.h"
#endif
#ifdef PARAVIEW_ENABLE_FFMPEG
#include "vtkFFMPEGWriter.h"
#endif

#include "vtkIOMovieConfigure.h" // for VTK_HAS_OGGTHEORA_SUPPORT
#ifdef VTK_HAS_OGGTHEORA_SUPPORT
#include "vtkOggTheoraWriter.h"
#endif

//-----------------------------------------------------------------------------
vtkSMAnimationSceneImageWriter::vtkSMAnimationSceneImageWriter()
  : Quality(100)
  , FileCount(0)
  , ErrorCode(vtkErrorCode::NoError)
  , FrameRate(1.0)
  , MovieWriterStarted(false)
{
}

//-----------------------------------------------------------------------------
vtkSMAnimationSceneImageWriter::~vtkSMAnimationSceneImageWriter()
{
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneImageWriter::SaveInitialize(int startCount)
{
  // Create writers.
  if (!this->CreateWriter())
  {
    return false;
  }

  if (this->MovieWriter)
  {
    this->MovieWriter->SetFileName(this->FileName);
  }

  // Animation scene call render on each tick. We override that render call
  // since it's a waste of rendering, the code to save the images will call
  // render anyways.
  this->AnimationScene->SetOverrideStillRender(1);

  this->FileCount = startCount;
  return true;
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneImageWriter::SaveFrame(double vtkNotUsed(time))
{
  vtkSmartPointer<vtkImageData> frame = this->CaptureFrame();
  if (!frame)
  {
    // skip empty frames.
    return true;
  }
  if (this->ImageWriter)
  {
    char number[1024];
    sprintf(number, ".%04d", this->FileCount);
    std::string filename = this->Prefix;
    filename = filename + number + this->Suffix;
    this->ImageWriter->SetInputData(frame);
    this->ImageWriter->SetFileName(filename.c_str());
    this->ImageWriter->Write();
    this->ImageWriter->SetInputData(0);
    this->ErrorCode = this->ImageWriter->GetErrorCode();
    this->FileCount =
      (this->ErrorCode == vtkErrorCode::NoError) ? this->FileCount + 1 : this->FileCount;
  }
  else if (this->MovieWriter)
  {
    this->MovieWriter->SetInputData(frame);
    if (!this->MovieWriterStarted)
    {
      this->MovieWriter->Start();
      this->MovieWriterStarted = true;
    }
    this->MovieWriter->Write();
    this->MovieWriter->SetInputData(0);

    int alg_error = this->MovieWriter->GetErrorCode();
    int movie_error = this->MovieWriter->GetError();

    if (movie_error && !alg_error)
    {
      // An error that the moviewriter caught, without setting any error code.
      // vtkGenericMovieWriter::GetStringFromErrorCode will result in
      // Unassigned Error. If this happens the Writer should be changed to set
      // a meaningful error code.

      this->ErrorCode = vtkErrorCode::UserError;
    }
    else
    {
      // if 0, then everything went well

      //< userError, means a vtkAlgorithm error (see vtkErrorCode.h)
      //= userError, means an unknown Error (Unassigned error)
      //> userError, means a vtkGenericMovieWriter error

      this->ErrorCode = alg_error;
    }
  }
  return this->ErrorCode == vtkErrorCode::NoError;
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneImageWriter::SaveFinalize()
{
  this->AnimationScene->SetOverrideStillRender(0);

  // TODO: If save failed, we must remove the partially
  // written files.
  if (this->MovieWriter && this->MovieWriterStarted)
  {
    this->MovieWriter->End();
  }

  this->MovieWriter = NULL;
  this->ImageWriter = NULL;
  return true;
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneImageWriter::CreateWriter()
{
  this->MovieWriterStarted = false;
  this->ImageWriter = NULL;
  this->MovieWriter = NULL;

  vtkSmartPointer<vtkImageWriter> iwriter;
  vtkSmartPointer<vtkGenericMovieWriter> mwriter;

  std::string extension = vtksys::SystemTools::GetFilenameLastExtension(this->FileName);
  if (extension == ".jpg" || extension == ".jpeg")
  {
    iwriter = vtkSmartPointer<vtkJPEGWriter>::New();
  }
  else if (extension == ".tif" || extension == ".tiff")
  {
    iwriter = vtkSmartPointer<vtkTIFFWriter>::New();
  }
  else if (extension == ".png")
  {
    int pngQuality = (9 * (100 - this->Quality)) / 100;
    vtkNew<vtkPNGWriter> pngwriter;
    pngwriter->SetCompressionLevel(pngQuality);

    iwriter = pngwriter.Get();
  }
#ifdef VTK_USE_MPEG2_ENCODER
  else if (extension == ".mpeg" || extension == ".mpg")
  {
    mwriter = vtkSmartPointer<vtkMPEG2Writer>::New();
  }
#endif
#ifdef PARAVIEW_ENABLE_FFMPEG
  else if (extension == ".avi")
  {
    vtkNew<vtkFFMPEGWriter> aviwriter;
    double quality = 3 * this->Quality / 100.0;
    if (quality > 2)
    {
      aviwriter->SetCompression(0);
    }
    else
    {
      aviwriter->SetCompression(1);
      aviwriter->SetQuality(static_cast<int>(quality));
    }
    aviwriter->SetRate(static_cast<int>(this->GetFrameRate()));
    mwriter = aviwriter.Get();
  }
#endif
#ifdef _WIN32
  else if (extension == ".avi")
  {
    vtkNew<vtkAVIWriter> avi;
    avi->SetQuality((2 * this->Quality) / 100);
    avi->SetRate(static_cast<int>(this->GetFrameRate()));
    // Also available are IYUV and I420, but these are ~10x larger than MSVC.
    // No other encoder seems to be available on a stock Windows 7 install.
    avi->SetCompressorFourCC("MSVC");
    mwriter = avi.Get();
  }
#else
#endif
#ifdef VTK_HAS_OGGTHEORA_SUPPORT
  else if (extension == ".ogv" || extension == ".ogg")
  {
    vtkNew<vtkOggTheoraWriter> ogvwriter;
    ogvwriter->SetQuality((2 * this->Quality) / 100);
    ogvwriter->SetRate(static_cast<int>(this->GetFrameRate()));
    mwriter = ogvwriter.Get();
  }
#endif
  else
  {
    vtkErrorMacro("Unknown extension " << extension);
    return false;
  }

  if (iwriter)
  {
    this->ImageWriter = iwriter;

    std::string filename = this->FileName;
    std::string::size_type dot_pos = filename.rfind(".");
    if (dot_pos != std::string::npos)
    {
      this->Prefix = filename.substr(0, dot_pos);
      this->Suffix = filename.substr(dot_pos);
    }
    else
    {
      this->Prefix = this->FileName;
      this->Suffix = "";
    }
  }
  if (mwriter)
  {
    this->MovieWriter = mwriter;
  }

  return (this->ImageWriter != NULL || this->MovieWriter != NULL);
}

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneImageWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Quality: " << this->Quality << endl;
  os << indent << "ErrorCode: " << this->ErrorCode << endl;
  os << indent << "FrameRate: " << this->FrameRate << endl;
}
