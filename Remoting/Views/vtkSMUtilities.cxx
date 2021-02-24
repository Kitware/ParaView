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
#include "vtkClientServerStreamInstantiator.h"
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
#include "vtkTIFFWriter.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"
#include "vtkTuple.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkSMUtilities);

//----------------------------------------------------------------------------
int vtkSMUtilities::SaveImage(vtkImageData* image, const char* filename, int quality /*=-1*/)
{
  vtkTimerLog::MarkStartEvent("vtkSMUtilities::SaveImage");
  if (!filename || !filename[0])
  {
    vtkTimerLog::MarkEndEvent("vtkSMUtilities::SaveImage");
    return vtkErrorCode::NoFileNameError;
  }

  std::string ext = vtksys::SystemTools::GetFilenameLastExtension(filename);
  ext = vtksys::SystemTools::LowerCase(ext);

  vtkImageWriter* writer = nullptr;
  if (ext == ".bmp")
  {
    writer = vtkBMPWriter::New();
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
    if (quality >= 0 && quality <= 100)
    {
      int compression = (quality * 9) / 100;
      static_cast<vtkPNGWriter*>(writer)->SetCompressionLevel(compression);
    }
  }
  else if (ext == ".jpg" || ext == ".jpeg")
  {
    vtkJPEGWriter* jpegWriter = vtkJPEGWriter::New();
    if (quality >= 0 && quality <= 100)
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
int vtkSMUtilities::SaveImage(vtkImageData* image, const char* filename, const char* writerName)
{
  vtkTimerLog::MarkStartEvent("vtkSMUtilities::SaveImage");

  if (!filename || !writerName)
  {
    vtkTimerLog::MarkEndEvent("vtkSMUtilities::SaveImage");
    return vtkErrorCode::UnknownError;
  }

  auto object = vtkClientServerStreamInstantiator::CreateInstance(writerName);
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
    vtkGenericWarningMacro("Object is not a vtkImageWriter: " << object->GetClassName());
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
int vtkSMUtilities::SaveImageOnProcessZero(
  vtkImageData* image, const char* filename, const char* writerName)
{
  vtkTimerLog::MarkStartEvent("vtkSMUtilities::SaveImageOnProcessZero");
  int error_code;
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();

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
vtkPoints* vtkSMUtilities::CreateOrbit(
  const double center[3], const double in_normal[3], double radius, int resolution)
{
  double normal[3] = { in_normal[0], in_normal[1], in_normal[2] };
  vtkMath::Normalize(normal);
  double x_axis[3] = { 1, 0, 0 };
  double y_axis[3] = { 1, 0, 0 };
  double startPoint[3];

  // Is X not colinear to the normal ?
  if (vtkMath::Dot(x_axis, normal) < 0.999)
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
  for (int i = 0; i < 3; i++)
  {
    // Scale start point to have a given radius
    startPoint[i] *= radius;
    // Translate regarding the center
    startPoint[i] += center[i];
  }

  return CreateOrbit(center, normal, resolution, startPoint);
}

//----------------------------------------------------------------------------
vtkPoints* vtkSMUtilities::CreateOrbit(
  const double center[3], const double in_normal[3], int resolution, const double startPoint[3])
{
  // Create the step rotation
  double normal[3] = { in_normal[0], in_normal[1], in_normal[2] };
  vtkMath::Normalize(normal);
  vtkTransform* transform = vtkTransform::New();
  transform->Identity();
  transform->RotateWXYZ(360 / resolution, normal);

  // Setup initial point location
  double point[3];
  point[0] = startPoint[0] - center[0];
  point[1] = startPoint[1] - center[1];
  point[2] = startPoint[2] - center[2];

  // Fill the result
  vtkPoints* pts = vtkPoints::New(VTK_DOUBLE);
  pts->SetNumberOfPoints(resolution);
  for (int i = 0; i < resolution; i++)
  {
    pts->SetPoint(i, point[0] + center[0], point[1] + center[1], point[2] + center[2]);
    transform->TransformPoint(point, point);
  }
  transform->Delete();

  return pts;
}

//-----------------------------------------------------------------------------
void vtkSMUtilities::Merge(
  vtkImageData* dest, vtkImageData* src, int borderWidth, const unsigned char* borderColorRGB)
{
  if (!src || !dest)
  {
    return;
  }

  assert(dest->GetScalarType() == VTK_UNSIGNED_CHAR);
  assert(src->GetScalarType() == VTK_UNSIGNED_CHAR);

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
      memcpy(spanOut, spanIn, (minO < minI) ? minO : minI);
    }
    inIt.NextSpan();
    outIt.NextSpan();
  }

  if (borderWidth < 1 || borderColorRGB == nullptr)
  {
    return;
  }

  // overlay the border.
  int oddBorderWidth = static_cast<int>(std::floor(borderWidth / 2.0));
  int evenBorderWidth = static_cast<int>(std::ceil(borderWidth / 2.0));
  bool draw_border[4] = { (outextent[0] != dest->GetExtent()[0]) && (evenBorderWidth > 0),
    (outextent[1] != dest->GetExtent()[1]) && (oddBorderWidth > 0),
    (outextent[2] != dest->GetExtent()[2]) && (evenBorderWidth > 0),
    (outextent[3] != dest->GetExtent()[3]) && (oddBorderWidth > 0) };

  for (int cc = 0; cc < 4; cc++)
  {
    if (draw_border[cc] == false)
    {
      // this is an outer edge. No need to put a border.
      continue;
    }

    int border_extent[6];
    memcpy(border_extent, outextent, 4 * sizeof(int));
    border_extent[4] = border_extent[5] = 0;

    if ((cc % 2) == 0)
    {
      // even == start
      border_extent[cc + 1] = border_extent[cc] + evenBorderWidth;
    }
    else
    {
      // odd == end
      border_extent[cc - 1] = border_extent[cc] - oddBorderWidth;
    }
    vtkSMUtilities::FillImage(dest, border_extent, borderColorRGB);
  }
}

//----------------------------------------------------------------------------
void vtkSMUtilities::FillImage(vtkImageData* image, const int extent[6], const unsigned char rgb[3])
{
  assert(image->GetScalarType() == VTK_UNSIGNED_CHAR);
  unsigned char rgba[4] = { rgb[0], rgb[1], rgb[2], 0xFF };
  vtkImageIterator<unsigned char> iter(image, const_cast<int*>(extent));
  int num_comps = image->GetNumberOfScalarComponents();
  int comps_to_fill = std::min(4, num_comps);
  while (!iter.IsAtEnd())
  {
    unsigned char* start = iter.BeginSpan();
    unsigned char* end = iter.EndSpan();
    for (; start < end; start += num_comps)
    {
      memcpy(start, rgba, sizeof(unsigned char) * comps_to_fill);
    }
    iter.NextSpan();
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkImageData> vtkSMUtilities::MergeImages(
  const std::vector<vtkSmartPointer<vtkImageData> >& images, int borderWidth,
  const unsigned char* borderColorRGB)
{
  if (images.size() == 0)
  {
    return nullptr;
  }
  if (images.size() == 1)
  {
    return images[0];
  }

  int extent[6] = { VTK_INT_MAX, VTK_INT_MIN, VTK_INT_MAX, VTK_INT_MIN, VTK_INT_MAX, VTK_INT_MIN };
  int numComps = -1;
  for (std::vector<vtkSmartPointer<vtkImageData> >::const_iterator iter = images.begin();
       iter != images.end(); ++iter)
  {
    if (vtkImageData* image = iter->GetPointer())
    {
      const int* image_extent = image->GetExtent();
      extent[0] = std::min(extent[0], image_extent[0]);
      extent[2] = std::min(extent[2], image_extent[2]);
      extent[4] = std::min(extent[4], image_extent[4]);
      extent[1] = std::max(extent[1], image_extent[1]);
      extent[3] = std::max(extent[3], image_extent[3]);
      extent[5] = std::max(extent[5], image_extent[5]);

      // all images should have same number of components.
      assert(numComps == -1 || numComps == image->GetNumberOfScalarComponents());
      assert(image->GetScalarType() == VTK_UNSIGNED_CHAR);
      numComps = image->GetNumberOfScalarComponents();
    }
  }
  if (numComps != 3 && numComps != 4)
  {
    vtkGenericWarningMacro(
      "Invalid images specified. Cannot merge. Expecting 3/4 component images.");
    return vtkSmartPointer<vtkImageData>();
  }

  vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
  image->SetExtent(extent);
  image->AllocateScalars(VTK_UNSIGNED_CHAR, numComps);

  vtkTuple<unsigned char, 4> rgba(static_cast<unsigned char>(0));
  if (borderColorRGB)
  {
    memcpy(rgba.GetData(), borderColorRGB, 3 * sizeof(unsigned char));
  }
  rgba[3] = 0xff;

  if (numComps == 3)
  {
    vtkTuple<unsigned char, 3>* image_ptr =
      reinterpret_cast<vtkTuple<unsigned char, 3>*>(image->GetScalarPointer());
    std::fill(image_ptr, image_ptr + image->GetNumberOfPoints(),
      vtkTuple<unsigned char, 3>(rgba.GetData()));
  }
  else if (numComps == 4)
  {
    vtkTuple<unsigned char, 4>* image_ptr =
      reinterpret_cast<vtkTuple<unsigned char, 4>*>(image->GetScalarPointer());
    std::fill(image_ptr, image_ptr + image->GetNumberOfPoints(), rgba);
  }
  for (std::vector<vtkSmartPointer<vtkImageData> >::const_iterator iter = images.begin();
       iter != images.end(); ++iter)
  {
    vtkSMUtilities::Merge(image, iter->GetPointer(), borderWidth, borderColorRGB);
  }
  return image;
}

//----------------------------------------------------------------------------
void vtkSMUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
