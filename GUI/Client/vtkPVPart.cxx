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
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkPVClassNameInformation.h"
#include "vtkPVConfig.h"
#include "vtkPVDataInformation.h"
#include "vtkPVProcessModule.h"
#include "vtkPVPartDisplay.h"
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
vtkCxxRevisionMacro(vtkPVPart, "1.54");

vtkCxxSetObjectMacro(vtkPVPart, SMPart, vtkSMPart);
vtkCxxSetObjectMacro(vtkPVPart, ProcessModule, vtkPVProcessModule);



//----------------------------------------------------------------------------
vtkPVPart::vtkPVPart()
{
  this->Name = NULL;

  this->ClassNameInformation = vtkPVClassNameInformation::New();

  this->SMPart = 0;
  this->ProcessModule = 0;
}

//----------------------------------------------------------------------------
vtkPVPart::~vtkPVPart()
{  
  this->SetName(NULL);

  this->ClassNameInformation->Delete();
  this->ClassNameInformation = NULL;

  if (this->SMPart)
    {
    this->SMPart->Delete();
    }
  this->SetProcessModule(0);
}


//----------------------------------------------------------------------------
void vtkPVPart::CreateParallelTclObjects(vtkPVProcessModule *pm)
{
  this->SetProcessModule(pm);
}

//----------------------------------------------------------------------------
void vtkPVPart::AddDisplay(vtkPVDisplay* disp)
{
  if (this->SMPart == 0)
    {
    vtkErrorMacro("Missing SMPart.");
    return;
    }
  this->SMPart->AddDisplay(disp);
} 

//----------------------------------------------------------------------------
void vtkPVPart::SetPartDisplay(vtkPVPartDisplay* disp)
{
  if (this->SMPart == 0)
    {
    vtkErrorMacro("Missing SMPart.");
    return;
    }
  this->SMPart->SetPartDisplay(disp);
} 

//----------------------------------------------------------------------------
vtkPVPartDisplay* vtkPVPart::GetPartDisplay()
{
  if (this->SMPart == 0)
    {
    vtkErrorMacro("Missing SMPart.");
    return 0;
    }
  return this->SMPart->GetPartDisplay();
} 

//----------------------------------------------------------------------------
void vtkPVPart::Update()
{
  if (this->SMPart == 0)
    {
    vtkErrorMacro("Missing SMPart.");
    return;
    }

  this->SMPart->Update();
}

//----------------------------------------------------------------------------
void vtkPVPart::SetVisibility(int v)
{
  if (this->SMPart == 0)
    {
    vtkErrorMacro("Missing SMPart.");
    return;
    }

  if (this->SMPart->GetPartDisplay())
    {
    this->SMPart->GetPartDisplay()->SetVisibility(v);
    }
}

//----------------------------------------------------------------------------
void vtkPVPart::MarkForUpdate()
{
  if (this->SMPart == 0)
    {
    vtkErrorMacro("Missing SMPart.");
    return;
    }

  this->SMPart->MarkForUpdate();
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
  vtkPVProcessModule *pm = this->GetProcessModule();
  if ( !pm )
    {
    return;
    }
  pm->SendPrepareProgress();

  // This does nothing if the geometry is already up to date.
  if (this->SMPart && this->SMPart->GetPartDisplay())
    {
    this->SMPart->GetPartDisplay()->Update();
    }

  // Recompute total visibile memory size. !!!!!!!
  // This should really be in vtkPVPartDisplay when it gathers its informantion.
  pm->GetRenderModule()->SetTotalVisibleMemorySizeValid(0);

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
  pm->SendCleanupPendingProgress();
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
void vtkPVPart::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Name: " << (this->Name?this->Name:"none") << endl;
  os << indent << "ClassNameInformation: " << this->ClassNameInformation << endl;
}
