/*=========================================================================

  Program:   ParaView
  Module:    vtkCleanArrays.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCleanArrays.h"

#include "vtkAbstractArray.h"
#include "vtkCellData.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessControllerHelper.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkDataSet.h"
#include "vtkDataArray.h"

#include <algorithm>
#include <set>
#include <string>
#include <iterator>

vtkStandardNewMacro(vtkCleanArrays);
vtkCxxSetObjectMacro(vtkCleanArrays, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkCleanArrays::vtkCleanArrays()
{
  this->FillPartialArrays = false;
  this->Controller = 0;
  this->SetController(
    vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkCleanArrays::~vtkCleanArrays()
{
  this->SetController(0);
}

//****************************************************************************
class vtkCleanArrays::vtkArrayData
{
public:
  std::string Name;
  int NumberOfComponents;
  int Type;
  vtkArrayData() 
    {
    this->NumberOfComponents = 0;
    this->Type = 0;
    }
  vtkArrayData(const vtkArrayData& other)
    {
    this->Name = other.Name;
    this->NumberOfComponents = other.NumberOfComponents;
    this->Type = other.Type;
    }

  bool operator < (const vtkArrayData& b) const
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

  vtkAbstractArray* NewArray(vtkIdType numTuples)const
    {
    vtkAbstractArray* array = vtkAbstractArray::CreateArray(this->Type);
    if (array)
      {
      array->SetName(this->Name.c_str());
      array->SetNumberOfComponents(this->NumberOfComponents);
      array->SetNumberOfTuples(numTuples);
      vtkDataArray* data_array = vtkDataArray::SafeDownCast(array);
      for (int cc=0; data_array && cc < this->NumberOfComponents; cc++)
        {
        data_array->FillComponent(cc, 0.0);
        }
      }
    return array;
    }

};

class vtkCleanArrays::vtkArraySet : public std::set<vtkCleanArrays::vtkArrayData>
{
  int Valid;
public:
  vtkArraySet():Valid(0) { }
  bool IsValid() const
    {
    return this->Valid != 0;
    }
  void MarkValid() 
    {
    this->Valid = 1;
    }

  // Fill up \c this with arrays from \c dsa
  void Initialize(vtkDataSet* ds, vtkFieldData* dsa)
    {
    this->Valid = (ds->GetNumberOfPoints() > 0)? 1 : 0;
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
        vtkCleanArrays::vtkArrayData mda;
        mda.Set(array);
        this->insert(mda);
        }
      }
    }

  // Remove arrays from \c dsa not present in \c this.
  void UpdateFieldData(vtkFieldData* dsa)
    {
    if (this->Valid == 0)
      {
      return;
      }
    int numArrays = dsa->GetNumberOfArrays();
    for (int cc=numArrays-1; cc >= 0; cc--)
      {
      vtkAbstractArray* array = dsa->GetAbstractArray(cc);
      if (array && array->GetName())
        {
        vtkCleanArrays::vtkArrayData mda;
        mda.Set(array);
        if (this->find(mda) == this->end())
          {
          //cout << "Removing: " << array->GetName() << endl;
          dsa->RemoveArray(array->GetName());
          }
        else
          {
          this->erase(mda);
          }
        }
      }
    // Now fill any missing arrays.
    for (iterator iter = this->begin(); iter != this->end(); ++iter)
      {
      vtkAbstractArray* array = iter->NewArray(dsa->GetNumberOfTuples());
      if (array)
        {
        dsa->AddArray(array);
        array->Delete();
        }
      }
    }

  void Save(vtkMultiProcessStream& stream)
    {
    stream.Reset();
    stream << this->Valid;
    stream << static_cast<unsigned int>(this->size());
    vtkCleanArrays::vtkArraySet::iterator iter;
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
    stream >> this->Valid;
    stream >> numvalues;
    for (unsigned int cc=0; cc < numvalues; cc++)
      {
      vtkCleanArrays::vtkArrayData mda;
      stream >> mda.Name
        >> mda.NumberOfComponents
        >> mda.Type;
      this->insert(mda);
      }
    }
  void Print()
    {
    vtkCleanArrays::vtkArraySet::iterator iter;
    cout << "Valid: " << this->Valid << endl;
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
  vtkCleanArrays::vtkArraySet setA;
  vtkCleanArrays::vtkArraySet setB;
  vtkCleanArrays::vtkArraySet setC;

  setA.Load(A);
  setB.Load(B);
  if (setA.IsValid() && setB.IsValid())
    {
    std::set_intersection(setA.begin(), setA.end(),
      setB.begin(), setB.end(),
      std::inserter(setC, setC.begin()));
    setC.MarkValid();
    }
  else if (setA.IsValid())
    {
    setC = setA;
    }
  else if (setB.IsValid())
    {
    setC = setB;
    }

  B.Reset();
  setC.Save(B);
}

//----------------------------------------------------------------------------
static void UnionStreams(
  vtkMultiProcessStream& A, vtkMultiProcessStream& B)
{
  vtkCleanArrays::vtkArraySet setA;
  vtkCleanArrays::vtkArraySet setB;
  vtkCleanArrays::vtkArraySet setC;

  setA.Load(A);
  setB.Load(B);
  if (setA.IsValid() && setB.IsValid())
    {
    std::set_union(setA.begin(), setA.end(),
      setB.begin(), setB.end(),
      std::inserter(setC, setC.begin()));
    setC.MarkValid();
    }
  else if (setA.IsValid())
    {
    setC = setA;
    }
  else if (setB.IsValid())
    {
    setC = setB;
    }

  B.Reset();
  setC.Save(B);
}


//----------------------------------------------------------------------------
int vtkCleanArrays::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0], 0);
  vtkDataSet* output = vtkDataSet::GetData(outputVector, 0);

  output->ShallowCopy(input);

  vtkMultiProcessController* controller = this->Controller;
  if (!controller || controller->GetNumberOfProcesses() <= 1)
    {
    // Nothing to do since not running in parallel.
    return 1;
    }

  vtkCleanArrays::vtkArraySet pdSet;
  vtkCleanArrays::vtkArraySet cdSet;
  pdSet.Initialize(output, output->GetPointData());
  cdSet.Initialize(output, output->GetCellData());

  vtkMultiProcessStream pdStream;
  vtkMultiProcessStream cdStream;
  pdSet.Save(pdStream);
  cdSet.Save(cdStream);

  vtkMultiProcessControllerHelper::ReduceToAll(
    controller,
    pdStream,
    this->FillPartialArrays ? ::UnionStreams : ::IntersectStreams,
    1278392);
  vtkMultiProcessControllerHelper::ReduceToAll(
    controller,
    cdStream,
    this->FillPartialArrays ? ::UnionStreams : ::IntersectStreams,
    1278393);
  pdSet.Load(pdStream);
  cdSet.Load(cdStream);

  cdSet.UpdateFieldData(output->GetCellData());
  pdSet.UpdateFieldData(output->GetPointData());
  return 1;
}

//----------------------------------------------------------------------------
void vtkCleanArrays::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FillPartialArrays: " << this->FillPartialArrays << endl;
  os << indent << "Controller: " << this->Controller << endl;
}


