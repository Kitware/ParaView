/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBatchCompositeManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVBatchCompositeManager.h"

#include "vtkCompositeManager.h"
#include "vtkCompressCompositer.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkRenderer.h"
#include "vtkToolkits.h"
#include "vtkUnsignedCharArray.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkPointData.h"

#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#endif

#ifdef VTK_USE_MPI
 #include <mpi.h>
#endif

vtkCxxRevisionMacro(vtkPVBatchCompositeManager, "1.4");
vtkStandardNewMacro(vtkPVBatchCompositeManager);


//-------------------------------------------------------------------------
vtkPVBatchCompositeManager::vtkPVBatchCompositeManager()
{
  this->RenderWindow = NULL;
  this->Controller = vtkMultiProcessController::GetGlobalController();

  this->Compositer = vtkCompressCompositer::New();
  this->Compositer->Register(this);
  this->Compositer->Delete();

  if (this->Controller)
    {
    this->Controller->Register(this);
    this->Compositer->SetNumberOfProcesses(
                         this->Controller->GetNumberOfProcesses());
    }

  this->PData = NULL;
  this->ZData = NULL;
  this->LocalPData = NULL;
  this->LocalZData = NULL;
  this->NumberOfProcesses = 0;
  this->RendererSize[0] = 0;
  this->RendererSize[1] = 0;
}

  
//-------------------------------------------------------------------------
vtkPVBatchCompositeManager::~vtkPVBatchCompositeManager()
{
  this->SetRenderWindow(NULL);
  
  this->SetRendererSize(0,0);
  
  if (this->Controller)
    {
    this->Controller->UnRegister(this);
    this->Controller = NULL;
    }
  if (this->Compositer)
    {
    this->Compositer->UnRegister(this);
    this->Compositer = NULL;
    }


  if (this->PData)
    {
    vtkCompositeManager::DeleteArray(this->PData);
    this->PData = NULL;
    }
  
  if (this->ZData)
    {
    vtkCompositeManager::DeleteArray(this->ZData);
    this->ZData = NULL;
    }

  if (this->LocalPData)
    {
    vtkCompositeManager::DeleteArray(this->LocalPData);
    this->LocalPData = NULL;
    }
  
  if (this->LocalZData)
    {
    vtkCompositeManager::DeleteArray(this->LocalZData);
    this->LocalZData = NULL;
    }
}





//-------------------------------------------------------------------------
// Only process 0 needs start and end render callbacks.
void vtkPVBatchCompositeManager::SetRenderWindow(vtkRenderWindow *renWin)
{
  if (this->RenderWindow == renWin)
    {
    return;
    }
  this->Modified();

  if (this->RenderWindow)
    {
    // Delete the reference.
    this->RenderWindow->UnRegister(this);
    this->RenderWindow =  NULL;
    }
  if (renWin)
    {
    renWin->Register(this);
    this->RenderWindow = renWin;
    }
}


//-------------------------------------------------------------------------
void vtkPVBatchCompositeManager::SetController(vtkMultiProcessController *mpc)
{
  if (this->Controller == mpc)
    {
    return;
    }
  if (mpc)
    {
    mpc->Register(this);
    }
  if (this->Controller)
    {
    this->Controller->UnRegister(this);
    }
  this->Controller = mpc;

  if (this->Compositer)
    {
    this->Compositer->SetController(mpc);
    }
}

//-------------------------------------------------------------------------
void vtkPVBatchCompositeManager::SetCompositer(vtkCompositer *c)
{
  if (c == this->Compositer)
    {
    return;
    }
  if (c)
    {
    c->Register(this);
    c->SetController(this->Controller);
    c->SetNumberOfProcesses(this->Controller->GetNumberOfProcesses());
    }
  if (this->Compositer)
    {
    this->Compositer->UnRegister(this);
    this->Compositer = NULL;
    }
  this->Compositer = c;
}







//-------------------------------------------------------------------------
void vtkPVBatchCompositeManager::InitializePieces()
{
  vtkRendererCollection *rens;
  vtkRenderer *ren;
  vtkActorCollection *actors;
  vtkActor *actor;
  vtkMapper *mapper;
  vtkPolyDataMapper *pdMapper;
  int piece, numPieces;

  if (this->RenderWindow == NULL || this->Controller == NULL)
    {
    return;
    }
  piece = this->Controller->GetLocalProcessId();
  numPieces = this->Controller->GetNumberOfProcesses();

  rens = this->RenderWindow->GetRenderers();
  rens->InitTraversal();
  while ( (ren = rens->GetNextItem()) )
    {
    actors = ren->GetActors();
    actors->InitTraversal();
    while ( (actor = actors->GetNextItem()) )
      {
      mapper = actor->GetMapper();
      pdMapper = vtkPolyDataMapper::SafeDownCast(mapper);
      if (pdMapper)
        {
        pdMapper->SetPiece(piece);
        pdMapper->SetNumberOfPieces(numPieces);
        }
      }
    }
}



//-------------------------------------------------------------------------
void vtkPVBatchCompositeManager::SetRendererSize(int x, int y)
{
  int numComps = 3;  // RGB  
  
  if (this->RendererSize[0] == x && this->RendererSize[1] == y)
    {
    return;
    }
  
  int numPixels = x * y;
  if (numPixels > 0)
    {
    if (!this->PData)
      {
      this->PData = vtkUnsignedCharArray::New();
      }
    vtkCompositeManager::ResizeUnsignedCharArray(
      static_cast<vtkUnsignedCharArray*>(this->PData), 
      numComps, numPixels);
    if (!this->LocalPData)
      {
      this->LocalPData = vtkUnsignedCharArray::New();
      }
    vtkCompositeManager::ResizeUnsignedCharArray(
      static_cast<vtkUnsignedCharArray*>(this->LocalPData), 
      numComps, numPixels);

    if (!this->ZData)
      {
      this->ZData = vtkFloatArray::New();
      }
    vtkCompositeManager::ResizeFloatArray(this->ZData, 1, numPixels);
    if (!this->LocalZData)
      {
      this->LocalZData = vtkFloatArray::New();
      }
    vtkCompositeManager::ResizeFloatArray(this->LocalZData, 1, numPixels);
    }
  else
    {
    if (this->PData)
      {
      vtkCompositeManager::DeleteArray(this->PData);
      this->PData = NULL;
      }
    
    if (this->ZData)
      {
      vtkCompositeManager::DeleteArray(this->ZData);
      this->ZData = NULL;
      }

    if (this->LocalPData)
      {
      vtkCompositeManager::DeleteArray(this->LocalPData);
      this->LocalPData = NULL;
      }
    
    if (this->LocalZData)
      {
      vtkCompositeManager::DeleteArray(this->LocalZData);
      this->LocalZData = NULL;
      }
    }

  this->RendererSize[0] = x;
  this->RendererSize[1] = y;
}


//----------------------------------------------------------------------------
void vtkPVBatchCompositeManager::Composite()
{
  int myId;
  int front = 1;
  int* size;

  if (this->RenderWindow == NULL)
    {
    return;
    }

  this->Modified();

  size = this->RenderWindow->GetSize();
  this->SetRendererSize(size[0], size[1]);
  
  myId = this->Controller->GetLocalProcessId();

  // Get the z buffer.
  this->RenderWindow->GetZbufferData(0,0, size[0]-1, size[1]-1,
                                     this->LocalZData);  

  // Get the pixel data.
  this->RenderWindow->GetPixelData(
          0,0,this->RendererSize[0]-1,this->RendererSize[1]-1, 
          front,static_cast<vtkUnsignedCharArray*>(this->LocalPData));
    
  // Let the subclass use its owns composite algorithm to
  // collect the results into "localPData" on process 0.
  this->Compositer->CompositeBuffer(this->LocalPData, this->LocalZData,
                                    this->PData, this->ZData);
    

  // No need to set the buffer.
  // Just set the output scalars (in Execute).
  
}


//----------------------------------------------------------------------------
void vtkPVBatchCompositeManager::ExecuteInformation()
{
  if (this->RenderWindow == NULL )
    {
    vtkErrorMacro(<<"Please specify a renderer as input!");
    return;
    }
  vtkImageData *out = this->GetOutput();
  
  // set the extent
  out->SetWholeExtent(0, this->RenderWindow->GetSize()[0] - 1,
                      0, this->RenderWindow->GetSize()[1] - 1,
                      0, 0);
  
  // set the spacing
  out->SetSpacing(1.0, 1.0, 1.0);
  
  // set the origin.
  out->SetOrigin(0.0, 0.0, 0.0);
  
  // set the scalar components
  out->SetNumberOfScalarComponents(3);
  out->SetScalarType(VTK_UNSIGNED_CHAR);
}




//----------------------------------------------------------------------------
void vtkPVBatchCompositeManager::Execute()
{
  vtkImageData *out = this->GetOutput();
  int myId;

  myId = this->Controller->GetLocalProcessId();

  out->SetExtent(0, this->RendererSize[0]-1,
                 0, this->RendererSize[1]-1,
                 0, 0);

  if (myId == 0) 
    {
    vtkUnsignedCharArray* buf;
    buf = static_cast<vtkUnsignedCharArray*>(this->LocalPData);
    vtkUnsignedCharArray* scalars = vtkUnsignedCharArray::New();
    scalars->DeepCopy(buf);
    out->GetPointData()->SetScalars(scalars);
    scalars->Delete();
    scalars = NULL;
    }
  else
    {
    // I could return some buffer here ....
    }
}



//----------------------------------------------------------------------------
void vtkPVBatchCompositeManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  
  if ( this->RenderWindow )
    {
    os << indent << "RenderWindow: " << this->RenderWindow << "\n";
    }
  else
    {
    os << indent << "RenderWindow: (none)\n";
    }

  os << indent << "Controller: (" << this->Controller << ")\n"; 
  if (this->Compositer)
    {
    os << indent << "Compositer: " << this->Compositer->GetClassName() 
       << " (" << this->Compositer << ")\n"; 
    }
  else
    {
    os << indent << "Compositer: NULL\n";
    }
  os << indent << "NumberOfProcesses: " << this->NumberOfProcesses << endl;
}



