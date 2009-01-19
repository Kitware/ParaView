/*=========================================================================

  Program:   ParaView
  Module:    vtkCompositeDataToUnstructuredGridFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataToUnstructuredGridFilter.h"

#include "vtkAppendFilter.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessControllerHelper.h"

vtkStandardNewMacro(vtkCompositeDataToUnstructuredGridFilter);
vtkCxxRevisionMacro(vtkCompositeDataToUnstructuredGridFilter, "1.3");
//----------------------------------------------------------------------------
vtkCompositeDataToUnstructuredGridFilter::vtkCompositeDataToUnstructuredGridFilter()
{
  this->SubTreeCompositeIndex = 0;
}

//----------------------------------------------------------------------------
vtkCompositeDataToUnstructuredGridFilter::~vtkCompositeDataToUnstructuredGridFilter()
{
}

//----------------------------------------------------------------------------
int vtkCompositeDataToUnstructuredGridFilter::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkCompositeDataSet* cd = vtkCompositeDataSet::GetData(inputVector[0], 0);
  vtkUnstructuredGrid* ug = vtkUnstructuredGrid::GetData(inputVector[0], 0);
  vtkDataSet* ds = vtkDataSet::GetData(inputVector[0], 0);
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::GetData(outputVector, 0);

  if (ug)
    {
    output->ShallowCopy(ug);
    return 1;
    }


  vtkAppendFilter* appender = vtkAppendFilter::New();
  if (ds)
    {
    this->AddDataSet(ds, appender);
    }
  else if (cd)
    {
    if (this->SubTreeCompositeIndex == 0)
      {
      this->ExecuteSubTree(cd, appender);
      }

    vtkCompositeDataIterator* iter = cd->NewIterator();
    iter->VisitOnlyLeavesOff();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal() && 
      iter->GetCurrentFlatIndex() <= this->SubTreeCompositeIndex;
      iter->GoToNextItem())
      {
      if (iter->GetCurrentFlatIndex() == this->SubTreeCompositeIndex)
        {
        vtkDataObject* curDO = iter->GetCurrentDataObject();
        vtkCompositeDataSet* curCD = vtkCompositeDataSet::SafeDownCast(curDO);
        vtkUnstructuredGrid* curUG = vtkUnstructuredGrid::SafeDownCast(curDO);
        vtkDataSet* curDS = vtkUnstructuredGrid::SafeDownCast(curDO);
        if (curUG)
          {
          output->ShallowCopy(curUG);
          // NOTE: Not using the appender at all.
          }
        else if (curDS)
          {
          this->AddDataSet(curDS, appender);
          }
        else if (curCD)
          {
          this->ExecuteSubTree(curCD, appender);
          }
        break;
        }
      }
    iter->Delete();
    }

  if (appender->GetNumberOfInputConnections(0) > 0)
    {
    appender->Update();
    output->ShallowCopy(appender->GetOutput());
    }

  appender->Delete();
  this->RemovePartialArrays(output);
  return 1;
}

//----------------------------------------------------------------------------
void vtkCompositeDataToUnstructuredGridFilter::ExecuteSubTree(
  vtkCompositeDataSet* curCD, vtkAppendFilter* appender)
{
  vtkCompositeDataIterator* iter2 = curCD->NewIterator();
  for (iter2->InitTraversal(); !iter2->IsDoneWithTraversal();
    iter2->GoToNextItem())
    {
    vtkDataSet* curDS = 
      vtkDataSet::SafeDownCast(iter2->GetCurrentDataObject());
    if (curDS)
      {
      appender->AddInput(curDS);
      }
    }
  iter2->Delete();
}

//----------------------------------------------------------------------------
void vtkCompositeDataToUnstructuredGridFilter::AddDataSet(
  vtkDataSet* ds, vtkAppendFilter* appender)
{
  vtkDataSet* clone = ds->NewInstance();
  clone->ShallowCopy(clone);
  appender->AddInput(clone);
  clone->Delete();
}

//----------------------------------------------------------------------------
int vtkCompositeDataToUnstructuredGridFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

#include <vtkstd/algorithm>
#include <vtkstd/set>
#include <vtkstd/string>
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkAbstractArray.h"
#include "vtkMultiProcessStream.h"

class vtkCDUGFMetaData
{
public:
  vtkstd::string Name;
  int NumberOfComponents;
  int Type;
  bool operator < (const vtkCDUGFMetaData& b) const
    {
    return (this->Name < b.Name ||
     this->NumberOfComponents < b.NumberOfComponents ||
     this->Type < b.Type);
    }
  void Set(vtkAbstractArray* array)
    {
    this->Name = array->GetName();
    this->NumberOfComponents = array->GetNumberOfComponents();
    this->Type = array->GetDataType();
    }
};

typedef vtkstd::set<vtkCDUGFMetaData> ArraySet;

static void CreateSet(ArraySet& arrays, vtkFieldData* dsa)
{
  int numArrays = dsa->GetNumberOfArrays();
  for (int cc=0; cc < numArrays; cc++)
    {
    vtkAbstractArray* array = dsa->GetAbstractArray(cc);
    if (array && array->GetName())
      {
      vtkCDUGFMetaData mda;
      mda.Set(array);
      arrays.insert(mda);
      }
    }
}

static void UpdateFromSet(vtkFieldData* dsa,
  ArraySet& arrays)
{
  int numArrays = dsa->GetNumberOfArrays();
  for (int cc=numArrays-1; cc >= 0; cc--)
    {
    vtkAbstractArray* array = dsa->GetAbstractArray(cc);
    if (array && array->GetName())
      {
      vtkCDUGFMetaData mda;
      mda.Set(array);
      if (arrays.find(mda) == arrays.end())
        {
        dsa->RemoveArray(array->GetName());
        }
      }
    }
}

static void SaveSet(vtkMultiProcessStream& stream,
  ArraySet& arrays)
{
  stream.Reset();
  stream << static_cast<unsigned int>(arrays.size());
  ArraySet::iterator iter;
  for (iter = arrays.begin(); iter != arrays.end(); ++iter)
    {
    stream << iter->Name
           << iter->NumberOfComponents
           << iter->Type;
    }
}

static void LoadSet(vtkMultiProcessStream& stream,
  ArraySet& arrays)
{
  arrays.clear();
  unsigned int numvalues;
  stream >> numvalues;
  for (unsigned int cc=0; cc < numvalues; cc++)
    {
    vtkCDUGFMetaData mda;
    stream >> mda.Name
           >> mda.NumberOfComponents
           >> mda.Type;
    arrays.insert(mda);
    }
}

static void IntersectStreams(
  vtkMultiProcessStream& A, vtkMultiProcessStream& B)
{
  ArraySet setA;
  ArraySet setB;
  ArraySet setC;

  ::LoadSet(A, setA);
  ::LoadSet(B, setB);
  vtkstd::set_intersection(setA.begin(), setA.end(),
    setB.begin(), setB.end(),
    vtkstd::inserter(setC, setC.begin()));

  B.Reset();
  ::SaveSet(B, setC);
}

//----------------------------------------------------------------------------
void vtkCompositeDataToUnstructuredGridFilter::RemovePartialArrays(
  vtkUnstructuredGrid* data)
{
  vtkMultiProcessController* controller =
    vtkMultiProcessController::GetGlobalController();
  if (!controller || controller->GetNumberOfProcesses() <= 1)
    {
    return;
    }

  ArraySet pdSet;
  ArraySet cdSet;

  ::CreateSet(pdSet, data->GetPointData());
  ::CreateSet(cdSet, data->GetCellData());

  vtkMultiProcessStream pdStream;
  vtkMultiProcessStream cdStream;
  ::SaveSet(pdStream, pdSet);
  ::SaveSet(cdStream, cdSet);

  vtkMultiProcessControllerHelper::ReduceToAll(
    controller,
    pdStream,
    ::IntersectStreams,
    1278392);
  vtkMultiProcessControllerHelper::ReduceToAll(
    controller,
    cdStream,
    ::IntersectStreams,
    1278393);
  ::LoadSet(pdStream, pdSet);
  ::LoadSet(cdStream, cdSet);

  ::UpdateFromSet(data->GetPointData(), pdSet);
  ::UpdateFromSet(data->GetCellData(), cdSet);

  //cout << controller->GetLocalProcessId() << "NumPts : " << pdSet.size() << endl;
  //cout << controller->GetLocalProcessId() << "NumCells : " << cdSet.size() << endl;
}

//----------------------------------------------------------------------------
void vtkCompositeDataToUnstructuredGridFilter::PrintSelf(ostream& os,
  vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SubTreeCompositeIndex: " 
    << this->SubTreeCompositeIndex << endl;
}


