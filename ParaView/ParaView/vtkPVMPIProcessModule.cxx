/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMPIProcessModule.cxx
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
#include "vtkPVMPIProcessModule.h"
#include "vtkObjectFactory.h"

#include "vtkToolkits.h"
#include "vtkPVConfig.h"
#include "vtkMultiProcessController.h"
#include "vtkPVApplication.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkCharArray.h"
#include "vtkLongArray.h"
#include "vtkShortArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkMapper.h"
#include "vtkString.h"
#ifdef VTK_USE_MPI
#include "vtkMPIController.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIGroup.h"
#endif

#include "vtkPVData.h"



// external global variable.
vtkMultiProcessController *VTK_PV_UI_CONTROLLER = NULL;


//----------------------------------------------------------------------------
void vtkPVSlaveScript(void *localArg, void *remoteArg, 
                      int vtkNotUsed(remoteArgLength),
                      int vtkNotUsed(remoteProcessId))
{
  vtkPVApplication *self = (vtkPVApplication *)(localArg);

  //cerr << " ++++ SlaveScript: " << ((char*)remoteArg) << endl;  
  self->SimpleScript((char*)remoteArg);
}


 //----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMPIProcessModule);
vtkCxxRevisionMacro(vtkPVMPIProcessModule, "1.1");

int vtkPVMPIProcessModuleCommand(ClientData cd, Tcl_Interp *interp,
                            int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVMPIProcessModule::vtkPVMPIProcessModule()
{
  this->Controller = NULL;

  this->ArgumentCount = 0;
  this->Arguments = NULL;
  this->ReturnValue = 0;
}

//----------------------------------------------------------------------------
vtkPVMPIProcessModule::~vtkPVMPIProcessModule()
{
  if (this->Controller)
    {
    this->Controller->Delete();
    this->Controller = NULL;
    }


  this->ArgumentCount = 0;
  this->Arguments = NULL;
  this->ReturnValue = 0;
}


//----------------------------------------------------------------------------
// Each process starts with this method.  One process is designated as
// "master" and starts the application.  The other processes are slaves to
// the application.
void vtkPVMPIProcessModuleInit(vtkMultiProcessController *controller, void *arg )
{
  vtkPVMPIProcessModule *self = (vtkPVMPIProcessModule *)arg;
  self->Initialize();
}


//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::Initialize()
{
  int myId, numProcs;
  
  myId = this->Controller->GetLocalProcessId();
  numProcs = this->Controller->GetNumberOfProcesses();

#ifdef MPIPROALLOC
  vtkCommunicator::SetUseCopy(1);
#endif

  if (myId ==  0)
    { // The last process is for UI.
    vtkPVApplication *pvApp = this->GetPVApplication();
    // This is for the SGI pipes option.
    pvApp->SetNumberOfPipes(numProcs);
    
#ifdef PV_HAVE_TRAPS_FOR_SIGNALS
    pvApp->SetupTrapsForSignals(myId);   
#endif // PV_HAVE_TRAPS_FOR_SIGNALS

    pvApp->Script("wm withdraw .");
    pvApp->Start(this->ArgumentCount,this->Arguments);
    this->ReturnValue = pvApp->GetExitStatus();
    }
  else
    {
    vtkPVApplication *pvApp = this->GetPVApplication();
    this->Controller->AddRMI(vtkPVSlaveScript, (void *)(pvApp), 
                             VTK_PV_SLAVE_SCRIPT_RMI_TAG);
    this->Controller->ProcessRMIs();
    }
}





//----------------------------------------------------------------------------
int vtkPVMPIProcessModule::Start(int argc, char **argv)
{
  // Initialize the MPI controller.
  this->Controller = vtkMultiProcessController::New();
  this->Controller->Initialize(&argc, &argv, 1);

  if (this->Controller->GetNumberOfProcesses() > 1)
    { // !!!!! For unix, this was done when MPI was defined (even for 1 process). !!!!!
    this->Controller->CreateOutputWindow();
    }
  this->ArgumentCount = argc;
  this->Arguments = argv;
 
  // Go through the motions.
  // This indirection is not really necessary and is just to mimick the
  // threaded controller.
  this->Controller->SetSingleMethod(vtkPVMPIProcessModuleInit,(void*)(this));
  this->Controller->SingleMethodExecute();
  
  this->Controller->Finalize();

  return this->ReturnValue;
}


//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::Exit()
{
  int id, myId, num;
  
  // Send a break RMI to each of the slaves.
  num = this->Controller->GetNumberOfProcesses();
  myId = this->Controller->GetLocalProcessId();
  for (id = 0; id < num; ++id)
    {
    if (id != myId)
      {
      this->Controller->TriggerRMI(id, 
                                   vtkMultiProcessController::BREAK_RMI_TAG);
      }
    }
}

//----------------------------------------------------------------------------
int vtkPVMPIProcessModule::GetPartitionId()
{
  if (this->Controller)
    {
    return this->Controller->GetLocalProcessId();
    }
  return 0;
}


//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::BroadcastSimpleScript(const char *str)
{
  int id, num;
    
  if (this->Application == NULL)
    {
    vtkErrorMacro("Missing application object.");
    return;
    }

  num = this->GetNumberOfPartitions();

  int len = vtkString::Length(str);
  if (!str || (len < 1))
    {
    return;
    }

  for (id = 1; id < num; ++id)
    {
    this->RemoteSimpleScript(id, str);
    }
  
  // Do reverse order, because 0 will block.
  this->Application->SimpleScript(str);
}


//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::RemoteSimpleScript(int remoteId, const char *str)
{
  int length;
  
  if (this->Application == NULL)
    {
    vtkErrorMacro("Missing application object.");
    return;
    }

  // send string to evaluate.
  length = vtkString::Length(str) + 1;
  if (length <= 1)
    {
    return;
    }

  if (this->Controller->GetLocalProcessId() == remoteId)
    {
    this->Application->SimpleScript(str);
    return;
    }
  
  this->Controller->TriggerRMI(remoteId, const_cast<char*>(str), 
                               VTK_PV_SLAVE_SCRIPT_RMI_TAG);
}







//----------------------------------------------------------------------------
int vtkPVMPIProcessModule::GetNumberOfPartitions()
{
  if (this->Controller)
    {
    return this->Controller->GetNumberOfProcesses();
    }
  return 1;
}


//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::GetPVDataBounds(vtkPVData *pvd, float bounds[6])
{
  vtkMultiProcessController *controller = this->GetController();
  float tmp[6];
  int id, num;
  vtkDataSet *ds = pvd->GetVTKData();

  pvd->GetVTKData()->GetBounds(bounds);

  // Process module should have a unique tcl name or variable !!!!!!!!!!
  this->BroadcastScript("[$Application GetProcessModule] SendDataBounds %s", 
                        pvd->GetVTKDataTclName());
  
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
int vtkPVMPIProcessModule::GetPVDataNumberOfCells(vtkPVData *pvd)
{
  vtkMultiProcessController *controller = this->GetController();
  vtkDataSet *ds = pvd->GetVTKData();

  int tmp = 0;
  int numCells, id, numProcs;

  numCells = ds->GetNumberOfCells();

  this->BroadcastScript("[$Application GetProcessModule] SendDataNumberOfCells %s", 
                        pvd->GetVTKDataTclName());
  
  numProcs = controller->GetNumberOfProcesses();
  for (id = 1; id < numProcs; ++id)
    {
    controller->Receive(&tmp, 1, id, 1968);
    numCells += tmp;
    }
  return numCells;
}

//----------------------------------------------------------------------------
int vtkPVMPIProcessModule::GetPVDataNumberOfPoints(vtkPVData *pvd)
{
  vtkMultiProcessController *controller = this->GetController();
  vtkDataSet *ds = pvd->GetVTKData();

  int tmp = 0;
  int numPoints, id, numProcs;

  numPoints = ds->GetNumberOfPoints();

  this->BroadcastScript("[$Application GetProcessModule] SendDataNumberOfPoints %s", 
                        pvd->GetVTKDataTclName());
  
  numProcs = controller->GetNumberOfProcesses();
  for (id = 1; id < numProcs; ++id)
    {
    controller->Receive(&tmp, 1, id, 1969);
    numPoints += tmp;
    }
  return numPoints;
}

//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::GetPVDataArrayComponentRange(vtkPVData *pvd, int pointDataFlag,
                            const char *arrayName, int component, float *range)
{
  vtkMultiProcessController *controller = this->GetController();
  int id, num;
  vtkDataArray *array;
  float temp[2];

  range[0] = VTK_LARGE_FLOAT;
  range[1] = -VTK_LARGE_FLOAT;

  if (pointDataFlag)
    {
    array = pvd->GetVTKData()->GetPointData()->GetArray(arrayName);
    }
  else
    {
    array = pvd->GetVTKData()->GetCellData()->GetArray(arrayName);
    }

  if (array == NULL || array->GetName() == NULL)
    {
    return;
    }

  array->GetRange(range, component);  

  this->BroadcastScript("[$Application GetProcessModule] SendDataArrayRange %s %d {%s} %d",
                         pvd->GetVTKDataTclName(),
                         pointDataFlag, array->GetName(), component);
  
  num = controller->GetNumberOfProcesses();
  for (id = 1; id < num; id++)
    {
    controller->Receive(temp, 2, id, 1976);
    // try to protect against invalid ranges.
    if (range[0] > range[1])
      {
      range[0] = temp[0];
      range[1] = temp[1];
      }
    else if (temp[0] <= temp[1])
      {
      if (temp[0] < range[0])
        {
        range[0] = temp[0];
        }
      if (temp[1] > range[1])
        {
        range[1] = temp[1];
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::SendDataBounds(vtkDataSet *data)
{
  float *bounds;
  
  if (this->Controller->GetLocalProcessId() == 0)
    {
    return;
    }
  bounds = data->GetBounds();
  this->Controller->Send(bounds, 6, 0, 1967);
}

//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::SendDataNumberOfCells(vtkDataSet *data)
{
  int num;
  
  if (this->Controller->GetLocalProcessId() == 0)
    {
    return;
    }
  num = data->GetNumberOfCells();
  this->Controller->Send(&num, 1, 0, 1968);
}

//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::SendDataNumberOfPoints(vtkDataSet *data)
{
  int num;
  
  if (this->Controller->GetLocalProcessId() == 0)
    {
    return;
    }
  num = data->GetNumberOfPoints();
  this->Controller->Send(&num, 1, 0, 1969);
}

//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::SendDataArrayRange(vtkDataSet *data, 
                                          int pointDataFlag, 
                                          char *arrayName,
                                          int component)
{
  float range[2];
  vtkDataArray *array;

  if (this->Controller->GetLocalProcessId() == 0)
    {
    return;
    }
  
  if (pointDataFlag)
    {
    array = data->GetPointData()->GetArray(arrayName);
    }
  else
    {
    array = data->GetCellData()->GetArray(arrayName);
    }

  if (array && component >= 0 && component < array->GetNumberOfComponents())
    {
    array->GetRange(range, component);
    }
  else
    {
    range[0] = VTK_LARGE_FLOAT;
    range[1] = -VTK_LARGE_FLOAT;
    }

  this->Controller->Send(range, 2, 0, 1976);
}

//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::CompleteArrays(vtkMapper *mapper, char *mapperTclName)
{
  int i, j;
  int numProcs;
  int nonEmptyFlag = 0;
  int activeAttributes[5];

  if (mapper->GetInput() == NULL || this->Controller == NULL ||
      mapper->GetInput()->GetNumberOfPoints() > 0 ||
      mapper->GetInput()->GetNumberOfCells() > 0)
    {
    return;
    }

  // Find the first non empty data object on another processes.
   numProcs = this->Controller->GetNumberOfProcesses();
  for (i = 1; i < numProcs; ++i)
    {
    this->RemoteScript(i, "[$Application GetProcessModule] SendCompleteArrays %s", mapperTclName);
    this->Controller->Receive(&nonEmptyFlag, 1, i, 987243);
    if (nonEmptyFlag)
      { // This process has data.  Receive all the arrays, type and component.
      int num = 0;
      vtkDataArray *array = 0;
      char *name;
      int nameLength = 0;
      int type = 0;
      int numComps = 0;
      
      // First Point data.
      this->Controller->Receive(&num, 1, i, 987244);
      for (j = 0; j < num; ++j)
        {
        this->Controller->Receive(&type, 1, i, 987245);
        switch (type)
          {
          case VTK_INT:
            array = vtkIntArray::New();
            break;
          case VTK_FLOAT:
            array = vtkFloatArray::New();
            break;
          case VTK_DOUBLE:
            array = vtkDoubleArray::New();
            break;
          case VTK_CHAR:
            array = vtkCharArray::New();
            break;
          case VTK_LONG:
            array = vtkLongArray::New();
            break;
          case VTK_SHORT:
            array = vtkShortArray::New();
            break;
          case VTK_UNSIGNED_CHAR:
            array = vtkUnsignedCharArray::New();
            break;
          case VTK_UNSIGNED_INT:
            array = vtkUnsignedIntArray::New();
            break;
          case VTK_UNSIGNED_LONG:
            array = vtkUnsignedLongArray::New();
            break;
          case VTK_UNSIGNED_SHORT:
            array = vtkUnsignedShortArray::New();
            break;
          }
        this->Controller->Receive(&numComps, 1, i, 987246);
        array->SetNumberOfComponents(numComps);
        this->Controller->Receive(&nameLength, 1, i, 987247);
        name = new char[nameLength];
        this->Controller->Receive(name, nameLength, i, 987248);
        array->SetName(name);
        delete [] name;
        mapper->GetInput()->GetPointData()->AddArray(array);
        array->Delete();
        } // end of loop over point arrays.
      // Which scalars, ... are active?
      this->Controller->Receive(activeAttributes, 5, i, 987258);
      mapper->GetInput()->GetPointData()->SetActiveAttribute(activeAttributes[0],0);
      mapper->GetInput()->GetPointData()->SetActiveAttribute(activeAttributes[1],1);
      mapper->GetInput()->GetPointData()->SetActiveAttribute(activeAttributes[2],2);
      mapper->GetInput()->GetPointData()->SetActiveAttribute(activeAttributes[3],3);
      mapper->GetInput()->GetPointData()->SetActiveAttribute(activeAttributes[4],4);
 
      // Next Cell data.
      this->Controller->Receive(&num, 1, i, 987244);
      for (j = 0; j < num; ++j)
        {
        this->Controller->Receive(&type, 1, i, 987245);
        switch (type)
          {
          case VTK_INT:
            array = vtkIntArray::New();
            break;
          case VTK_FLOAT:
            array = vtkFloatArray::New();
            break;
          case VTK_DOUBLE:
            array = vtkDoubleArray::New();
            break;
          case VTK_CHAR:
            array = vtkCharArray::New();
            break;
          case VTK_LONG:
            array = vtkLongArray::New();
            break;
          case VTK_SHORT:
            array = vtkShortArray::New();
            break;
          case VTK_UNSIGNED_CHAR:
            array = vtkUnsignedCharArray::New();
            break;
          case VTK_UNSIGNED_INT:
            array = vtkUnsignedIntArray::New();
            break;
          case VTK_UNSIGNED_LONG:
            array = vtkUnsignedLongArray::New();
            break;
          case VTK_UNSIGNED_SHORT:
            array = vtkUnsignedShortArray::New();
            break;
          }
        this->Controller->Receive(&numComps, 1, i, 987246);
        array->SetNumberOfComponents(numComps);
        this->Controller->Receive(&nameLength, 1, i, 987247);
        name = new char[nameLength];
        this->Controller->Receive(name, nameLength, i, 987248);
        array->SetName(name);
        delete [] name;
        mapper->GetInput()->GetCellData()->AddArray(array);
        array->Delete();
        } // end of loop over cell arrays.
      // Which scalars, ... are active?
      this->Controller->Receive(activeAttributes, 5, i, 987258);
      mapper->GetInput()->GetCellData()->SetActiveAttribute(activeAttributes[0],0);
      mapper->GetInput()->GetCellData()->SetActiveAttribute(activeAttributes[1],1);
      mapper->GetInput()->GetCellData()->SetActiveAttribute(activeAttributes[2],2);
      mapper->GetInput()->GetCellData()->SetActiveAttribute(activeAttributes[3],3);
      mapper->GetInput()->GetCellData()->SetActiveAttribute(activeAttributes[4],4);
      
      // We only need information from one.
      return;
      } // End of if-non-empty check.
    }// End of loop over processes.
}

//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::SendCompleteArrays(vtkMapper *mapper)
{
  int nonEmptyFlag;
  int num;
  int i;
  int type;
  int numComps;
  int nameLength;
  const char *name;
  vtkDataArray *array;
  int activeAttributes[5];

  if (mapper->GetInput() == NULL ||
      (mapper->GetInput()->GetNumberOfPoints() == 0 &&
       mapper->GetInput()->GetNumberOfCells() == 0))
    {
    nonEmptyFlag = 0;
    this->Controller->Send(&nonEmptyFlag, 1, 0, 987243);
    return;
    }
  nonEmptyFlag = 1;
  this->Controller->Send(&nonEmptyFlag, 1, 0, 987243);

  // First point data.
  num = mapper->GetInput()->GetPointData()->GetNumberOfArrays();
  this->Controller->Send(&num, 1, 0, 987244);
  for (i = 0; i < num; ++i)
    {
    array = mapper->GetInput()->GetPointData()->GetArray(i);
    type = array->GetDataType();

    this->Controller->Send(&type, 1, 0, 987245);
    numComps = array->GetNumberOfComponents();

    this->Controller->Send(&numComps, 1, 0, 987246);
    name = array->GetName();
    if (name == NULL)
      {
      name = "";
      }
    nameLength = vtkString::Length(name)+1;
    this->Controller->Send(&nameLength, 1, 0, 987247);
    // I am pretty sure that Send does not modify the string.
    this->Controller->Send(const_cast<char*>(name), nameLength, 0, 987248);
    }
  mapper->GetInput()->GetPointData()->GetAttributeIndices(activeAttributes);
  this->Controller->Send(activeAttributes, 5, 0, 987258);

  // Next cell data.
  num = mapper->GetInput()->GetCellData()->GetNumberOfArrays();
  this->Controller->Send(&num, 1, 0, 987244);
  for (i = 0; i < num; ++i)
    {
    array = mapper->GetInput()->GetCellData()->GetArray(i);
    type = array->GetDataType();

    this->Controller->Send(&type, 1, 0, 987245);
    numComps = array->GetNumberOfComponents();

    this->Controller->Send(&numComps, 1, 0, 987246);
    name = array->GetName();
    if (name == NULL)
      {
      name = "";
      }
    nameLength = vtkString::Length(name+1);
    this->Controller->Send(&nameLength, 1, 0, 987247);
    this->Controller->Send(const_cast<char*>(name), nameLength, 0, 987248);
    }
  mapper->GetInput()->GetCellData()->GetAttributeIndices(activeAttributes);
  this->Controller->Send(activeAttributes, 5, 0, 987258);
}

//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::CompleteArrays(vtkDataSet *data, char *dataTclName)
{
  int i, j;
  int numProcs;
  int nonEmptyFlag = 0;
  int activeAttributes[5];

  if (data == NULL || this->Controller == NULL ||
      data->GetNumberOfPoints() > 0 ||
      data->GetNumberOfCells() > 0)
    {
    return;
    }

  // Find the first non empty data object on another processes.
  numProcs = this->Controller->GetNumberOfProcesses();
  for (i = 1; i < numProcs; ++i)
    {
    this->RemoteScript(i, "[$Application GetProcessModule] SendCompleteArrays %s", dataTclName);
    this->Controller->Receive(&nonEmptyFlag, 1, i, 987243);
    if (nonEmptyFlag)
      { // This process has data.  Receive all the arrays, type and component.
      int num = 0;
      vtkDataArray *array = 0;
      char *name;
      int nameLength = 0;
      int type = 0;
      int numComps = 0;
      
      // First Point data.
      this->Controller->Receive(&num, 1, i, 987244);
      for (j = 0; j < num; ++j)
        {
        this->Controller->Receive(&type, 1, i, 987245);
        switch (type)
          {
          case VTK_INT:
            array = vtkIntArray::New();
            break;
          case VTK_FLOAT:
            array = vtkFloatArray::New();
            break;
          case VTK_DOUBLE:
            array = vtkDoubleArray::New();
            break;
          case VTK_CHAR:
            array = vtkCharArray::New();
            break;
          case VTK_LONG:
            array = vtkLongArray::New();
            break;
          case VTK_SHORT:
            array = vtkShortArray::New();
            break;
          case VTK_UNSIGNED_CHAR:
            array = vtkUnsignedCharArray::New();
            break;
          case VTK_UNSIGNED_INT:
            array = vtkUnsignedIntArray::New();
            break;
          case VTK_UNSIGNED_LONG:
            array = vtkUnsignedLongArray::New();
            break;
          case VTK_UNSIGNED_SHORT:
            array = vtkUnsignedShortArray::New();
            break;
          }
        this->Controller->Receive(&numComps, 1, i, 987246);
        array->SetNumberOfComponents(numComps);
        this->Controller->Receive(&nameLength, 1, i, 987247);
        name = new char[nameLength];
        this->Controller->Receive(name, nameLength, i, 987248);
        array->SetName(name);
        delete [] name;
        data->GetPointData()->AddArray(array);
        array->Delete();
        } // end of loop over point arrays.
      // Which scalars, ... are active?
      this->Controller->Receive(activeAttributes, 5, i, 987258);
      data->GetPointData()->SetActiveAttribute(activeAttributes[0],0);
      data->GetPointData()->SetActiveAttribute(activeAttributes[1],1);
      data->GetPointData()->SetActiveAttribute(activeAttributes[2],2);
      data->GetPointData()->SetActiveAttribute(activeAttributes[3],3);
      data->GetPointData()->SetActiveAttribute(activeAttributes[4],4);
 
      // Next Cell data.
      this->Controller->Receive(&num, 1, i, 987244);
      for (j = 0; j < num; ++j)
        {
        this->Controller->Receive(&type, 1, i, 987245);
        switch (type)
          {
          case VTK_INT:
            array = vtkIntArray::New();
            break;
          case VTK_FLOAT:
            array = vtkFloatArray::New();
            break;
          case VTK_DOUBLE:
            array = vtkDoubleArray::New();
            break;
          case VTK_CHAR:
            array = vtkCharArray::New();
            break;
          case VTK_LONG:
            array = vtkLongArray::New();
            break;
          case VTK_SHORT:
            array = vtkShortArray::New();
            break;
          case VTK_UNSIGNED_CHAR:
            array = vtkUnsignedCharArray::New();
            break;
          case VTK_UNSIGNED_INT:
            array = vtkUnsignedIntArray::New();
            break;
          case VTK_UNSIGNED_LONG:
            array = vtkUnsignedLongArray::New();
            break;
          case VTK_UNSIGNED_SHORT:
            array = vtkUnsignedShortArray::New();
            break;
          }
        this->Controller->Receive(&numComps, 1, i, 987246);
        array->SetNumberOfComponents(numComps);
        this->Controller->Receive(&nameLength, 1, i, 987247);
        name = new char[nameLength];
        this->Controller->Receive(name, nameLength, i, 987248);
        array->SetName(name);
        delete [] name;
        data->GetCellData()->AddArray(array);
        array->Delete();
        } // end of loop over cell arrays.
      // Which scalars, ... are active?
      this->Controller->Receive(activeAttributes, 5, i, 987258);
      data->GetCellData()->SetActiveAttribute(activeAttributes[0],0);
      data->GetCellData()->SetActiveAttribute(activeAttributes[1],1);
      data->GetCellData()->SetActiveAttribute(activeAttributes[2],2);
      data->GetCellData()->SetActiveAttribute(activeAttributes[3],3);
      data->GetCellData()->SetActiveAttribute(activeAttributes[4],4);
      
      // We only need information from one.
      return;
      } // End of if-non-empty check.
    }// End of loop over processes.
}

//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::SendCompleteArrays(vtkDataSet *data)
{
  int nonEmptyFlag;
  int num;
  int i;
  int type;
  int numComps;
  int nameLength;
  const char *name;
  vtkDataArray *array;
  int activeAttributes[5];

  if (data == NULL ||
      (data->GetNumberOfPoints() == 0 &&
       data->GetNumberOfCells() == 0))
    {
    nonEmptyFlag = 0;
    this->Controller->Send(&nonEmptyFlag, 1, 0, 987243);
    return;
    }
  nonEmptyFlag = 1;
  this->Controller->Send(&nonEmptyFlag, 1, 0, 987243);

  // First point data.
  num = data->GetPointData()->GetNumberOfArrays();
  this->Controller->Send(&num, 1, 0, 987244);
  for (i = 0; i < num; ++i)
    {
    array = data->GetPointData()->GetArray(i);
    type = array->GetDataType();

    this->Controller->Send(&type, 1, 0, 987245);
    numComps = array->GetNumberOfComponents();

    this->Controller->Send(&numComps, 1, 0, 987246);
    name = array->GetName();
    if (name == NULL)
      {
      name = "";
      }
    nameLength = vtkString::Length(name)+1;
    this->Controller->Send(&nameLength, 1, 0, 987247);
    // I am pretty sure that Send does not modify the string.
    this->Controller->Send(const_cast<char*>(name), nameLength, 0, 987248);
    }
  data->GetPointData()->GetAttributeIndices(activeAttributes);
  this->Controller->Send(activeAttributes, 5, 0, 987258);

  // Next cell data.
  num = data->GetCellData()->GetNumberOfArrays();
  this->Controller->Send(&num, 1, 0, 987244);
  for (i = 0; i < num; ++i)
    {
    array = data->GetCellData()->GetArray(i);
    type = array->GetDataType();

    this->Controller->Send(&type, 1, 0, 987245);
    numComps = array->GetNumberOfComponents();

    this->Controller->Send(&numComps, 1, 0, 987246);
    name = array->GetName();
    if (name == NULL)
      {
      name = "";
      }
    nameLength = vtkString::Length(name+1);
    this->Controller->Send(&nameLength, 1, 0, 987247);
    this->Controller->Send(const_cast<char*>(name), nameLength, 0, 987248);
    }
  data->GetCellData()->GetAttributeIndices(activeAttributes);
  this->Controller->Send(activeAttributes, 5, 0, 987258);
}



//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::InitializePVDataPartition(vtkPVData *pvd)
{
  int numProcs, id;

  // Hard code assignment based on processes.
  numProcs = this->GetNumberOfPartitions();

  // Special debug situation. Only generate half the data.
  // This allows us to debug the parallel features of the
  // application and VTK on only one process.
  int debugNum = numProcs;
  if (getenv("PV_DEBUG_ZERO") != NULL)
    {
    this->Script("%s SetNumberOfPieces 0",pvd->GetMapperTclName());
    this->Script("%s SetPiece 0", pvd->GetMapperTclName());
    this->Script("%s SetUpdateNumberOfPieces 0",pvd->GetUpdateSuppressorTclName());
    this->Script("%s SetUpdatePiece 0", pvd->GetUpdateSuppressorTclName());
    this->Script("%s SetNumberOfPieces 0", pvd->GetLODMapperTclName());
    this->Script("%s SetPiece 0", pvd->GetLODMapperTclName());
    for (id = 1; id < numProcs; ++id)
      {
      this->RemoteScript(id, "%s SetNumberOfPieces %d",
                         pvd->GetMapperTclName(), debugNum-1);
      this->RemoteScript(id, "%s SetPiece %d", pvd->GetMapperTclName(), id-1);
      this->RemoteScript(id, "%s SetUpdateNumberOfPieces %d",
                         pvd->GetUpdateSuppressorTclName(), debugNum-1);
      this->RemoteScript(id, "%s SetUpdatePiece %d", 
                         pvd->GetUpdateSuppressorTclName(), id-1);
      this->RemoteScript(id, "%s SetNumberOfPieces %d",
                         pvd->GetLODMapperTclName(), debugNum-1);
      this->RemoteScript(id, "%s SetPiece %d", pvd->GetLODMapperTclName(), id-1);
      this->RemoteScript(id, "%s SetUpdateNumberOfPieces %d",
                         pvd->GetLODUpdateSuppressorTclName(), debugNum-1);
      this->RemoteScript(id, "%s SetUpdatePiece %d", 
                         pvd->GetLODUpdateSuppressorTclName(), id-1);
      }
    }
  else 
    {
    if (getenv("PV_DEBUG_HALF") != NULL)
      {
      debugNum *= 2;
      }
    for (id = 0; id < numProcs; ++id)
      {
      this->RemoteScript(id, "%s SetNumberOfPieces %d",
                         pvd->GetMapperTclName(), debugNum);
      this->RemoteScript(id, "%s SetPiece %d", pvd->GetMapperTclName(), id);
      this->RemoteScript(id, "%s SetUpdateNumberOfPieces %d",
                         pvd->GetUpdateSuppressorTclName(), debugNum);
      this->RemoteScript(id, "%s SetUpdatePiece %d", 
                         pvd->GetUpdateSuppressorTclName(), id);
      this->RemoteScript(id, "%s SetNumberOfPieces %d",
                         pvd->GetLODMapperTclName(), debugNum);
      this->RemoteScript(id, "%s SetPiece %d", pvd->GetLODMapperTclName(), id);
      this->RemoteScript(id, "%s SetUpdateNumberOfPieces %d",
                         pvd->GetLODUpdateSuppressorTclName(), debugNum);
      this->RemoteScript(id, "%s SetUpdatePiece %d", 
                         pvd->GetLODUpdateSuppressorTclName(), id);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVMPIProcessModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller: " << this->Controller << endl;;
}
