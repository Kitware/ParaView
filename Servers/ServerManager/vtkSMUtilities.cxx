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
#include "vtkObjectFactory.h"
#include "vtkPNGWriter.h"
#include "vtkPNMWriter.h"
#include "vtkTIFFWriter.h"

#include <vtksys/SystemTools.hxx>
#include <vtkstd/string>

vtkStandardNewMacro(vtkSMUtilities);
vtkCxxRevisionMacro(vtkSMUtilities, "1.2");

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
void vtkSMUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


