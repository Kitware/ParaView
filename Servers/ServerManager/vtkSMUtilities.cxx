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
#include "vtkInstantiator.h"
#include "vtkJPEGWriter.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPNGWriter.h"
#include "vtkPNMWriter.h"
#include "vtkPoints.h"
#include "vtkTIFFWriter.h"
#include "vtkTransform.h"

#include <vtksys/SystemTools.hxx>
#include <vtkstd/string>

vtkStandardNewMacro(vtkSMUtilities);

//----------------------------------------------------------------------------
int vtkSMUtilities::SaveImage(vtkImageData* image, const char* filename,
  int quality /*=-1*/)
{
  if (!filename || !filename[0])
    {
    return vtkErrorCode::NoFileNameError;
    }

  vtkstd::string ext = vtksys::SystemTools::GetFilenameLastExtension(filename);
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
    return vtkErrorCode::UnrecognizedFileTypeError;
    }

  writer->SetInput(image);
  writer->SetFileName(filename);
  writer->Write();
  int error_code = writer->GetErrorCode();

  writer->Delete();
  return error_code;
}

//----------------------------------------------------------------------------
int vtkSMUtilities::
SaveImage(vtkImageData* image, const char* filename, const char* writerName)
{
  if (!filename || !writerName)
    {
    return vtkErrorCode::UnknownError;
    }

  vtkObject* object = vtkInstantiator::CreateInstance(writerName);
  if (!object)
    {
    vtkGenericWarningMacro("Failed to create Writer " << writerName);
    return vtkErrorCode::UnknownError;
    }
  vtkImageWriter* writer = vtkImageWriter::SafeDownCast(object);
  if (!writer)
    {
    vtkGenericWarningMacro("Object is not a vtkImageWriter: "
                                     << object->GetClassName());
    object->Delete();
    return vtkErrorCode::UnknownError;
    }

  writer->SetInput(image);
  writer->SetFileName(filename);
  writer->Write();
  int error_code = writer->GetErrorCode();
  writer->Delete();
  return error_code;
}

//----------------------------------------------------------------------------
// This is usually called on a serial client, but if it is called in
// a parallel job (for example, while coprocessing for a solver), then
// we really only want to write out an image on process 0.
int vtkSMUtilities::SaveImageOnProcessZero(vtkImageData* image,
                   const char* filename, const char* writerName)
{
  int error_code = vtkErrorCode::NoError;
  vtkMultiProcessController *controller =
    vtkMultiProcessController::GetGlobalController();

  if (controller)
    {
    if (controller->GetLocalProcessId() == 0)
      {
      error_code = SaveImage(image, filename, writerName);
      }
    }
  else
    {
    error_code = SaveImage(image, filename, writerName);
    }

  return error_code;
}

//----------------------------------------------------------------------------
vtkPoints* vtkSMUtilities::CreateOrbit(const double center[3],
  const double in_normal[3], double radius, int resolution)
{
  vtkPoints* pts = vtkPoints::New(VTK_DOUBLE);
  pts->SetNumberOfPoints(resolution);
  vtkTransform * transform = vtkTransform::New();
  transform->Identity();


  double normal[3] = {in_normal[0], in_normal[1], in_normal[2]};
  vtkMath::Normalize(normal);
  if (normal[0] == 0.0 && normal[1] == 0.0 && normal[2] == 1.0)
    {
    // no need to do any rotations.
    }
  else
    {
    double z_axis[3] = {0,0,1};
    double rotation_axis[3];
    vtkMath::Cross(z_axis, normal, rotation_axis);
    transform->RotateWXYZ(
      acos(vtkMath::Dot(z_axis, normal))*180/3.141592,
      rotation_axis);
    }

  for (int i=0; i < resolution; i++)
    {
    double a = radius*cos(i*2*3.141592/resolution);
    double b = radius*sin(i*2*3.141592/resolution);
    double point[3];
    point[0] = a;
    point[1] = b;
    point[2] = 0.0;

    // Now this point is in the XY plane (with normal (0, 0, 1). We need to
    // rotate it to match the normal.
    transform->TransformPoint(point, point); 
    point[0] += center[0];
    point[1] += center[1];
    point[2] += center[2];
    pts->SetPoint(i, point);
    }
  transform->Delete();
  return pts;
}


//----------------------------------------------------------------------------
void vtkSMUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


