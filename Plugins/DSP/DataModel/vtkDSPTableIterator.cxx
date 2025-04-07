// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDSPTableIterator.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchDSPArrayList.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
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
  virtual ~Worker() = default;
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

  ~TypedWorker() override = default;

  void SetIndex(vtkIdType index) override { this->Array->SetIndex(index); }

private:
  MDArrayT* Array = nullptr;
};

//-----------------------------------------------------------------------------
struct NbIterationsIdentifier
{
  vtkIdType NbIterations = 0;

  template <typename ArrayT>
  void operator()(ArrayT* array)
  {
    vtkIdType nbArrays = array->GetNumberOfArrays();
    this->NbIterations =
      ((nbArrays < this->NbIterations || this->NbIterations == 0) ? nbArrays : this->NbIterations);
  }
};

//-----------------------------------------------------------------------------
struct ImplicitShallowCopier
{
  vtkTable* InternalTable = nullptr;

  ImplicitShallowCopier(vtkTable* table)
    : InternalTable(table)
  {
  }

  template <typename ArrayT>
  vtkSmartPointer<ArrayT> operator()(ArrayT* array)
  {
    auto shallowCopied = vtkSmartPointer<ArrayT>::New();
    shallowCopied->ImplicitShallowCopy(array);
    this->InternalTable->GetRowData()->AddArray(shallowCopied);
    return shallowCopied;
  }
};

//-----------------------------------------------------------------------------
struct WorkerCreator
{
  template <typename ArrayT>
  void operator()(ArrayT* array, std::shared_ptr<Worker>& worker,
    NbIterationsIdentifier& identifier, ImplicitShallowCopier& copier)
  {
    auto copiedArray = copier(array);
    worker = std::make_shared<TypedWorker<ArrayT>>(copiedArray.Get());
    identifier(copiedArray.Get());
  }
};

//-----------------------------------------------------------------------------
std::tuple<vtkIdType, std::vector<std::shared_ptr<Worker>>, vtkSmartPointer<vtkTable>>
DispatchInitialize(vtkTable* table)
{
  using MDArrayList = vtkArrayDispatch::MultiDimensionalArrays;
  using MDDispatcher = vtkArrayDispatch::DispatchByArray<MDArrayList>;

  std::vector<std::shared_ptr<Worker>> workers;
  WorkerCreator workerCreator;
  NbIterationsIdentifier nbIterationsID;
  auto internalTable = vtkSmartPointer<vtkTable>::New();
  ImplicitShallowCopier copier(internalTable);

  for (vtkIdType iArr = 0; iArr < table->GetNumberOfColumns(); ++iArr)
  {
    vtkDataArray* array = vtkDataArray::SafeDownCast(table->GetColumn(iArr));
    if (!array)
    {
      continue;
    }

    std::shared_ptr<Worker> typeErasedWorker;
    if (!MDDispatcher::Execute(array, workerCreator, typeErasedWorker, nbIterationsID, copier))
    {
      internalTable->GetRowData()->AddArray(array);
    }
    else
    {
      workers.emplace_back(typeErasedWorker);
    }
  }

  return std::make_tuple(nbIterationsID.NbIterations, workers, internalTable);
}
}

//-----------------------------------------------------------------------------
struct vtkDSPTableIterator::vtkInternals
{
  // Needed since there is no GetIndex method
  // on vtkMultiDimensionalArray
  vtkIdType CurrentIdx = 0;
  vtkIdType NbIterations = 0;
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
  vtkIdType dispatchedNbIterations = std::get<0>(dispatchedRes);
  result->Internals->NbIterations = dispatchedNbIterations != 0 ? dispatchedNbIterations : 1;
  result->Internals->Workers = std::get<1>(dispatchedRes);
  result->Internals->Table = std::get<2>(dispatchedRes);

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
  if (!this->IsDoneWithTraversal())
  {
    std::for_each(this->Internals->Workers.begin(), this->Internals->Workers.end(),
      [&](std::shared_ptr<::Worker>& worker) { worker->SetIndex(this->Internals->CurrentIdx); });
  }
}

//-----------------------------------------------------------------------------
bool vtkDSPTableIterator::IsDoneWithTraversal()
{
  return this->Internals->CurrentIdx >= this->Internals->NbIterations;
}

//-----------------------------------------------------------------------------
vtkTable* vtkDSPTableIterator::GetCurrentTable()
{
  return this->Internals->Table;
}

//-----------------------------------------------------------------------------
vtkIdType vtkDSPTableIterator::GetNumberOfIterations()
{
  return this->Internals->NbIterations;
}
