/*=========================================================================

  Program:   ParaView
  Module:    vtkPVData.cxx
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
#include "vtkPVData.h"
#include "vtkPVSource.h"
#include "vtkKWView.h"
#include "vtkPVWindow.h"
#include "vtkPVApplication.h"
#include "vtkPVActorComposite.h"
#include "vtkKWMenuButton.h"
#include "vtkObjectFactory.h"


int vtkPVDataCommand(ClientData cd, Tcl_Interp *interp,
		     int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVData::vtkPVData()
{
  this->CommandFunction = vtkPVDataCommand;

  this->VTKData = NULL;
  this->VTKDataTclName = NULL;
  this->PVSource = NULL;
  
  this->NumberOfPVConsumers = 0;
  this->PVConsumers = 0;
}

//----------------------------------------------------------------------------
vtkPVData::~vtkPVData()
{
  // Get rid of the circular reference created by the extent translator.
  if (this->VTKDataTclName)
    {
    this->GetPVApplication()->BroadcastScript("%s SetExtentTranslator {}",
					      this->VTKDataTclName);
    }  
    
  this->SetVTKData(NULL, NULL);
  this->SetPVSource(NULL);
  
  delete [] this->PVConsumers;
}

//----------------------------------------------------------------------------
vtkPVData* vtkPVData::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVData");
  if(ret)
    {
    return (vtkPVData*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVData;
}

//----------------------------------------------------------------------------
void vtkPVData::SetApplication(vtkPVApplication *pvApp)
{
  this->CreateParallelTclObjects(pvApp);
  this->vtkPVActorComposite::SetApplication(pvApp);
}


//----------------------------------------------------------------------------
// Tcl does the reference counting, so we are not going to put an 
// additional reference of the data.
void vtkPVData::SetVTKData(vtkDataSet *data, const char *tclName)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp == NULL)
    {
    vtkErrorMacro("Set the application before you set the VTKDataTclName.");
    return;
    }
  
  if (this->VTKDataTclName)
    {
    pvApp->BroadcastScript("%s Delete", this->VTKDataTclName);
    delete [] this->VTKDataTclName;
    this->VTKDataTclName = NULL;
    this->VTKData = NULL;
    }
  if (tclName)
    {
    this->VTKDataTclName = new char[strlen(tclName) + 1];
    strcpy(this->VTKDataTclName, tclName);
    this->VTKData = data;
    
    // This is why we need a special method.  The actor comoposite can't set its input until
    // the data tcl name is set.  These dependencies on the order things are set
    // leads me to think there sould be one initialize method which sets all the variables.
    
    this->SetInput(this);
    }
}

void vtkPVData::AddPVConsumer(vtkPVSource *c)
{
  // make sure it isn't already there
  if (this->IsPVConsumer(c))
    {
    return;
    }
  // add it to the list, reallocate memory
  vtkPVSource **tmp = this->PVConsumers;
  this->NumberOfPVConsumers++;
  this->PVConsumers = new vtkPVSource* [this->NumberOfPVConsumers];
  for (int i = 0; i < (this->NumberOfPVConsumers-1); i++)
    {
    this->PVConsumers[i] = tmp[i];
    }
  this->PVConsumers[this->NumberOfPVConsumers-1] = c;
  // free old memory
  delete [] tmp;
}

void vtkPVData::RemovePVConsumer(vtkPVSource *c)
{
  // make sure it is already there
  if (!this->IsPVConsumer(c))
    {
    return;
    }
  // remove it from the list, reallocate memory
  vtkPVSource **tmp = this->PVConsumers;
  this->NumberOfPVConsumers--;
  this->PVConsumers = new vtkPVSource* [this->NumberOfPVConsumers];
  int cnt = 0;
  int i;
  for (i = 0; i <= this->NumberOfPVConsumers; i++)
    {
    if (tmp[i] != c)
      {
      this->PVConsumers[cnt] = tmp[i];
      cnt++;
      }
    }
  // free old memory
  delete [] tmp;
}

int vtkPVData::IsPVConsumer(vtkPVSource *c)
{
  int i;
  for (i = 0; i < this->NumberOfPVConsumers; i++)
    {
    if (this->PVConsumers[i] == c)
      {
      return 1;
      }
    }
  return 0;
}

vtkPVSource *vtkPVData::GetPVConsumer(int i)
{
  if (i >= this->NumberOfPVConsumers)
    {
    return 0;
    }
  return this->PVConsumers[i];
}

//----------------------------------------------------------------------------
// Data is expected to be updated.
void vtkPVData::GetBounds(float bounds[6])
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkMultiProcessController *controller = pvApp->GetController();
  float tmp[6];
  int id, num;
  
  if (this->VTKData == NULL)
    {
    bounds[0] = bounds[1] = bounds[2] = VTK_LARGE_FLOAT;
    bounds[3] = bounds[4] = bounds[5] = -VTK_LARGE_FLOAT;
    return;
    }

  pvApp->BroadcastScript("Application SendDataBounds %s", 
			this->VTKDataTclName);
  
  this->VTKData->GetBounds(bounds);
  
  num = controller->GetNumberOfProcesses();
  for (id = 1; id < num; ++id)
    {
    controller->Receive(tmp, 6, id, 1967);
    if (tmp[0] < bounds[0])
      {
      bounds[0] = tmp[0];
      }
    if (tmp[1] > bounds[1])
      {
      bounds[1] = tmp[1];
      }
    if (tmp[2] < bounds[2])
      {
      bounds[2] = tmp[2];
      }
    if (tmp[3] > bounds[3])
      {
      bounds[3] = tmp[3];
      }
    if (tmp[4] < bounds[4])
      {
      bounds[4] = tmp[4];
      }
    if (tmp[5] > bounds[5])
      {
      bounds[5] = tmp[5];
      }
    }
}

//----------------------------------------------------------------------------
// Data is expected to be updated.
int vtkPVData::GetNumberOfCells()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkMultiProcessController *controller = pvApp->GetController();
  int tmp, numCells, id, numProcs;
  
  if (this->VTKData == NULL)
    {
    return 0;
    }

  pvApp->BroadcastScript("Application SendDataNumberOfCells %s", 
			this->VTKDataTclName);
  
  numCells = this->VTKData->GetNumberOfCells();
  
  numProcs = controller->GetNumberOfProcesses();
  for (id = 1; id < numProcs; ++id)
    {
    controller->Receive(&tmp, 1, id, 1968);
    numCells += tmp;
    }
  return numCells;
}

//----------------------------------------------------------------------------
// Data is expected to be updated.
int vtkPVData::GetNumberOfPoints()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkMultiProcessController *controller = pvApp->GetController();
  int tmp, numPoints, id, numProcs;
  
  if (this->VTKData == NULL)
    {
    return 0;
    }

  pvApp->BroadcastScript("Application SendDataNumberOfPoints %s", 
			this->VTKDataTclName);
  
  numPoints = this->VTKData->GetNumberOfPoints();
  
  numProcs = controller->GetNumberOfProcesses();
  for (id = 1; id < numProcs; ++id)
    {
    controller->Receive(&tmp, 1, id, 1969);
    numPoints += tmp;
    }
  return numPoints;
}

//----------------------------------------------------------------------------
// WE DO NOT REFERENCE COUNT HERE BECAUSE OF CIRCULAR REFERENCES.
// THE SOURCE OWNS THE DATA.
void vtkPVData::SetPVSource(vtkPVSource *source)
{
  if (this->PVSource == source)
    {
    return;
    }
  this->Modified();

  this->PVSource = source;
}

//----------------------------------------------------------------------------
void vtkPVData::Update()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  // The mapper has the assignment for this processor.
  pvApp->BroadcastScript("%s SetUpdateExtent [%s GetPiece] [%s GetNumberOfPieces]", 
			 this->VTKDataTclName, 
			 this->MapperTclName, this->MapperTclName);
  pvApp->BroadcastScript("%s Update", this->VTKDataTclName);
}


//----------------------------------------------------------------------------
void vtkPVData::InsertExtractPiecesIfNecessary()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if ( this->VTKData == NULL)
    {
    return;
    }
  this->VTKData->UpdateInformation();
  if (this->VTKData->GetMaximumNumberOfPieces() != 1)
    { // The source can already produce pieces.
    return;
    }
  
  // We are going to create the piece filter with a dummy tcl name,
  // setup the pipeline, and remove tcl's reference to the objects.
  // The vtkData object will be moved to the output of the piece filter.
  if (this->VTKData->IsA("vtkPolyData"))
    {
    pvApp->BroadcastSimpleScript("vtkExtractPolyDataPiece pvTemp");
    }
  else if (this->VTKData->IsA("vtkUnstructuredGrid"))
    {
    pvApp->BroadcastSimpleScript("vtkExtractUnstructuredGridPiece pvTemp");
    }
  else
    {
    vtkWarningMacro("We do not extract pieces from structured data.");
    return;
    }

  pvApp->BroadcastSimpleScript("pvTemp SetInput [pvTemp GetOutput]");
  pvApp->BroadcastScript("pvTemp SetOutput %s", this->VTKDataTclName);
  pvApp->BroadcastScript("%s SetOutput [pvTemp GetInput]",
                         this->PVSource->GetVTKSourceTclName());

  // Now delete Tcl's reference to the piece filter.
  pvApp->BroadcastSimpleScript("pvTemp Delete");
}
