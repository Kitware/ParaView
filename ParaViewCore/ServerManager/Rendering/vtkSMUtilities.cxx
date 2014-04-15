/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUtilities.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUtilities.h"

#include "vtkBMPWriter.h"
#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkImageIterator.h"
#include "vtkJPEGWriter.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPNGWriter.h"
#include "vtkPNMWriter.h"
#include "vtkPoints.h"
#include "vtkPVInstantiator.h"
#include "vtkTIFFWriter.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"

#include <vtksys/SystemTools.hxx>
#include <sstream>
#include <string>

vtkStandardNewMacro(vtkSMUtilities);

//----------------------------------------------------------------------------
int vtkSMUtilities::SaveImage(vtkImageData* image, const char* filename,
  int quality /*=-1*/)
{
  vtkTimerLog::MarkStartEvent("vtkSMUtilities::SaveImage");
  if (!filename || !filename[0])
    {
    vtkTimerLog::MarkEndEvent("vtkSMUtilities::SaveImage");
    return vtkErrorCode::NoFileNameError;
    }

  std::string ext = vtksys::SystemTools::GetFilenameLastExtension(filename);
  ext = vtksys::SystemTools::LowerCase(ext);

  vtkImageWriter* writer = 0;
  if (ext == ".bmp")
    {
    writer= vtkBMPWriter::New();
    }
  else if (ext == ".tif" || ext == ".tiff")
    {
    writer = vtkTIFFWriter::New();
    }
  else if (ext == ".ppm")
    {
    writer = vtkPNMWriter::New();
    }
  else if (ext == ".png")
    {
    writer = vtkPNGWriter::New();
    }
  else if (ext == ".jpg" || ext == ".jpeg")
    {
    vtkJPEGWriter* jpegWriter = vtkJPEGWriter::New();
    if (quality >=0 && quality <= 100)
      {
      jpegWriter->SetQuality(quality);
      }
    writer = jpegWriter;
    }
  else
    {
    vtkTimerLog::MarkEndEvent("vtkSMUtilities::SaveImage");
    return vtkErrorCode::UnrecognizedFileTypeError;
    }

  writer->SetInputData(image);
  writer->SetFileName(filename);
  writer->Write();
  int error_code = writer->GetErrorCode();

  writer->Delete();
  vtkTimerLog::MarkEndEvent("vtkSMUtilities::SaveImage");
  return error_code;
}

//----------------------------------------------------------------------------
int vtkSMUtilities::
SaveImage(vtkImageData* image, const char* filename, const char* writerName)
{
  vtkTimerLog::MarkStartEvent("vtkSMUtilities::SaveImage");

  if (!filename || !writerName)
    {
    vtkTimerLog::MarkEndEvent("vtkSMUtilities::SaveImage");
    return vtkErrorCode::UnknownError;
    }

  vtkObject* object = vtkPVInstantiator::CreateInstance(writerName);
  if (!object)
    {
    vtkTimerLog::MarkEndEvent("vtkSMUtilities::SaveImage");
    vtkGenericWarningMacro("Failed to create Writer " << writerName);
    return vtkErrorCode::UnknownError;
    }
  vtkImageWriter* writer = vtkImageWriter::SafeDownCast(object);
  if (!writer)
    {
    vtkTimerLog::MarkEndEvent("vtkSMUtilities::SaveImage");
    vtkGenericWarningMacro("Object is not a vtkImageWriter: "
                                     << object->GetClassName());
    object->Delete();
    return vtkErrorCode::UnknownError;
    }

  writer->SetInputData(image);
  writer->SetFileName(filename);
  writer->Write();
  int error_code = writer->GetErrorCode();
  writer->Delete();

  vtkTimerLog::MarkEndEvent("vtkSMUtilities::SaveImage");
  return error_code;
}

//----------------------------------------------------------------------------
// This is usually called on a serial client, but if it is called in
// a parallel job (for example, while coprocessing for a solver), then
// we really only want to write out an image on process 0.
int vtkSMUtilities::SaveImageOnProcessZero(vtkImageData* image,
                   const char* filename, const char* writerName)
{
  vtkTimerLog::MarkStartEvent("vtkSMUtilities::SaveImageOnProcessZero");
  int error_code;
  vtkMultiProcessController *controller =
    vtkMultiProcessController::GetGlobalController();

  if (controller)
    {
    if (controller->GetLocalProcessId() == 0)
      {
      error_code = SaveImage(image, filename, writerName);
      }
    controller->Broadcast(&error_code, 1, 0);
    }
  else
    {
    error_code = SaveImage(image, filename, writerName);
    }

  vtkTimerLog::MarkEndEvent("vtkSMUtilities::SaveImageOnProcessZero");
  return error_code;
}

//----------------------------------------------------------------------------
vtkPoints* vtkSMUtilities::CreateOrbit(const double center[3],
  const double in_normal[3], double radius, int resolution)
{
  double normal[3] = {in_normal[0], in_normal[1], in_normal[2]};
  vtkMath::Normalize(normal);
  double x_axis[3] = {1,0,0};
  double y_axis[3] = {1,0,0};
  double startPoint[3];

  // Is X not colinear to the normal ?
  if(vtkMath::Dot(x_axis, normal) < 0.999)
    {
    // Do with X axis
    vtkMath::Cross(x_axis, normal, startPoint);
    vtkMath::Normalize(startPoint);
    }
  else
    {
    // Do with Y axis
    vtkMath::Cross(y_axis, normal, startPoint);
    vtkMath::Normalize(startPoint);
    }

  // Fix start point
  for(int i=0;i<3;i++)
    {
    // Scale start point to have a given radius
    startPoint[i] *= radius;
    // Translate regarding the center
    startPoint[i] += center[i];
    }

  return CreateOrbit(center, normal, resolution, startPoint);
}

//----------------------------------------------------------------------------
vtkPoints* vtkSMUtilities::CreateOrbit(const double center[3],
  const double in_normal[3], int resolution, const double startPoint[3])
{
  // Create the step rotation
  double normal[3] = {in_normal[0], in_normal[1], in_normal[2]};
  vtkMath::Normalize(normal);
  vtkTransform * transform = vtkTransform::New();
  transform->Identity();
  transform->RotateWXYZ(360/resolution, normal);

  // Setup initial point location
  double point[3];
  point[0] = startPoint[0] - center[0];
  point[1] = startPoint[1] - center[1];
  point[2] = startPoint[2] - center[2];

  // Fill the result
  vtkPoints* pts = vtkPoints::New(VTK_DOUBLE);
  pts->SetNumberOfPoints(resolution);
  for (int i=0; i < resolution; i++)
    {
    pts->SetPoint( i,
                   point[0] + center[0],
                   point[1] + center[1],
                   point[2] + center[2]);
    transform->TransformPoint(point, point);
    }
  transform->Delete();

  return pts;
}

//-----------------------------------------------------------------------------
void vtkSMUtilities::Merge(vtkImageData* dest, vtkImageData* src)
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
  outextent[0] = std::max(outextent[0], dest->GetExtent()[0]);
  outextent[1] = std::min(outextent[1], dest->GetExtent()[1]);
  outextent[2] = std::max(outextent[2], dest->GetExtent()[2]);
  outextent[3] = std::min(outextent[3], dest->GetExtent()[3]);
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

//----------------------------------------------------------------------------
void vtkSMUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
