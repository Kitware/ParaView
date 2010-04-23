/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClientServerRenderManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVClientServerRenderManager.h"

#include "vtkObjectFactory.h"
#include "vtkServerConnection.h"
#include "vtkWeakPointer.h"
#include "vtkSocketController.h"
#include "vtkProcessModule.h"
#include "vtkImageCompressor.h"
#include "vtkSquirtCompressor.h"
#include "vtkZlibImageCompressor.h"
#include "vtkUnsignedCharArray.h"

#include <vtkstd/vector>
#include <vtksys/ios/sstream>


class vtkPVClientServerRenderManager::vtkInternal
{
public:
  typedef vtkstd::vector<vtkWeakPointer<vtkRemoteConnection> >
    ConnectionsType;
  ConnectionsType Connections;
  unsigned int Find(vtkRemoteConnection* conn)
    {
    unsigned int index=0;
    ConnectionsType::iterator iter;
    for (iter = this->Connections.begin(); iter != this->Connections.end();
      ++iter, ++index)
      {
      if (iter->GetPointer() == conn)
        {
        return index;
        }
      }
    return VTK_UNSIGNED_INT_MAX;
    }
};

static void RenderRMI(void *arg, void *, int, int)
{
  vtkPVClientServerRenderManager *self = 
    reinterpret_cast<vtkPVClientServerRenderManager*>(arg);
  self->RenderRMI();
}


//----------------------------------------------------------------------------
vtkPVClientServerRenderManager::vtkPVClientServerRenderManager()
{
  this->Internal = new vtkPVClientServerRenderManager::vtkInternal();

  // Compressor related.
  this->Compressor=0;
  this->ConfigureCompressor("vtkSquirtCompressor 0 3");
  this->LossLessCompression=1;
  this->CompressionEnabled=1;
  this->CompressorBuffer = vtkUnsignedCharArray::New();
}

//----------------------------------------------------------------------------
vtkPVClientServerRenderManager::~vtkPVClientServerRenderManager()
{
  delete this->Internal;
  this->Internal = 0;

  // compressor related
  this->CompressorBuffer->Delete();
  this->SetCompressor(0);
}

//----------------------------------------------------------------------------
void vtkPVClientServerRenderManager::Initialize(vtkRemoteConnection* conn)
{
  if (!conn || this->Internal->Find(conn) != VTK_UNSIGNED_INT_MAX)
    {
    // Already initialized
    return;
    }

  vtkSocketController* soc = conn->GetSocketController();

  vtkServerConnection* sconn = vtkServerConnection::SafeDownCast(conn);
  if (sconn && sconn->GetRenderServerSocketController())
    {
    soc = sconn->GetRenderServerSocketController();
    }

  // Register for RenderRMI. As far as ParaView's concerned, we only need the
  // RenderRMI, hence I am manually setting that up.

  soc->AddRMI(::RenderRMI, this, vtkParallelRenderManager::RENDER_RMI_TAG);
}


//----------------------------------------------------------------------------
void vtkPVClientServerRenderManager::InitializeRMIs()
{
  //vtkWarningMacro(
  //  "Please use Initialize(vtkRemoteConnection*) instead.");
  this->Superclass::InitializeRMIs();
}

//----------------------------------------------------------------------------
void vtkPVClientServerRenderManager::SetController(
  vtkMultiProcessController* controller)
{
  if (controller && (controller->GetNumberOfProcesses() != 2))
    {
    vtkErrorMacro("Client-Server needs controller with exactly 2 processes.");
    return;
    }
  // vtkWarningMacro(
  //   "Please use Initialize(vtkRemoteConnection*) instead.");
  this->Superclass::SetController(controller);
}

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkPVClientServerRenderManager,Compressor,vtkImageCompressor);


//-----------------------------------------------------------------------------
void vtkPVClientServerRenderManager::ConfigureCompressor(const char *stream)
{
  // cerr << this->GetClassName() << "::ConfigureCompressor " << stream << endl;

  // Conmfigure the compressor from a string. The string will
  // contain the class name of the compressor type to use,
  // follwed by a stream that the named class will restore itself
  // from.
  vtkstd::istringstream iss(stream);
  vtkstd::string className;
  iss >> className;

  // Allocate the desired compressor unless we have one in hand.
  if (!(this->Compressor && this->Compressor->IsA(className.c_str())))
    {
    vtkImageCompressor *comp=0;
    if (className=="vtkSquirtCompressor")
      {
      comp=vtkSquirtCompressor::New();
      }
    else
    if (className=="vtkZlibImageCompressor")
      {
      comp=vtkZlibImageCompressor::New();
      }
    else
    if (className=="NULL")
      {
      this->SetCompressor(0);
      return;
      }
    if (comp==0)
      {
      vtkWarningMacro("Could not create the compressor by name " << className << ".");
      return;
      }
    this->SetCompressor(comp);
    comp->Delete();
    }
  // move passed the class name and let the compressor configure itself
  // from the stream.
  const char *ok=this->Compressor->RestoreConfiguration(stream);
  if (!ok)
    {
    vtkWarningMacro("Could not configure the compressor, invalid stream. " << stream << ".");
    return;
    }
}

//-----------------------------------------------------------------------------
char *vtkPVClientServerRenderManager::GetCompressorConfiguration()
{
  // cerr
  //   << this->GetClassName()
  //   << "::GetCompressorConfiguration "
  //   << this->Compressor->SaveConfiguration()
  //   << endl;

  return
    const_cast<char *>(this->Compressor->SaveConfiguration());
}

//----------------------------------------------------------------------------
void vtkPVClientServerRenderManager::Activate()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkSocketController* soc = pm->GetActiveRenderServerSocketController();
  if (!soc)
    {
    abort();
    }
  this->SetController(soc);
}

//----------------------------------------------------------------------------
void vtkPVClientServerRenderManager::DeActivate()
{
  this->SetController(0);
}

//----------------------------------------------------------------------------
void vtkPVClientServerRenderManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Compressor: " << this->Compressor << endl;
  if (this->Compressor)
    {
    this->Compressor->PrintSelf(os,indent.GetNextIndent());
    }
  os << indent << "LossLessCompression: " << this->LossLessCompression << endl;
  os << indent << "CompressionEnabled: " << this->CompressionEnabled << endl;
}

// virtual void SetCompressionEnabled(int i) {
//     cerr << ":::::::::::::::::::SetCompressionEnabled " << i << endl;
//     this->CompressionEnabled=i;
//     this->Modified();
//     }
