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

#include "vtkAbstractArray.h"
#include "vtkAppendFilter.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessControllerHelper.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

#include <vtkstd/algorithm>
#include <vtkstd/set>
#include <vtkstd/string>

vtkStandardNewMacro(vtkCompositeDataToUnstructuredGridFilter);
vtkCxxRevisionMacro(vtkCompositeDataToUnstructuredGridFilter, "1.5");
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
        else if (curDS && curCD->GetNumberOfPoints() > 0)
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

//****************************************************************************
class vtkCDUGFMetaData
{
public:
  vtkstd::string Name;
  int NumberOfComponents;
  int Type;
  vtkCDUGFMetaData() 
    {
    this->NumberOfComponents = 0;
    this->Type = 0;
    }
  vtkCDUGFMetaData(const vtkCDUGFMetaData& other)
    {
    this->Name = other.Name;
    this->NumberOfComponents = other.NumberOfComponents;
    this->Type = other.Type;
    }

  bool operator < (const vtkCDUGFMetaData& b) const
    {
    if (this->Name != b.Name)
      {
      return this->Name < b.Name;
      }
    if (this->NumberOfComponents != b.NumberOfComponents)
      {
      return this->NumberOfComponents < b.NumberOfComponents;
      }
    return this->Type < b.Type;
    }

  void Set(vtkAbstractArray* array)
    {
    this->Name = array->GetName();
    this->NumberOfComponents = array->GetNumberOfComponents();
    this->Type = array->GetDataType();
    }

};

class vtkCGUGArraySet : public vtkstd::set<vtkCDUGFMetaData>
{
public:
  // Fill up \c this with arrays from \c dsa
  void Initialize(vtkFieldData* dsa)
    {
    int numArrays = dsa->GetNumberOfArrays();
    if (dsa->GetNumberOfTuples() == 0)
      {
      numArrays = 0;
      }
    for (int cc=0; cc < numArrays; cc++)
      {
      vtkAbstractArray* array = dsa->GetAbstractArray(cc);
      if (array && array->GetName())
        {
        vtkCDUGFMetaData mda;
        mda.Set(array);
        this->insert(mda);
        }
      }
    }

  // Remove arrays from \c dsa not present in \c this.
  void UpdateFieldData(vtkFieldData* dsa)
    {
    if (this->size() == 0)
      {
      return;
      }
    int numArrays = dsa->GetNumberOfArrays();
    for (int cc=numArrays-1; cc >= 0; cc--)
      {
      vtkAbstractArray* array = dsa->GetAbstractArray(cc);
      if (array && array->GetName())
        {
        vtkCDUGFMetaData mda;
        mda.Set(array);
        if (this->find(mda) == this->end())
          {
          //cout << "Removing: " << array->GetName() << endl;
          dsa->RemoveArray(array->GetName());
          }
        }
      }
    }

  void Save(vtkMultiProcessStream& stream)
    {
    stream.Reset();
    stream << static_cast<unsigned int>(this->size());
    vtkCGUGArraySet::iterator iter;
    for (iter = this->begin(); iter != this->end(); ++iter)
      {
      stream << iter->Name
        << iter->NumberOfComponents
        << iter->Type;
      }
    }

  void Load(vtkMultiProcessStream& stream)
    {
    this->clear();
    unsigned int numvalues;
    stream >> numvalues;
    for (unsigned int cc=0; cc < numvalues; cc++)
      {
      vtkCDUGFMetaData mda;
      stream >> mda.Name
        >> mda.NumberOfComponents
        >> mda.Type;
      this->insert(mda);
      }
    }
  void Print()
    {
    vtkCGUGArraySet::iterator iter;
    for (iter = this->begin(); iter != this->end(); ++iter)
      {
      cout << iter->Name << ", "
        << iter->NumberOfComponents << ", "
        << iter->Type << endl;
      }
    cout << "-----------------------------------" << endl << endl;
    }
};


//----------------------------------------------------------------------------
static void IntersectStreams(
  vtkMultiProcessStream& A, vtkMultiProcessStream& B)
{
  vtkCGUGArraySet setA;
  vtkCGUGArraySet setB;
  vtkCGUGArraySet setC;

  setA.Load(A);
  setB.Load(B);
  if (setA.size() > 0 && setB.size() > 0)
    {
    vtkstd::set_intersection(setA.begin(), setA.end(),
      setB.begin(), setB.end(),
      vtkstd::inserter(setC, setC.begin()));
    }
  else
    {
    vtkstd::set_union(setA.begin(), setA.end(),
      setB.begin(), setB.end(),
      vtkstd::inserter(setC, setC.begin()));
    }

  B.Reset();
  setC.Save(B);
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

  vtkCGUGArraySet pdSet;
  vtkCGUGArraySet cdSet;
  pdSet.Initialize(data->GetPointData());
  cdSet.Initialize(data->GetCellData());

  vtkMultiProcessStream pdStream;
  vtkMultiProcessStream cdStream;
  pdSet.Save(pdStream);
  cdSet.Save(cdStream);

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
  pdSet.Load(pdStream);
  cdSet.Load(cdStream);

  cdSet.UpdateFieldData(data->GetCellData());
  pdSet.UpdateFieldData(data->GetPointData());
}

//----------------------------------------------------------------------------
void vtkCompositeDataToUnstructuredGridFilter::PrintSelf(ostream& os,
  vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SubTreeCompositeIndex: " 
    << this->SubTreeCompositeIndex << endl;
}


