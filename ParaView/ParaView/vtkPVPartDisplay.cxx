/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPartDisplay.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVPartDisplay.h"

#include "vtkPVRenderModule.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkProp3D.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkString.h"
#include "vtkPVColorMap.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkFieldDataToAttributeDataFilter.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPartDisplay);
vtkCxxRevisionMacro(vtkPVPartDisplay, "1.6");


//----------------------------------------------------------------------------
vtkPVPartDisplay::vtkPVPartDisplay()
{
  this->PVApplication = NULL;

  this->DirectColorFlag = 1;
  this->Visibility = 1;
  this->Part = NULL;

  // Used to be in vtkPVActorComposite
  static int instanceCount = 0;

  this->Mapper = NULL;
  this->Property = NULL;
  this->PropertyTclName = NULL;
  this->Prop = NULL;
  this->PropTclName = NULL;
  this->MapperTclName = NULL;
  this->UpdateSuppressorTclName = NULL;

  this->GeometryIsValid = 0;

  // Create a unique id for creating tcl names.
  ++instanceCount;
  this->InstanceCount = instanceCount;
}

//----------------------------------------------------------------------------
vtkPVPartDisplay::~vtkPVPartDisplay()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
    
  if ( pvApp && this->MapperTclName)
    {
    pvApp->BroadcastScript("%s Delete", this->MapperTclName);
    }
  this->SetMapperTclName(NULL);
  this->Mapper = NULL;
    
  if ( pvApp && this->PropTclName)
    {
    pvApp->BroadcastScript("%s Delete", this->PropTclName);
    }
  this->SetPropTclName(NULL);
  this->Prop = NULL;
  
  if ( pvApp && this->PropertyTclName)
    {
    pvApp->BroadcastScript("%s Delete", this->PropertyTclName);
    }
  this->SetPropertyTclName(NULL);
  this->Property = NULL;
  
  if (this->UpdateSuppressorTclName)
    {
    if ( pvApp )
      {
      pvApp->BroadcastScript("%s Delete", this->UpdateSuppressorTclName);
      }
    this->SetUpdateSuppressorTclName(NULL);
    }

  this->SetPart(NULL);

  this->SetPVApplication( NULL);
}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::InvalidateGeometry()
{
  this->GeometryIsValid = 0;
  this->RemoveAllCaches();
}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::ConnectToGeometry(char* geometryTclName)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  pvApp->BroadcastScript("%s SetInput [%s GetOutput]", 
                         this->UpdateSuppressorTclName, geometryTclName);
}


//----------------------------------------------------------------------------
void vtkPVPartDisplay::CreateParallelTclObjects(vtkPVApplication *pvApp)
{
  char tclName[100];
  

  // Now create the update supressors which keep the renderers/mappers
  // from updating the pipeline.  These are here to ensure that all
  // processes get updated at the same time.
  // ===== Primary branch:
  sprintf(tclName, "UpdateSuppressor%d", this->InstanceCount);
  pvApp->BroadcastScript("vtkPVUpdateSuppressor %s", tclName);
  this->SetUpdateSuppressorTclName(tclName);

  // Now create the mapper.
  sprintf(tclName, "Mapper%d", this->InstanceCount);
  this->Mapper = (vtkPolyDataMapper*)pvApp->MakeTclObject("vtkPolyDataMapper",
                                                          tclName);
  this->MapperTclName = NULL;
  this->SetMapperTclName(tclName);
  pvApp->BroadcastScript("%s UseLookupTableScalarRangeOn", this->MapperTclName);
  //pvApp->BroadcastScript("%s SetColorModeToMapScalars", this->MapperTclName);
  pvApp->BroadcastScript("%s SetInput [%s GetOutput]", this->MapperTclName,
                         this->UpdateSuppressorTclName);

  // Create a LOD Actor for the subclasses.
  // I could use just a plain actor for this class.
  sprintf(tclName, "Actor%d", this->InstanceCount);
  this->Prop = (vtkProp*)pvApp->MakeTclObject("vtkPVLODActor", tclName);
  this->SetPropTclName(tclName);

  // Make a new tcl object.
  sprintf(tclName, "Property%d", this->InstanceCount);
  this->Property = (vtkProperty*)pvApp->MakeTclObject("vtkProperty", tclName);
  this->SetPropertyTclName(tclName);
  pvApp->BroadcastScript("%s SetAmbient 0.15", this->PropertyTclName);
  pvApp->BroadcastScript("%s SetDiffuse 0.85", this->PropertyTclName);
  pvApp->BroadcastScript("%s SetProperty %s", this->PropTclName, 
                         this->PropertyTclName);
  pvApp->BroadcastScript("%s SetMapper %s", this->PropTclName, 
                         this->MapperTclName);

  int numPartitions = pvApp->GetProcessModule()->GetNumberOfPartitions();
  int partition =  pvApp->GetProcessModule()->GetPartitionId();

  // Broadcast for subclasses.  
  pvApp->BroadcastScript("%s SetUpdateNumberOfPieces [[$Application GetProcessModule] GetNumberOfPartitions]",
                        this->UpdateSuppressorTclName);
  pvApp->BroadcastScript("%s SetUpdatePiece [[$Application GetProcessModule] GetPartitionId]",
                        this->UpdateSuppressorTclName);
}


//----------------------------------------------------------------------------
void vtkPVPartDisplay::SetUseImmediateMode(int val)
{
  vtkPVApplication* pvApp = this->GetPVApplication();

  pvApp->BroadcastScript("%s SetImmediateModeRendering %d",
                         this->MapperTclName, val);
}


//----------------------------------------------------------------------------
void vtkPVPartDisplay::SetVisibility(int v)
{
  vtkPVApplication* pvApp = this->GetPVApplication();

  if (this->GetPropTclName())
    {
    pvApp->BroadcastScript("%s SetVisibility %d", this->GetPropTclName(), v);
    }
  this->Visibility = v;

  // Recompute total visibile memory size.
  pvApp->GetRenderModule()->SetTotalVisibleMemorySizeValid(0);
}


//----------------------------------------------------------------------------
void vtkPVPartDisplay::SetColor(float r, float g, float b)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  pvApp->BroadcastScript("%s SetColor %f %f %f", 
                         this->PropertyTclName, r, g, b);

  // Add a bit of specular when just coloring by property.
  pvApp->BroadcastScript("%s SetSpecular 0.1", 
                         this->PropertyTclName);

  pvApp->BroadcastScript("%s SetSpecularPower 100.0", 
                         this->PropertyTclName);

  pvApp->BroadcastScript("%s SetSpecularColor 1.0 1.0 1.0", 
                         this->PropertyTclName);
}  


//----------------------------------------------------------------------------
void vtkPVPartDisplay::Update()
{
  vtkPVApplication* pvApp = this->GetPVApplication();

  if ( ! this->GeometryIsValid && this->UpdateSuppressorTclName )
    {
    pvApp->BroadcastScript("%s ForceUpdate", this->UpdateSuppressorTclName);
    this->GeometryIsValid = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::SetPVApplication(vtkPVApplication *pvApp)
{
  if (pvApp == NULL)
    {
    if (this->PVApplication)
      {
      this->PVApplication->Delete();
      this->PVApplication = NULL;
      }
    return;
    }

  if (this->PVApplication)
    {
    vtkErrorMacro("PVApplication already set and part has been initialized.");
    return;
    }

  this->CreateParallelTclObjects(pvApp);
  this->PVApplication = pvApp;
  this->PVApplication->Register(this);
}



//----------------------------------------------------------------------------
void vtkPVPartDisplay::RemoveAllCaches()
{
  this->GetPVApplication()->BroadcastScript(
             "%s RemoveAllCaches",
             this->UpdateSuppressorTclName);
}


//----------------------------------------------------------------------------
// Assume that this method is only called when the part is visible.
// This is like the ForceUpdate method, but uses cached values if possible.
void vtkPVPartDisplay::CacheUpdate(int idx, int total)
{
  this->GetPVApplication()->BroadcastScript(
             "%s CacheUpdate %d %d",
             this->UpdateSuppressorTclName, idx, total);
}






//----------------------------------------------------------------------------
void vtkPVPartDisplay::SetScalarVisibility(int val)
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  pvApp->BroadcastScript("%s SetScalarVisibility %d", 
                         this->MapperTclName, val);
}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::SetDirectColorFlag(int val)
{
  if (val)
    {
    val = 1;
    }
  if (val == this->DirectColorFlag)
    {
    return;
    }

  vtkPVApplication* pvApp = this->GetPVApplication();
  this->DirectColorFlag = val;
  if (val)
    {
    pvApp->BroadcastScript("%s SetColorModeToDefault", this->MapperTclName);
    }
  else
    {
    pvApp->BroadcastScript("%s SetColorModeToMapScalars", this->MapperTclName);
    }
}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::ColorByArray(vtkPVColorMap *colorMap,
                                    int field)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  // Turn off the specualr so it does not interfere with data.
  pvApp->BroadcastScript("%s SetSpecular 0.0", 
                         this->PropertyTclName);

  pvApp->BroadcastScript("%s SetLookupTable %s", 
                         this->MapperTclName,
                         colorMap->GetLookupTableTclName());
  pvApp->BroadcastScript("%s ScalarVisibilityOn", 
                         this->MapperTclName);
  if (field == VTK_CELL_DATA_FIELD)
    {
    pvApp->BroadcastScript("%s SetScalarModeToUseCellFieldData",
                           this->MapperTclName);
    }
  else if (field == VTK_POINT_DATA_FIELD)
    {
    pvApp->BroadcastScript("%s SetScalarModeToUsePointFieldData",
                           this->MapperTclName);
    }
  else
    {
    vtkErrorMacro("Only point or cell field please.");
    }

  pvApp->BroadcastScript("%s SelectColorArray {%s}",
                         this->MapperTclName, colorMap->GetArrayName());

}

//----------------------------------------------------------------------------
void vtkPVPartDisplay::PrintSelfostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Visibility: " << this->Visibility << endl;
  os << indent << "Part: " << this->Part << endl;
  os << indent << "Mapper: " << this->GetMapper() << endl;
  os << indent << "MapperTclName: " << (this->MapperTclName?this->MapperTclName:"none") << endl;
  os << indent << "PropTclName: " << (this->PropTclName?this->PropTclName:"none") << endl;
  os << indent << "PropertyTclName: " << (this->PropertyTclName?this->PropertyTclName:"none") << endl;
  os << indnet << "PVApplication: " << this->PVApplication << endl;

  os << indent << "DirectColorFlag: " << this->DirectColorFlag << endl;

  if (this->UpdateSuppressorTclName)
    {
    os << indent << "UpdateSuppressor: " << this->UpdateSuppressorTclName << endl;
    }
}


  



