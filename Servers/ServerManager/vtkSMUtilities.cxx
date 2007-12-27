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

#include "vtkObjectFactory.h"
#include "vtkInstantiator.h"
#include "vtkImageWriter.h"
#include "vtkImageData.h"
#include "vtkErrorCode.h"

#include <vtksys/SystemTools.hxx>
#include <vtkstd/string>

vtkStandardNewMacro(vtkSMUtilities);
vtkCxxRevisionMacro(vtkSMUtilities, "1.1");

//----------------------------------------------------------------------------
int vtkSMUtilities::SaveImage(vtkImageData* image, const char* filename)
{
  if (!filename || !filename[0])
    {
    return vtkErrorCode::NoFileNameError;
    }

  vtkstd::string ext = vtksys::SystemTools::GetFilenameLastExtension(filename);
  ext = vtksys::SystemTools::LowerCase(ext);

  const char* writername = 0;
  if(ext == ".bmp")
    {
    writername = "vtkBMPWriter";
    }
  else if(ext == ".tif" || ext == ".tiff")
    {
    writername = "vtkTIFFWriter"; 
    }
  else if(ext == ".ppm")
    {
    writername = "vtkPNMWriter";
    }
  else if(ext == ".png")
    {
    writername = "vtkPNGWriter";
    }
  else if(ext == ".jpg" || ext == ".jpeg")
    {
    writername = "vtkJPEGWriter";
    }
  else 
    {
    return vtkErrorCode::UnrecognizedFileTypeError;
    }

  vtkObject* object = vtkInstantiator::CreateInstance(writername);
  if (!object)
    {
    return vtkErrorCode::UnknownError;
    }
  vtkImageWriter* writer = vtkImageWriter::SafeDownCast(object);
  if (!writer)
    {
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
void vtkSMUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


