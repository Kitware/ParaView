/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkAcuSolveReaderProxy.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAcuSolveReaderProxy.h"

#include "vtkAcuSolveReaderBase.h"
#include "vtkCallbackCommand.h"
#include "vtkCellArray.h"
#include "vtkCellType.h"
#include "vtkCharArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkInstantiator.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include <sys/stat.h>

/*=======================================================================
  Include AcuSolve related files
  =======================================================================*/

vtkCxxRevisionMacro(vtkAcuSolveReaderProxy, "1.1.2.2");
vtkStandardNewMacro(vtkAcuSolveReaderProxy);

//----------------------------------------------------------------------------
vtkAcuSolveReaderProxy::vtkAcuSolveReaderProxy()
{
  this->SetNumberOfInputPorts(0);

  char* ACUSIM_PARAVIEW_LIB;
  char adbLibPath[1024];
  CreateFptr* createAcuSolveReader;
  this->adbLoader       = vtkDynamicLoader::New();
  this->FileName        = 0;
  this->NumberOfSets    = 0;
  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->InformationError        = 0;
  this->ReadGeometryFlag        = 0;
  this->DataError               = 0;
  this->ProgressRange[0]        = 0;
  this->ProgressRange[1]        = 1;
    
  /*-----------------------------------------------------------------
   * Setup the selection callback to modify this object when an array
   * selection is changed.
   *-----------------------------------------------------------------
   */
     
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(
    &vtkAcuSolveReaderProxy::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->PointDataArraySelection->AddObserver(vtkCommand::ModifiedEvent,
                                             this->SelectionObserver);

  //this->NumberOfTimeSteps = this->UpdateTimeStep = 0;
  this->NumberOfTimeSteps = this->TimeStep = 0;
  this->PrevTimeStep = 0;
  this->AleFlag = false;
  this->Cache = 0;
  this->CacheFileName = 0;

  this->TimeStepRange[0] = 0 ;
  this->TimeStepRange[1] = 0 ;
    
  ACUSIM_PARAVIEW_LIB = getenv("ACUSIM_PARAVIEW_LIB");
    
  if(ACUSIM_PARAVIEW_LIB == NULL ) 
    {
    vtkErrorMacro("ACUSIM: Could not find ACUSIM_PARAVIEW_LIB..");
    vtkErrorMacro("ACUSIM: setenv ACUSIM_PARAVIEW_LIB to libvtkAcuSolveReader.so");
    this->AdbReader = NULL;
    } 
  else 
    {
    sprintf(adbLibPath,"%s",ACUSIM_PARAVIEW_LIB);
    this->adbLibHandle = this->adbLoader->OpenLibrary(adbLibPath);
    if(this->adbLibHandle == NULL) 
      {
      this->AdbReader = NULL;
      vtkErrorMacro("ACUSIM: Error Loading AcuSolve Libraries..");
      vtkErrorMacro(<< " Library: " 
                    << adbLibPath 
                    <<  this->adbLoader->LastError());
      } 
    else 
      {
      createAcuSolveReader = (CreateFptr*) (this->adbLoader->GetSymbolAddress(this->adbLibHandle,"Create") );
      
      if(createAcuSolveReader != NULL) 
        {
        this->AdbReader = createAcuSolveReader();
        } 
      else 
        {
        this->AdbReader = NULL;
        }
      }
    }

} /* End of  vtkPvAcuSolveReaderModule() */

//----------------------------------------------------------------------------
vtkAcuSolveReaderProxy::~vtkAcuSolveReaderProxy()
{
  this->SetFileName(0);
  this->PointDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->SelectionObserver->Delete();
  this->PointDataArraySelection->Delete();
  this->SelectionObserver = 0;
  this->PointDataArraySelection = 0;
  this->DeleteCache();
  this->SetCacheFileName(0);
} /* End of ~vtkAcuSolveReaderProxy */

//----------------------------------------------------------------------------
void vtkAcuSolveReaderProxy::PrintSelf( ostream&  os,
                                           vtkIndent indent )
{
  this->Superclass::PrintSelf(os, indent);
  os << indent 
     << "PointDataArraySelection: " 
     << this->PointDataArraySelection
     << "\n";

  os << indent
     << "NumberOfTimeSteps: " 
     << this->NumberOfTimeSteps 
     << endl;

  //os << indent << "UpdateTimeStep: " << this->UpdateTimeStep << endl;
  os << indent 
     << "TimeStep: " 
     << this->TimeStep 
     << endl;

  os << indent
     << "FileName: "
     << (this->FileName?this->FileName:"(null)")
     << endl;

  os << indent
     << "TimeStepRange: "
     << this->TimeStepRange[0] << " " << this->TimeStepRange[1]
     << endl;
}


void vtkAcuSolveReaderProxy::ReadGeometry()
{
  int                   i;

  if(this->AdbReader == NULL) 
    {
    vtkErrorMacro("ACUSIM: Error Loading Acusolve Libraries Aborting....");
    vtkErrorMacro("ACUSIM: To Get libvtkAcuSolveReader.so, contact");
    vtkErrorMacro(" ...............support@acusim.com ..............");
    return;
    }
    
  this->AdbReader->ReadData(this->FileName);
  this->NumberOfSets = this->AdbReader->GetNumberOfCellSets();
  if (this->NumberOfSets < 0) 
    {
    vtkErrorMacro("ACUSIM: Error reading AcuSolve database.NumberOfSets < 0 ");
    this->InformationError = 1;
    return;
    }

  this->DeleteCache();
  this->Cache = new vtkUnstructuredGrid*[this->NumberOfSets];

  for (i = 0; i < this->NumberOfSets; ++i) 
    {
    this->Cache[i] = 0;
    }

  this->ReadGeometryFlag = 1; 
} /* End of ReadGeometry() */

//----------------------------------------------------------------------------
int vtkAcuSolveReaderProxy::RequestInformation(
  vtkInformation*, 
  vtkInformationVector**, 
  vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  if (info)
    {
    info->Set(
      vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), 1);
    }

  char  nodalOutputName[1024];
  int   i;
  int   numberOfTimeSteps = 0 ; 
  int   numberOfNodalData;
 
  if ( this->AdbReader == NULL ) 
    {
    return 0;
    }

  if (this->ReadGeometryFlag == 0) 
    {
    this->ReadGeometry();
    }

  this->AleFlag = this->AdbReader->GetAleFlag();
  // Now take care of other light weight information.
  numberOfTimeSteps = this->AdbReader->GetNumberOfTimeSteps();

  this->NumberOfTimeSteps = numberOfTimeSteps;
  this->SetTimeStepRange( 0, numberOfTimeSteps - 1 ) ;
    
  // if (this->UpdateTimeStep >= numberOfTimeSteps) {
  //  this->UpdateTimeStep = numberOfTimeSteps - 1;
  // }
  if (this->TimeStep >= numberOfTimeSteps) 
    {
    this->TimeStep = numberOfTimeSteps - 1;
    }

  // Read information about the available attribute arrays.
  // Change this: Get the actual array names from the file/database.
  vtkDataArraySelection* tmp = vtkDataArraySelection::New();
  tmp->CopySelections(this->PointDataArraySelection);
  this->PointDataArraySelection->RemoveAllArrays();
    
  numberOfNodalData = this->AdbReader->GetNumberOfNodalData();
    
  for(i=0;i<numberOfNodalData;i++) 
    {
    sprintf(nodalOutputName, "%s", 
            (this->AdbReader)->GetNodalOutputName(i)      );
    this->PointDataArraySelection->AddArray(nodalOutputName);
    
    if( ( tmp->ArrayExists(nodalOutputName) ) && 
      (! tmp->ArrayIsEnabled(nodalOutputName)) ) 
      {
      this->PointDataArraySelection->DisableArray(nodalOutputName);
      }
    }
    
  tmp->Delete();
  tmp = NULL;

  return 1;
  // We do not want to stomp on user set values.
  // The selection object does not have the correct method.
  // ArrayIsEnabled returns 0 if the array has not been added,
  // and CopySelections first removes all arrays.
  // Since arrays default to enabled when first added,
  // we need to see if the user has disabled an array
  // and then copy that user change. 
} /* End of ExecuteInformation */

//----------------------------------------------------------------------------
// This method is called for each output.  It reads the heavy output.
int vtkAcuSolveReaderProxy::RequestData(vtkInformation*, 
                                        vtkInformationVector**, 
                                        vtkInformationVector* outputVector)
{
  vtkMultiBlockDataSet* output = 
    vtkMultiBlockDataSet::GetData(outputVector, 0);

  // Don't bother reading.  We could not even get the information.
  if(this->InformationError || this->AdbReader == NULL ) 
    {
    return 0;
    }
    
  this->DataError = 0;
  //if(this->UpdateTimeStep < 0)
  if(this->TimeStep < 0) 
    {
    this->DataError = 1;
    return 0;
    }
    
  // We are just starting to read.  Do not call UpdateProgressDiscrete
  // because we want a 0 progress callback the first time.
  this->UpdateProgress(0);
    
  // Initialize progress range to entire 0..1 range.
  float wholeProgressRange[2] = {0,1};
  this->SetProgressRange(wholeProgressRange, 0, 1);
    
  for (int i = 0; i < this->NumberOfSets; ++i) 
    {
    vtkUnstructuredGrid* block = this->ReadSet(i);
    output->SetDataSet(i, 0, block);
    block->Delete();
    }
    
  // We have finished reading.
  this->UpdateProgressDiscrete(1);

  return 1;
} /* End of ExecuteData() */

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkAcuSolveReaderProxy::ReadSet(int set)
{
  char  name[1024];
  char  nodalOutputName[1024];
  char* copy;
  int           cellType;
  int           numComponents;
  int           numberOfNodalOutputDataSets;
  size_t        len;
  vtkDoubleArray* nodalDataArray;
  vtkIdType     i;
  vtkIdType     numPoints = 0;
  vtkUnstructuredGrid*  output = vtkUnstructuredGrid::New();
  vtkIdType     numCells;
  vtkCharArray* nameArray = vtkCharArray::New();
  nameArray->SetName("Name");
    
  if((this->AdbReader)->GetCellSetName(set) != NULL) 
    {
    sprintf(name, "%s", (this->AdbReader)->GetCellSetName(set));
    } 
  else 
    {
    sprintf(name, "Set-%d", set);
    }
    
  len = strlen(name);
  nameArray->SetNumberOfTuples(static_cast<vtkIdType>(len)+1);
  copy = nameArray->GetPointer(0);
  memcpy(copy, name, len);
  copy[len] = '\0';
  output->GetFieldData()->AddArray(nameArray);
  nameArray->Delete();
    
  // Here we are assuming that the connectivity and points are the same
  // for all time steps.
  if( this->Cache && this->Cache[set] && 
      this->CacheFileName && this->FileName &&
      strcmp(this->FileName,this->CacheFileName) == 0 ) 
    {
    // cache exists copy connectivity and points.
    // Shallow copy points and connectivity.
    output->CopyStructure(this->Cache[set]);
    } 
  else 
    {
    // read points and connectivity.
    // Save points and connectivity in cache. (shallow/by reference).
    numPoints = (this->AdbReader)->GetNumberOfPoints() ;
    numCells = (this->AdbReader)->GetNumberOfCells(set) ;
    output->Allocate(numCells);
    cellType = (this->AdbReader)->GetCellType(set);
    output->SetCells(cellType,(this->AdbReader)->GetCells(set));
    output->SetPoints( (this->AdbReader)->GetPoints(0));
    
    if (this->Cache) 
      {
      if (this->Cache[set] == NULL) 
        {
        this->Cache[set] = vtkUnstructuredGrid::New();
        }
      this->Cache[set]->CopyStructure(output);
      this->SetCacheFileName(this->FileName);
      }
    }
    
  //if ( (this->AleFlag) && (PrevTimeStep != (this->UpdateTimeStep) ) ) {
  //    output->SetPoints((this->AdbReader)->GetPoints(this->UpdateTimeStep));
  //    PrevTimeStep = this->UpdateTimeStep;
  // }

  if ( (this->AleFlag) && (PrevTimeStep != (this->TimeStep) ) ) 
    {
    output->SetPoints( (this->AdbReader)->GetPoints(this->TimeStep));
    PrevTimeStep = this->TimeStep;
    }
    
  numPoints = output->GetNumberOfPoints();
  numberOfNodalOutputDataSets = this->AdbReader->GetNumberOfNodalData();
    
  for(i=0;i<numberOfNodalOutputDataSets;i++) 
    {
    sprintf(nodalOutputName, "%s",(this->AdbReader)->GetNodalOutputName(i));
    
    if (this->GetPointArrayStatus(nodalOutputName)) 
      {
      nodalDataArray = vtkDoubleArray::New();
      nodalDataArray->SetName(nodalOutputName);
      numComponents = this->AdbReader->GetNodalOutputDimension(i);
      nodalDataArray->SetNumberOfComponents(numComponents);
      nodalDataArray->SetNumberOfTuples(numPoints);
      //nodalDataArray->SetArray((double *)(this->AdbReader->GetNodalData(i, this->UpdateTimeStep)),    numPoints* numComponents, 1 ) ;
      nodalDataArray->SetArray((double *)(this->AdbReader->GetNodalData(i, this->TimeStep)),    numPoints* numComponents, 1 ) ;
      output->GetPointData()->AddArray(nodalDataArray);
      nodalDataArray->Delete();
      nodalDataArray = 0;
      if( (numComponents == 1)  || 
          (strstr(nodalOutputName,"pressure") != NULL)) 
        {
        output->GetPointData()->SetActiveScalars(nodalOutputName);
        }

  // The code below caused some problems
  //if( (numComponents == 3)  || (strstr(nodalOutputName,"velocity") != NULL))
  //{
  //    output->GetPointData()->SetActiveNormals(nodalOutputName);
  //}
      }
    }
    
  // Reclaim extra memory allocated.  This reallocates memory and copies.
  // It is not be necessary if you allocate arrays accurately.
  output->Squeeze();

  return output;
} /* End of ReadSet */

//----------------------------------------------------------------------------
void vtkAcuSolveReaderProxy::DeleteCache()
{
  if (this->Cache == 0) {
  return;
  }

  for (int i = 0; i < this->NumberOfSets; ++i) {
  if (this->Cache[i]) {
  this->Cache[i]->Delete();
  this->Cache[i]=0;
  }
  }
  delete [] this->Cache;
  this->Cache = 0;
} /* End of DeleteCache */

//----------------------------------------------------------------------------
int vtkAcuSolveReaderProxy::CanReadFile(const char* name)
{
  // First make sure the file exists.  This prevents an empty file
  // from being created on older compilers.
  struct stat fs;
    
  if(stat(name, &fs) != 0) 
    {
    return 0;
    }
    
  if(this->AdbReader == NULL) 
    {
    return 0;
    }
  return 1;
} /* End of CanReadFile */

//----------------------------------------------------------------------------
void vtkAcuSolveReaderProxy::SelectionModifiedCallback(vtkObject*, 
                                                          unsigned long,
                                                          void* clientdata, 
                                                          void* )
{
  static_cast<vtkAcuSolveReaderProxy*>(clientdata)->Modified();
} /* End of SelectionModifiedCallback */

//----------------------------------------------------------------------------
int vtkAcuSolveReaderProxy::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
} /* End of GetNumberOfPointArrays */

//----------------------------------------------------------------------------
const char* vtkAcuSolveReaderProxy::GetPointArrayName(int index)
{
  return this->PointDataArraySelection->GetArrayName(index);
} /* End of GetPointArrayName */

//----------------------------------------------------------------------------
int vtkAcuSolveReaderProxy::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
} /* End of GetPointArrayStatus */

//----------------------------------------------------------------------------
void vtkAcuSolveReaderProxy::SetPointArrayStatus(const char* name, int status)
{
  if(status) 
    {
    this->PointDataArraySelection->EnableArray(name);
    } 
  else 
    {
    this->PointDataArraySelection->DisableArray(name);
    }
} /* End of SetPointArrayStatus */

//----------------------------------------------------------------------------
void vtkAcuSolveReaderProxy::GetProgressRange(float* range)
{
  range[0] = this->ProgressRange[0];
  range[1] = this->ProgressRange[1];
}

//----------------------------------------------------------------------------
void vtkAcuSolveReaderProxy::SetProgressRange(float* range,
                                                 int    curStep,
                                                 int    numSteps        )
{
  float stepSize = (range[1] - range[0])/numSteps;
  this->ProgressRange[0] = range[0] + stepSize*curStep;
  this->ProgressRange[1] = range[0] + stepSize*(curStep+1);
  this->UpdateProgressDiscrete(this->ProgressRange[0]);
}

//----------------------------------------------------------------------------
void vtkAcuSolveReaderProxy::SetProgressRange(float* range,
                                                 int    curStep,
                                                 float* fractions       )
{
  float width = range[1] - range[0];
  this->ProgressRange[0] = range[0] + fractions[curStep]*width;
  this->ProgressRange[1] = range[0] + fractions[curStep+1]*width;
  this->UpdateProgressDiscrete(this->ProgressRange[0]);
}

//----------------------------------------------------------------------------
void vtkAcuSolveReaderProxy::UpdateProgressDiscrete(float progress)
{
  if(!this->AbortExecute) 
    {
    // Round progress to nearest 100th.
    float rounded = float(int((progress*100)+0.5))/100;
    if(this->GetProgress() != rounded) 
      {
      this->UpdateProgress(rounded);
      }
    }
}
