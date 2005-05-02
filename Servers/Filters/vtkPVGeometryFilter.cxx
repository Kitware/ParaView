/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGeometryFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVGeometryFilter.h"

#include "vtkAppendPolyData.h"
#include "vtkCTHAMRSurface.h"
#include "vtkCTHData.h"
#include "vtkCallbackCommand.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkFloatArray.h"
#include "vtkGarbageCollector.h"
#include "vtkGeometryFilter.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineSource.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridOutlineFilter.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStripper.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridOutlineFilter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkGenericDataSet.h"

vtkCxxRevisionMacro(vtkPVGeometryFilter, "1.52");
vtkStandardNewMacro(vtkPVGeometryFilter);

vtkCxxSetObjectMacro(vtkPVGeometryFilter, Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkPVGeometryFilter::vtkPVGeometryFilter ()
{
  this->OutlineFlag = 0;
  this->UseOutline = 1;
  this->UseStrips = 0;
  this->GenerateCellNormals = 1;

  this->DataSetSurfaceFilter = vtkDataSetSurfaceFilter::New();

  // Setup a callback for the internal readers to report progress.
  this->InternalProgressObserver = vtkCallbackCommand::New();
  this->InternalProgressObserver->SetCallback(
    &vtkPVGeometryFilter::InternalProgressCallbackFunction);
  this->InternalProgressObserver->SetClientData(this);

  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->OutlineSource = vtkOutlineSource::New();
}

//----------------------------------------------------------------------------
vtkPVGeometryFilter::~vtkPVGeometryFilter ()
{
  if(this->DataSetSurfaceFilter)
    {
    this->DataSetSurfaceFilter->Delete();
    }
  this->OutlineSource->Delete();
  this->InternalProgressObserver->Delete();
  this->SetController(0);
}

//----------------------------------------------------------------------------
vtkExecutive* vtkPVGeometryFilter::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::InternalProgressCallbackFunction(vtkObject*,
                                                           unsigned long,
                                                           void* clientdata,
                                                           void*)
{
  reinterpret_cast<vtkPVGeometryFilter*>(clientdata)
    ->InternalProgressCallback();
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::InternalProgressCallback()
{
  // This limits progress for only the DataSetSurfaceFilter.
  float progress = this->DataSetSurfaceFilter->GetProgress();
  this->UpdateProgress(progress);
  if (this->AbortExecute)
    {
    this->DataSetSurfaceFilter->SetAbortExecute(1);
    }
}

//----------------------------------------------------------------------------
int vtkPVGeometryFilter::CheckAttributes(vtkDataObject* input)
{
  if (input->IsA("vtkDataSet"))
    {
    if (static_cast<vtkDataSet*>(input)->CheckAttributes())
      {
      return 1;
      }
    }
  else if (input->IsA("vtkCompositeDataSet"))
    {
    vtkCompositeDataSet* compInput = 
      static_cast<vtkCompositeDataSet*>(input);
    vtkCompositeDataIterator* iter = compInput->NewIterator();
    iter->GoToFirstItem();
    while (!iter->IsDoneWithTraversal())
      {
      vtkDataObject* curDataSet = iter->GetCurrentDataObject();
      if (curDataSet && this->CheckAttributes(curDataSet))
        {
        return 1;
        }
      iter->GoToNextItem();
      }
    iter->Delete();
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVGeometryFilter::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // RequestData() synchronizes (communicates among processes), so we need
  // all procs to call RequestData().
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::ExecuteBlock(
  vtkDataObject* input, vtkPolyData* output, int doCommunicate)
{
  if (input->IsA("vtkImageData"))
    {
    this->ImageDataExecute(static_cast<vtkImageData*>(input), output, doCommunicate);
    this->ExecuteCellNormals(output, doCommunicate);
    return;
    }

  if (input->IsA("vtkStructuredGrid"))
    {
    this->StructuredGridExecute(static_cast<vtkStructuredGrid*>(input), output);
    this->ExecuteCellNormals(output, doCommunicate);
    return;
    }

  if (input->IsA("vtkRectilinearGrid"))
    {
    this->RectilinearGridExecute(static_cast<vtkRectilinearGrid*>(input),output);
    this->ExecuteCellNormals(output, doCommunicate);
    return;
    }

  if (input->IsA("vtkUnstructuredGrid"))
    {
    this->UnstructuredGridExecute(
      static_cast<vtkUnstructuredGrid*>(input), output, doCommunicate);
    this->ExecuteCellNormals(output, doCommunicate);
    return;
    }

  if (input->IsA("vtkPolyData"))
    {
    this->PolyDataExecute(static_cast<vtkPolyData*>(input), output, doCommunicate);
    this->ExecuteCellNormals(output, doCommunicate);
    return;
    }
  if (input->IsA("vtkCTHData"))
    {
    this->CTHDataExecute(static_cast<vtkCTHData*>(input), output, doCommunicate);
    this->ExecuteCellNormals(output, doCommunicate);
    return;
    }
  if (input->IsA("vtkDataSet"))
    {
    this->DataSetExecute(static_cast<vtkDataSet*>(input), output, doCommunicate);
    this->ExecuteCellNormals(output, doCommunicate);
    return;
    }
  if (input->IsA("vtkGenericDataSet"))
    {
    this->GenericDataSetExecute(static_cast<vtkGenericDataSet*>(input), output, doCommunicate);
    this->ExecuteCellNormals(output, doCommunicate);
    return;
    }
}

//----------------------------------------------------------------------------
int vtkPVGeometryFilter::RequestData(vtkInformation* request,
                                     vtkInformationVector** inputVector,
                                     vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  vtkCompositeDataSet *hdInput = vtkCompositeDataSet::SafeDownCast(
    inInfo->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET()));
  if (hdInput) 
    {
    return this->RequestCompositeData(request, inputVector, outputVector);
    }

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));
  if (!output) {return 0;}

  vtkDataObject *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input)
    {
    input = vtkGenericDataSet::SafeDownCast(
      inInfo->Get(vtkDataObject::DATA_OBJECT()));
    if (!input) {return 0;}
    }
  
  this->ExecuteBlock(input, output, 1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkPVGeometryFilter::RequestCompositeData(vtkInformation*,
                                              vtkInformationVector** inputVector,
                                              vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));
  if (!output) {return 0;}

  vtkCompositeDataSet *hdInput = vtkCompositeDataSet::SafeDownCast(
    inInfo->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET()));
  if (!hdInput) {return 0;}


  if (this->CheckAttributes(hdInput))
    {
    return 0;
    }

  vtkCompositeDataIterator* iter = hdInput->NewIterator();
  iter->GoToFirstItem();
  vtkAppendPolyData* append = vtkAppendPolyData::New();
  int numInputs = 0;
  while (!iter->IsDoneWithTraversal())
    {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (ds)
      {
      vtkPolyData* tmpOut = vtkPolyData::New();
      this->ExecuteBlock(ds, tmpOut, 0);
      append->AddInput(tmpOut);
      // Call FastDelete() instead of Delete() to avoid garbage
      // collection checks. This improves the preformance significantly
      tmpOut->FastDelete();
      numInputs++;
      }
    iter->GoToNextItem();
    }
  iter->Delete();
  if (numInputs > 0)
    {
    append->Update();
    }

  output->ShallowCopy(append->GetOutput());

  append->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// We need to change the mapper.  Now it always flat shades when cell normals
// are available.
void vtkPVGeometryFilter::ExecuteCellNormals(vtkPolyData* output, int doCommunicate)
{
  if ( ! this->GenerateCellNormals)
    {
    return;
    }

  // Do not generate cell normals if any of the processes
  // have lines, verts or strips.
  vtkCellArray* aPrim;
  int skip = 0;
  aPrim = output->GetVerts();
  if (aPrim && aPrim->GetNumberOfCells())
    {
    skip = 1;
    }
  aPrim = output->GetLines();
  if (aPrim && aPrim->GetNumberOfCells())
    {
    skip = 1;
    }
  aPrim = output->GetStrips();
  if (aPrim && aPrim->GetNumberOfCells())
    {
    skip = 1;
    }
  if( this->Controller && doCommunicate )
    {
    // An MPI gather or reduce would be easier ...
    if (this->Controller->GetLocalProcessId() == 0)  
      {
      int tmp, idx;
      for (idx = 1; idx < this->Controller->GetNumberOfProcesses(); ++idx)
        {
        this->Controller->Receive(&tmp, 1, idx, 89743);
        if (tmp)
          {
          skip = 1;
          }
        }
      for (idx = 1; idx < this->Controller->GetNumberOfProcesses(); ++idx)
        {
        this->Controller->Send(&skip, 1, idx, 89744);
        }
      }
    else
      {
      this->Controller->Send(&skip, 1, 0, 89743);
      this->Controller->Receive(&skip, 1, 0, 89744);
      }
    }
  if (skip)
    {
    return;
    }

  vtkIdType* endCellPtr;
  vtkIdType* cellPtr;
  vtkIdType *pts = 0;
  vtkIdType npts = 0;
  double polyNorm[3];
  vtkFloatArray* cellNormals = vtkFloatArray::New();
  cellNormals->SetName("cellNormals");
  cellNormals->SetNumberOfComponents(3);
  cellNormals->Allocate(3*output->GetNumberOfCells());

  aPrim = output->GetPolys();
  if (aPrim && aPrim->GetNumberOfCells())
    {
    vtkPoints* p = output->GetPoints();

    cellPtr = aPrim->GetPointer();
    endCellPtr = cellPtr+aPrim->GetNumberOfConnectivityEntries();

    while (cellPtr < endCellPtr)
      {
      npts = *cellPtr++;
      pts = cellPtr;
      cellPtr += npts;

      vtkPolygon::ComputeNormal(p,npts,pts,polyNorm);
      cellNormals->InsertNextTuple(polyNorm);    
      }
    }

  if (cellNormals->GetNumberOfTuples() != output->GetNumberOfCells())
    {
    vtkErrorMacro("Number of cell normals does not match output.");
    cellNormals->Delete();
    return;
    }

  output->GetCellData()->AddArray(cellNormals);
  output->GetCellData()->SetActiveNormals(cellNormals->GetName());
  cellNormals->Delete();
  cellNormals = NULL;
}


//----------------------------------------------------------------------------
void vtkPVGeometryFilter::DataSetExecute(
  vtkDataSet* input, vtkPolyData* output, int doCommunicate)
{
  double bds[6];
  int procid = 0;
  int numProcs = 1;

  if (!doCommunicate && input->GetNumberOfPoints() == 0)
    {
    return;
    }

  if (this->Controller )
    {
    procid = this->Controller->GetLocalProcessId();
    numProcs = this->Controller->GetNumberOfProcesses();
    }

  input->GetBounds(bds);

  if ( procid && doCommunicate )
    {
    // Satellite node
    this->Controller->Send(bds, 6, 0, 792390);
    }
  else
    {
    int idx;
    double tmp[6];
    
    if (doCommunicate)
      {
      for (idx = 1; idx < numProcs; ++idx)
        {
        this->Controller->Receive(tmp, 6, idx, 792390);
        if (tmp[0] < bds[0])
          {
          bds[0] = tmp[0];
          }
        if (tmp[1] > bds[1])
          {
          bds[1] = tmp[1];
          }
        if (tmp[2] < bds[2])
          {
          bds[2] = tmp[2];
          }
        if (tmp[3] > bds[3])
          {
          bds[3] = tmp[3];
          }
        if (tmp[4] < bds[4])
          {
          bds[4] = tmp[4];
          }
        if (tmp[5] > bds[5])
          {
          bds[5] = tmp[5];
          }
        }
      }

    // only output in process 0.
    this->OutlineSource->SetBounds(bds);
    this->OutlineSource->Update();
    
    output->SetPoints(this->OutlineSource->GetOutput()->GetPoints());
    output->SetLines(this->OutlineSource->GetOutput()->GetLines());
    }
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::GenericDataSetExecute(
  vtkGenericDataSet* input, vtkPolyData* output, int doCommunicate)
{
  double bds[6];
  int procid = 0;
  int numProcs = 1;

  if (!doCommunicate && input->GetNumberOfPoints() == 0)
    {
    return;
    }

  if (this->Controller )
    {
    procid = this->Controller->GetLocalProcessId();
    numProcs = this->Controller->GetNumberOfProcesses();
    }

  input->GetBounds(bds);

  if ( procid && doCommunicate )
    {
    // Satellite node
    this->Controller->Send(bds, 6, 0, 792390);
    }
  else
    {
    int idx;
    double tmp[6];
    
    if (doCommunicate)
      {
      for (idx = 1; idx < numProcs; ++idx)
        {
        this->Controller->Receive(tmp, 6, idx, 792390);
        if (tmp[0] < bds[0])
          {
          bds[0] = tmp[0];
          }
        if (tmp[1] > bds[1])
          {
          bds[1] = tmp[1];
          }
        if (tmp[2] < bds[2])
          {
          bds[2] = tmp[2];
          }
        if (tmp[3] > bds[3])
          {
          bds[3] = tmp[3];
          }
        if (tmp[4] < bds[4])
          {
          bds[4] = tmp[4];
          }
        if (tmp[5] > bds[5])
          {
          bds[5] = tmp[5];
          }
        }
      }

    // only output in process 0.
    this->OutlineSource->SetBounds(bds);
    this->OutlineSource->Update();
    
    output->SetPoints(this->OutlineSource->GetOutput()->GetPoints());
    output->SetLines(this->OutlineSource->GetOutput()->GetLines());
    }
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::DataSetSurfaceExecute(vtkDataSet* input, 
                                                vtkPolyData* output)
{
  vtkDataSet* ds = input->NewInstance();
  ds->ShallowCopy(input);
  this->DataSetSurfaceFilter->SetInput(ds);
  ds->Delete();


  // Observe the progress of the internal filter.
  this->DataSetSurfaceFilter->AddObserver(vtkCommand::ProgressEvent, 
                                          this->InternalProgressObserver);
  this->DataSetSurfaceFilter->Update();
  // The internal filter is finished.  Remove the observer.
  this->DataSetSurfaceFilter->RemoveObserver(this->InternalProgressObserver);

  output->ShallowCopy(this->DataSetSurfaceFilter->GetOutput());
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::ImageDataExecute(vtkImageData *input,
                                           vtkPolyData* output,
                                           int doCommunicate)
{
  double *spacing;
  double *origin;
  int *ext;
  double bounds[6];

  // If doCommunicate is false, use extent because the block is
  // entirely contained in this process.
  if (doCommunicate)
    {
    ext = input->GetWholeExtent();
    }
  else
    {
    ext = input->GetExtent();
    }

  // If 2d then default to superclass behavior.
//  if (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5] ||
//      !this->UseOutline)
  if (!this->UseOutline)
    {
    this->DataSetSurfaceExecute(input, output);
    this->OutlineFlag = 0;
    return;
    }  
  this->OutlineFlag = 1;

  //
  // Otherwise, let OutlineSource do all the work
  //
  
  if (output->GetUpdatePiece() == 0 || !doCommunicate)
    {
    spacing = input->GetSpacing();
    origin = input->GetOrigin();
    
    bounds[0] = spacing[0] * ((float)ext[0]) + origin[0];
    bounds[1] = spacing[0] * ((float)ext[1]) + origin[0];
    bounds[2] = spacing[1] * ((float)ext[2]) + origin[1];
    bounds[3] = spacing[1] * ((float)ext[3]) + origin[1];
    bounds[4] = spacing[2] * ((float)ext[4]) + origin[2];
    bounds[5] = spacing[2] * ((float)ext[5]) + origin[2];

    vtkOutlineSource *outline = vtkOutlineSource::New();
    outline->SetBounds(bounds);
    outline->Update();

    output->SetPoints(outline->GetOutput()->GetPoints());
    output->SetLines(outline->GetOutput()->GetLines());
    outline->Delete();
    }
  else
    {
    vtkPoints* pts = vtkPoints::New();
    output->SetPoints(pts);
    pts->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::StructuredGridExecute(vtkStructuredGrid* input,
                                                vtkPolyData* output)
{
  int *ext;

  ext = input->GetWholeExtent();

  // If 2d then default to superclass behavior.
//  if (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5] ||
//      !this->UseOutline)
  if (!this->UseOutline)
    {
    this->DataSetSurfaceExecute(input, output);
    this->OutlineFlag = 0;
    return;
    }  
  this->OutlineFlag = 1;

  //
  // Otherwise, let Outline do all the work
  //
  

  vtkStructuredGridOutlineFilter *outline = vtkStructuredGridOutlineFilter::New();
  // Because of streaming, it is important to set the input and not copy it.
  outline->SetInput(input);
  outline->GetOutput()->SetUpdateNumberOfPieces(output->GetUpdateNumberOfPieces());
  outline->GetOutput()->SetUpdatePiece(output->GetUpdatePiece());
  outline->GetOutput()->SetUpdateGhostLevel(output->GetUpdateGhostLevel());
  outline->GetOutput()->Update();

  output->CopyStructure(outline->GetOutput());
  outline->Delete();
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::RectilinearGridExecute(vtkRectilinearGrid* input,
                                                 vtkPolyData* output)
{
  int *ext;

  ext = input->GetWholeExtent();

  // If 2d then default to superclass behavior.
//  if (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5] ||
//      !this->UseOutline)
  if (!this->UseOutline)
    {
    this->DataSetSurfaceExecute(input, output);
    this->OutlineFlag = 0;
    return;
    }  
  this->OutlineFlag = 1;

  //
  // Otherwise, let Outline do all the work
  //

  vtkRectilinearGridOutlineFilter *outline = vtkRectilinearGridOutlineFilter::New();
  // Because of streaming, it is important to set the input and not copy it.
  outline->SetInput(input);
  outline->GetOutput()->SetUpdateNumberOfPieces(output->GetUpdateNumberOfPieces());
  outline->GetOutput()->SetUpdatePiece(output->GetUpdatePiece());
  outline->GetOutput()->SetUpdateGhostLevel(output->GetUpdateGhostLevel());
  outline->GetOutput()->Update();

  output->CopyStructure(outline->GetOutput());
  outline->Delete();
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::CTHDataExecute(
  vtkCTHData *input, vtkPolyData* output, int doCommunicate)
{
  if (!this->UseOutline)
    {
    vtkCTHData* inCopy = vtkCTHData::New();
    inCopy->ShallowCopy(input);
    vtkCTHAMRSurface *surface = vtkCTHAMRSurface::New();
    surface->SetInput(inCopy);
    surface->Update();
    output->CopyStructure(surface->GetOutput());
    output->GetCellData()->PassData(surface->GetOutput()->GetCellData());
    output->GetPointData()->PassData(surface->GetOutput()->GetPointData());
    surface->SetInput(0);
    surface->Delete();
    surface = 0;
    inCopy->Delete();
    inCopy = 0;
    this->OutlineFlag = 0;
    return;
    }  
  this->OutlineFlag = 1;

  //
  // Otherwise, let Outline do all the work
  //
  this->DataSetExecute(input, output, doCommunicate);
}


//----------------------------------------------------------------------------
void vtkPVGeometryFilter::UnstructuredGridExecute(
  vtkUnstructuredGrid* input, vtkPolyData* output, int doCommunicate)
{
  if (!this->UseOutline)
    {
    this->OutlineFlag = 0;
    this->DataSetSurfaceExecute(input, output);
    return;
    }
  
  this->OutlineFlag = 1;

  this->DataSetExecute(input, output, doCommunicate);
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::PolyDataExecute(
  vtkPolyData* input, vtkPolyData* out, int doCommunicate)
{
  if (!this->UseOutline)
    {
    this->OutlineFlag = 0;
    if (this->UseStrips)
      {
      vtkPolyData *inCopy = vtkPolyData::New();
      vtkStripper *stripper = vtkStripper::New();
      inCopy->ShallowCopy(input);
      inCopy->RemoveGhostCells(1);
      stripper->SetInput(inCopy);
      stripper->Update();
      out->CopyStructure(stripper->GetOutput());
      out->GetPointData()->ShallowCopy(stripper->GetOutput()->GetPointData());
      out->GetCellData()->ShallowCopy(stripper->GetOutput()->GetCellData());
      inCopy->Delete();
      stripper->Delete();
      }
    else
      {
      out->ShallowCopy(input);
      out->RemoveGhostCells(1);
      }
    return;
    }
  
  this->OutlineFlag = 1;
  this->DataSetExecute(input, out, doCommunicate);
}

//----------------------------------------------------------------------------
int vtkPVGeometryFilter::FillInputPortInformation(int port,
                                                  vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  info->Set(vtkCompositeDataPipeline::INPUT_REQUIRED_COMPOSITE_DATA_TYPE(), 
            "vtkCompositeDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
void vtkPVGeometryFilter::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->DataSetSurfaceFilter,
                            "DataSetSurfaceFilter");
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->OutlineFlag)
    {
    os << indent << "OutlineFlag: On\n";
    }
  else
    {
    os << indent << "OutlineFlag: Off\n";
    }
  
  os << indent << "UseOutline: " << (this->UseOutline?"on":"off") << endl;
  os << indent << "UseStrips: " << (this->UseStrips?"on":"off") << endl;
  os << indent << "GenerateCellNormals: " << (this->GenerateCellNormals?"on":"off") << endl;
  os << indent << "Controller: " << this->Controller << endl;
}
