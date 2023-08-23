// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDSPTableIterator.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchDSPArrayList.h"
#include "vtkDataObject.h"
#include "vtkMultiDimensionalArray.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include <utility>

namespace
{
//-----------------------------------------------------------------------------
class Worker
{
public:
  virtual void SetIndex(vtkIdType index) = 0;
};

//-----------------------------------------------------------------------------
template <typename MDArrayT>
class TypedWorker : public Worker
{
public:
  TypedWorker(MDArrayT* typedArray)
    : Array(typedArray)
  {
  }

  void SetIndex(vtkIdType index) override { this->Array->SetIndex(index); }

private:
  MDArrayT* Array = nullptr;
};

//-----------------------------------------------------------------------------
struct LastIndexIdentifier
{
  vtkIdType LastIndex = 0;

  template <typename ArrayT>
  void operator()(ArrayT* array)
  {
    vtkIdType nbArrays = array->GetNumberOfArrays();
    this->LastIndex =
      ((nbArrays < this->LastIndex || this->LastIndex == 0) ? nbArrays : this->LastIndex);
  }
};

//-----------------------------------------------------------------------------
struct WorkerCreator
{
  template <typename ArrayT>
  void operator()(ArrayT* array, std::shared_ptr<Worker>& worker, LastIndexIdentifier& identifier)
  {
    worker = std::make_shared<TypedWorker<ArrayT>>(array);
    identifier(array);
  }
};

//-----------------------------------------------------------------------------
std::pair<vtkIdType, std::vector<std::shared_ptr<Worker>>> DispatchInitialize(vtkTable* table)
{
  using MDArrayList = vtkArrayDispatch::MultiDimensionalArrays;
  using MDDispatcher = vtkArrayDispatch::DispatchByArray<MDArrayList>;

  std::vector<std::shared_ptr<Worker>> workers;
  WorkerCreator workerCreator;
  LastIndexIdentifier lastIndexID;

  for (vtkIdType iArr = 0; iArr < table->GetNumberOfColumns(); ++iArr)
  {
    vtkDataArray* array = vtkDataArray::SafeDownCast(table->GetColumn(iArr));
    if (!array)
    {
      continue;
    }

    std::shared_ptr<Worker> typeErasedWorker;
    MDDispatcher::Execute(array, workerCreator, typeErasedWorker, lastIndexID);
    if (typeErasedWorker)
    {
      workers.emplace_back(typeErasedWorker);
    }
  }

  return std::make_pair(lastIndexID.LastIndex, workers);
}
}

//-----------------------------------------------------------------------------
struct vtkDSPTableIterator::vtkInternals
{
  // Needed since there is no GetIndex method
  // on vtkMultiDimensionalArray
  vtkIdType CurrentIdx = 0;
  vtkIdType LastIdx = 0;
  vtkSmartPointer<vtkTable> Table;
  std::vector<std::shared_ptr<Worker>> Workers;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkDSPTableIterator);

//-----------------------------------------------------------------------------
vtkDSPTableIterator* vtkDSPTableIterator::New(vtkTable* table)
{
  auto result = vtkDSPTableIterator::New();
  auto dispatchedRes = ::DispatchInitialize(table);
  result->Internals->Table = table;
  result->Internals->LastIdx = dispatchedRes.first;
  result->Internals->Workers = dispatchedRes.second;

  return result;
}

//-----------------------------------------------------------------------------
vtkDSPTableIterator::vtkDSPTableIterator()
  : Internals(new vtkInternals())
{
}

//-----------------------------------------------------------------------------
void vtkDSPTableIterator::GoToFirstItem()
{
  this->Internals->CurrentIdx = 0;
  std::for_each(this->Internals->Workers.begin(), this->Internals->Workers.end(),
    [&](std::shared_ptr<::Worker>& worker) { worker->SetIndex(this->Internals->CurrentIdx); });
}

//-----------------------------------------------------------------------------
void vtkDSPTableIterator::GoToNextItem()
{
  this->Internals->CurrentIdx++;
  std::for_each(this->Internals->Workers.begin(), this->Internals->Workers.end(),
    [&](std::shared_ptr<::Worker>& worker) { worker->SetIndex(this->Internals->CurrentIdx); });
}

//-----------------------------------------------------------------------------
bool vtkDSPTableIterator::IsDoneWithTraversal()
{
  return this->Internals->CurrentIdx >= this->Internals->LastIdx;
}

//-----------------------------------------------------------------------------
vtkTable* vtkDSPTableIterator::GetCurrentTable()
{
  return this->Internals->Table;
}
