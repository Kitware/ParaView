/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPart.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPart.h"

#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkImageData.h"
#include "vtkKWCheckButton.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVClassNameInformation.h"
#include "vtkPVConfig.h"
#include "vtkPVDataInformation.h"
#include "vtkPVPartDisplay.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderModule.h"
#include "vtkPVRenderView.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRectilinearGrid.h"
#include "vtkSMPart.h"
#include "vtkString.h"
#include "vtkStructuredGrid.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPart);
vtkCxxRevisionMacro(vtkPVPart, "1.51");

vtkCxxSetObjectMacro(vtkPVPart, SMPart, vtkSMPart);

int vtkPVPartCommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVPart::vtkPVPart()
{
  this->CommandFunction = vtkPVPartCommand;

  this->PartDisplay = NULL;
  this->Displays = vtkCollection::New();

  this->Name = NULL;

  this->ClassNameInformation = vtkPVClassNameInformation::New();

  this->SMPart = 0;
}

//----------------------------------------------------------------------------
vtkPVPart::~vtkPVPart()
{  
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = 0;
  if(pvApp)
    {
    pm = pvApp->GetProcessModule();
    }

  this->SetPartDisplay(NULL);
  this->Displays->Delete();
  this->Displays = NULL;

  this->SetName(NULL);

  this->ClassNameInformation->Delete();
  this->ClassNameInformation = NULL;

  if (this->SMPart)
    {
    this->SMPart->Delete();
    }
}


//----------------------------------------------------------------------------
void vtkPVPart::SetPVApplication(vtkPVApplication *pvApp)
{
  if ( pvApp )
    {
    this->CreateParallelTclObjects(pvApp);
    }
  this->vtkKWObject::SetApplication(pvApp);
}


//----------------------------------------------------------------------------
void vtkPVPart::CreateParallelTclObjects(vtkPVApplication *pvApp)
{
  if ( !pvApp )
    {
    return;
    }
  this->vtkKWObject::SetApplication(pvApp);
}

//----------------------------------------------------------------------------
void vtkPVPart::AddDisplay(vtkPVDisplay* disp)
{
  if (disp)
    {
    this->Displays->AddItem(disp);
    }
} 

//----------------------------------------------------------------------------
void vtkPVPart::SetPartDisplay(vtkPVPartDisplay* pDisp)
{
  if (this->PartDisplay)
    {
    this->Displays->RemoveItem(this->PartDisplay);
    this->PartDisplay->UnRegister(this);
    this->PartDisplay = NULL;
    }
  if (pDisp)
    {
    this->Displays->AddItem(pDisp);
    this->PartDisplay = pDisp;
    this->PartDisplay->Register(this);
    this->PartDisplay->SetInput(this);
    }
}

//----------------------------------------------------------------------------
void vtkPVPart::Update()
{
  vtkCollectionIterator* sit = this->Displays->NewIterator();

  for (sit->InitTraversal(); !sit->IsDoneWithTraversal(); sit->GoToNextItem())
    {
    vtkPVDisplay* disp = vtkPVDisplay::SafeDownCast(sit->GetObject());
    if (disp)
      {
      disp->Update();
      }
    }
  sit->Delete();
}

//----------------------------------------------------------------------------
void vtkPVPart::SetVisibility(int v)
{
  if (this->PartDisplay)
    {
    this->PartDisplay->SetVisibility(v);
    }
}

//----------------------------------------------------------------------------
void vtkPVPart::MarkForUpdate()
{
  vtkCollectionIterator* sit = this->Displays->NewIterator();

  for (sit->InitTraversal(); !sit->IsDoneWithTraversal(); sit->GoToNextItem())
    {
    vtkPVDisplay* disp = vtkPVDisplay::SafeDownCast(sit->GetObject());
    if (disp)
      {
      disp->InvalidateGeometry();
      }
    }
  sit->Delete();
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkPVPart::GetDataInformation()
{
  if (!this->SMPart)
    {
    return 0;
    }
  return this->SMPart->GetDataInformation();
}

//----------------------------------------------------------------------------
void vtkPVPart::GatherDataInformation()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  if ( !pvApp )
    {
    return;
    }
  pvApp->SendPrepareProgress();

  // This does nothing if the geometry is already up to date.
  if (this->PartDisplay)
    {
    this->PartDisplay->Update();
    }

  // Recompute total visibile memory size. !!!!!!!
  // This should really be in vtkPVPartDisplay when it gathers its informantion.
  pvApp->GetRenderModule()->SetTotalVisibleMemorySizeValid(0);

  vtkPVDataInformation* info = this->SMPart->GetDataInformation();

  // Look for a name defined in Field data.
  this->SetName(info->GetName());

  // I would like to have all sources generate names and all filters
  // Pass the field data, but until all is working well, this is a default.
  if (this->Name == NULL || this->Name[0] == '\0')
    {
    char str[256];
    if (info->GetDataSetType() == VTK_POLY_DATA)
      {
      long nc = info->GetNumberOfCells();
      sprintf(str, "Polygonal: %ld cells", nc);
      }
    else if (info->GetDataSetType() == VTK_UNSTRUCTURED_GRID)
      {
      long nc = info->GetNumberOfCells();
      sprintf(str, "Unstructured Grid: %ld cells", nc);
      }
    else if (info->GetDataSetType() == VTK_IMAGE_DATA)
      {
      int *ext = info->GetExtent();
      sprintf(str, "Uniform Rectilinear: extent (%d, %d) (%d, %d) (%d, %d)", 
              ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
      }
    else if (info->GetDataSetType() == VTK_RECTILINEAR_GRID)
      {
      int *ext = info->GetExtent();
      sprintf(str, "Nonuniform Rectilinear: extent (%d, %d) (%d, %d) (%d, %d)", 
              ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
      }
    else if (info->GetDataSetType() == VTK_STRUCTURED_GRID)
      {
      int *ext = info->GetExtent();
      sprintf(str, "Curvilinear: extent (%d, %d) (%d, %d) (%d, %d)", 
              ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
      }
    else
      {
      sprintf(str, "Part of unknown type");
      }    
    this->SetName(str);
    }
  pvApp->SendCleanupPendingProgress();
}

//----------------------------------------------------------------------------
vtkClientServerID vtkPVPart::GetVTKDataID()
{
  if (!this->SMPart)
    {
    vtkClientServerID id = {0};
    return id;
    }
  return this->SMPart->GetID(0);
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVPart::GetPVApplication()
{
  if (this->GetApplication() == NULL)
    {
    return NULL;
    }
  
  if (this->GetApplication()->IsA("vtkPVApplication"))
    {  
    return (vtkPVApplication*)(this->GetApplication());
    }
  else
    {
    vtkErrorMacro("Bad typecast");
    return NULL;
    } 
}


//----------------------------------------------------------------------------
void vtkPVPart::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Name: " << (this->Name?this->Name:"none") << endl;
  os << indent << "ClassNameInformation: " << this->ClassNameInformation << endl;
}
