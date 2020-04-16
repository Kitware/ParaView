/*=========================================================================

  Program:   ParaView
  Module:    vtkNetworkImageSource.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkNetworkImageSource.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#if VTK_MODULE_ENABLE_VTK_IOImage
#include "vtkBMPReader.h"
#include "vtkHDRReader.h"
#include "vtkJPEGReader.h"
#include "vtkPNGReader.h"
#include "vtkPNMReader.h"
#include "vtkTIFFReader.h"
#endif

#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkNetworkImageSource);
//----------------------------------------------------------------------------
vtkNetworkImageSource::vtkNetworkImageSource()
{
  this->SetNumberOfInputPorts(0);
  this->Buffer = vtkImageData::New();
  this->FileName = 0;
}

//----------------------------------------------------------------------------
vtkNetworkImageSource::~vtkNetworkImageSource()
{
  this->SetFileName(0);
  this->Buffer->Delete();
  this->Buffer = NULL;
}

//----------------------------------------------------------------------------
void vtkNetworkImageSource::UpdateImage()
{
  if (this->GetMTime() < this->UpdateImageTime)
  {
    return;
  }

  if (this->FileName == NULL || this->FileName[0] == 0)
  {
    return;
  }

  vtkPVSession* session =
    vtkPVSession::SafeDownCast(vtkProcessModule::GetProcessModule()->GetActiveSession());
  if (!session)
  {
    vtkErrorMacro("Active session must be a vtkPVSession.");
    return;
  }

  vtkPVSession::ServerFlags roles = session->GetProcessRoles();
  if ((roles & vtkPVSession::CLIENT) != 0)
  {
    // We are expected to read the image on this process.
    this->ReadImageFromFile(this->FileName);
    vtkMultiProcessController* rs_controller = session->GetController(vtkPVSession::RENDER_SERVER);
    if (rs_controller)
    {
      rs_controller->Send(this->Buffer, 1, 0x287823);
    }
  }
  else if ((roles & vtkPVSession::RENDER_SERVER) != 0 ||
    (roles & vtkPVSession::RENDER_SERVER_ROOT) != 0)
  {
    // receive the image from the client.
    vtkMultiProcessController* client_controller = session->GetController(vtkPVSession::CLIENT);
    if (client_controller)
    {
      client_controller->Receive(this->Buffer, 1, 0x287823);
    }
  }

  vtkMultiProcessController* globalController = vtkMultiProcessController::GetGlobalController();
  if (globalController->GetNumberOfProcesses() > 1)
  {
    globalController->Broadcast(this->Buffer, 0);
  }

  this->UpdateImageTime.Modified();
}

//----------------------------------------------------------------------------
int vtkNetworkImageSource::ReadImageFromFile(const char* filename)
{
  if (!filename || !filename[0])
  {
    vtkErrorMacro("FileName must be set.");
    return 0;
  }

#if VTK_MODULE_ENABLE_VTK_IOImage
  vtkSmartPointer<vtkImageReader2> reader;
  // determine type of reader to create.
  std::string ext =
    vtksys::SystemTools::LowerCase(vtksys::SystemTools::GetFilenameLastExtension(filename));
  if (ext == ".bmp")
  {
    reader.TakeReference(vtkBMPReader::New());
  }
  else if (ext == ".jpg")
  {
    reader.TakeReference(vtkJPEGReader::New());
  }
  else if (ext == ".png")
  {
    reader.TakeReference(vtkPNGReader::New());
  }
  else if (ext == ".ppm")
  {
    reader.TakeReference(vtkPNMReader::New());
  }
  else if (ext == ".tif")
  {
    reader.TakeReference(vtkTIFFReader::New());
  }
  else if (ext == ".hdr")
  {
    reader.TakeReference(vtkHDRReader::New());
  }
  else
  {
    vtkErrorMacro("Unknown texture file extension: " << filename);
    return 0;
  }
  if (!reader->CanReadFile(filename))
  {
    vtkErrorMacro("Reader cannot read file " << filename);
    return 0;
  }
  reader->SetFileName(filename);
  reader->Update();
  this->Buffer->ShallowCopy(reader->GetOutput());
  return 1;
#else
  vtkErrorMacro("Image readers are not available in this build. Please "
                "enable `VTK::IOImage` module.");
  return 0;
#endif
}

//----------------------------------------------------------------------------
int vtkNetworkImageSource::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->Buffer->GetExtent(), 6);

  return 1;
}

//----------------------------------------------------------------------------
int vtkNetworkImageSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // shallow copy internal buffer to output
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkImageData* output = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  output->ShallowCopy(this->Buffer);
  return 1;
}

//----------------------------------------------------------------------------
void vtkNetworkImageSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
