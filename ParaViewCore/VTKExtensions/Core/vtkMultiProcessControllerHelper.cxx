/*=========================================================================

  Program:   ParaView
  Module:    vtkMultiProcessControllerHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiProcessControllerHelper.h"

#include "vtkAppendCompositeDataLeaves.h"
#include "vtkAppendFilter.h"
#include "vtkAppendPolyData.h"
#include "vtkCompositeDataSet.h"
#include "vtkGraph.h"
#include "vtkImageAppend.h"
#include "vtkImageData.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGridAppend.h"
#include "vtkTrivialProducer.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkMultiProcessControllerHelper);
//----------------------------------------------------------------------------
vtkMultiProcessControllerHelper::vtkMultiProcessControllerHelper()
{
}

//----------------------------------------------------------------------------
vtkMultiProcessControllerHelper::~vtkMultiProcessControllerHelper()
{
}

//----------------------------------------------------------------------------
int vtkMultiProcessControllerHelper::ReduceToAll(vtkMultiProcessController* controller,
  vtkMultiProcessStream& data,
  void (*operation)(vtkMultiProcessStream& A, vtkMultiProcessStream& B), int tag)
{
  int myid = controller->GetLocalProcessId();
  int numProcs = controller->GetNumberOfProcesses();
  int children[2] = { 2 * myid + 1, 2 * myid + 2 };
  int parent = myid > 0 ? (myid - 1) / 2 : -1;
  int childno = 0;

  for (childno = 0; childno < 2; childno++)
  {
    int childid = children[childno];
    if (childid >= numProcs)
    {
      // skip nonexistent children.
      continue;
    }

    vtkMultiProcessStream child_stream;
    controller->Receive(child_stream, childid, tag);
    (*operation)(child_stream, data);
  }

  if (parent >= 0)
  {
    controller->Send(data, parent, tag);
    data.Reset();
    controller->Receive(data, parent, tag);
  }

  for (childno = 0; childno < 2; childno++)
  {
    int childid = children[childno];
    if (childid >= numProcs)
    {
      // skip nonexistent children.
      continue;
    }
    controller->Send(data, childid, tag);
  }
  return 1;
}

//-----------------------------------------------------------------------------
vtkDataObject* vtkMultiProcessControllerHelper::MergePieces(
  vtkDataObject** pieces, unsigned int num_pieces)
{
  if (num_pieces == 0)
  {
    return NULL;
  }

  vtkDataObject* result = pieces[0]->NewInstance();

  std::vector<vtkSmartPointer<vtkDataObject> > piece_vector;
  piece_vector.resize(num_pieces);
  for (unsigned int cc = 0; cc < num_pieces; cc++)
  {
    piece_vector[cc] = pieces[cc];
  }

  if (vtkMultiProcessControllerHelper::MergePieces(piece_vector, result))
  {
    return result;
  }
  result->Delete();
  return NULL;
}

//-----------------------------------------------------------------------------
bool vtkMultiProcessControllerHelper::MergePieces(
  std::vector<vtkSmartPointer<vtkDataObject> >& pieces, vtkDataObject* result)
{
  if (pieces.size() == 0)
  {
    return false;
  }

  if (pieces.size() == 1)
  {
    result->ShallowCopy(pieces[0]);
    vtkImageData* id = vtkImageData::SafeDownCast(pieces[0]);
    if (id)
    {
      vtkStreamingDemandDrivenPipeline::SetWholeExtent(
        result->GetInformation(), static_cast<vtkImageData*>(pieces[0].GetPointer())->GetExtent());
    }
    return true;
  }

  // PolyData and Unstructured grid need different append filters.
  vtkAlgorithm* appender = NULL;
  if (vtkPolyData::SafeDownCast(result))
  {
    appender = vtkAppendPolyData::New();
  }
  else if (vtkUnstructuredGrid::SafeDownCast(result))
  {
    appender = vtkAppendFilter::New();
  }
  else if (vtkImageData::SafeDownCast(result))
  {
    vtkImageAppend* ia = vtkImageAppend::New();
    ia->PreserveExtentsOn();
    appender = ia;
  }
  else if (vtkStructuredGrid::SafeDownCast(result))
  {
    appender = vtkStructuredGridAppend::New();
    ;
  }
  else if (vtkGraph::SafeDownCast(result))
  {
    vtkGenericWarningMacro("Support for vtkGraph has been depreciated.") return false;
  }
  else if (vtkCompositeDataSet::SafeDownCast(result))
  {
    // this only supports composite datasets of polydata and unstructured
    // grids.
    vtkAppendCompositeDataLeaves* cdl = vtkAppendCompositeDataLeaves::New();
    cdl->AppendFieldDataOn();
    appender = cdl;
  }
  else
  {
    vtkGenericWarningMacro(<< result->GetClassName() << " cannot be merged");
    result->ShallowCopy(pieces[0]);
    return false;
  }
  std::vector<vtkSmartPointer<vtkDataObject> >::iterator iter;
  for (iter = pieces.begin(); iter != pieces.end(); ++iter)
  {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetPointer());

    if (ds && ds->GetNumberOfPoints() == 0)
    {
      // skip empty pieces.
      continue;
    }
    vtkNew<vtkTrivialProducer> tp;
    tp->SetOutput(iter->GetPointer());
    appender->AddInputConnection(0, tp->GetOutputPort());
  }
  // input connections may be 0, since we skip empty inputs in the loop above.
  if (appender->GetNumberOfInputConnections(0) > 0)
  {
    appender->Update();
    result->ShallowCopy(appender->GetOutputDataObject(0));
  }
  appender->Delete();
  return true;
}

//----------------------------------------------------------------------------
void vtkMultiProcessControllerHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
