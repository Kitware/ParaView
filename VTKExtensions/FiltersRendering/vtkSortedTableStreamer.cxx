/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSortedTableStreamer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSortedTableStreamer.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkEdgeListIterator.h"
#include "vtkEventForwarderCommand.h"
#include "vtkExtractSelection.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSignedCharArray.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTree.h"
#include "vtkVertexListIterator.h"

#include "vtkCommunicator.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkUnsignedIntArray.h"

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include <float.h>

#include <sstream>
#include <string>
using std::ostringstream;

//****************************************************************************
class vtkSortedTableStreamer::InternalsBase
{
public:
  InternalsBase() {}
  virtual ~InternalsBase() {}

  virtual void SetSelectedComponent(int newValue) = 0;
  virtual void InvalidateCache() = 0;
  virtual int Extract(
    vtkTable* input, vtkTable* output, vtkIdType block, vtkIdType blockSize, bool revertOrder) = 0;
  virtual int Compute(
    vtkTable* input, vtkTable* output, vtkIdType block, vtkIdType blockSize, bool revertOrder) = 0;
  virtual bool IsInvalid(vtkTable* input, vtkDataArray* dataToProcess) = 0;
  virtual bool IsSortable() = 0;
  virtual bool TestInternalClasses() = 0;

  // --------------------------------------------------------------------------
  //  static void WaitForGDB()
  //    {
  //    bool debug = true;
  //    cout << "Waiting GDB for process "
  //         << vtkMultiProcessController::GetGlobalController()->GetLocalProcessId()
  //         << " launch: gdb --pid=" << getpid() << endl
  //         << " - Go up till you are in the while loop (sleep)." << endl
  //         << " - Type: set var debug = false" << endl;
  //    while (debug) sleep(5);
  //    }
  // --------------------------------------------------------------------------
  static void MergeTable(
    vtkIdType processId, vtkTable* otherTable, vtkTable* mergedTable, vtkIdType minSize)
  {
    // Loop on all column of the table
    vtkAbstractArray* otherArray = 0;
    vtkAbstractArray* dstArray = 0;
    bool needNewArray = false;
    for (vtkIdType colIdx = 0; colIdx < otherTable->GetNumberOfColumns(); ++colIdx)
    {
      otherArray = otherTable->GetColumn(colIdx);
      dstArray = mergedTable->GetColumnByName(otherArray->GetName());
      needNewArray = (!dstArray);

      if (needNewArray)
      {
        dstArray = otherArray->NewInstance();
        dstArray->SetNumberOfComponents(otherArray->GetNumberOfComponents());
        dstArray->SetName(otherArray->GetName());
        dstArray->Allocate(minSize * otherArray->GetNumberOfComponents());
        if (auto oinfo = otherArray->GetInformation())
        {
          dstArray->CopyInformation(oinfo);
        }
      }

      for (vtkIdType idx = 0; idx < otherArray->GetNumberOfTuples(); ++idx)
      {
        if (dstArray->InsertNextTuple(idx, otherArray) == -1)
        {
          cout << "ERROR MergeTable::InsertNextTuple is not working." << endl;
        }
      }

      if (needNewArray)
      {
        mergedTable->GetRowData()->AddArray(dstArray);
        dstArray->FastDelete();
      }
    }

    if (processId > -1 && mergedTable->GetColumnByName("vtkOriginalProcessIds"))
    {
      vtkIdTypeArray* processIdArray =
        vtkIdTypeArray::SafeDownCast(mergedTable->GetColumnByName("vtkOriginalProcessIds"));

      for (vtkIdType idx = 0; idx < otherTable->GetNumberOfRows(); idx++)
      {
        processIdArray->InsertNextTuple1(processId);
      }
    }
  }
};
//----------------------------------------------------------------------------
template <class T>
class vtkSortedTableStreamer::Internals : public vtkSortedTableStreamer::InternalsBase
{
public:
  class Histogram
  {
  public:
    vtkIdType* Values;
    double Delta;
    double Min;
    int Size;
    vtkIdType TotalValues;
    bool Inverted;

    Histogram()
    {
      this->Inverted = false;
      this->TotalValues = 0;
      this->Size = 0;
      this->Min = 0;
      this->Delta = 1;
      this->Values = 0;
    }

    Histogram(int size)
      : Size(size)
      , TotalValues(0)
    {
      this->Min = 0;
      this->Delta = 0;
      this->Inverted = false;
      this->Values = new vtkIdType[size];
      for (int i = 0; i < this->Size; ++i)
      {
        this->Values[i] = 0;
      }
    }

    virtual ~Histogram()
    {
      if (this->Values)
      {
        delete[] this->Values;
        this->Values = 0;
      }
    }

    bool CanBeReduced() { return this->TotalValues != this->Values[0] && (this->Delta > 10E-5); }

    bool SetScalarRange(double* scalarRange)
    {
      this->Min = scalarRange[0];
      this->Delta = (scalarRange[1] - scalarRange[0]) / (double)this->Size;
      return (scalarRange[1] > scalarRange[0]);
    }

    void CopyRangeTo(Histogram& other)
    {
      other.Min = this->Min;
      other.Delta = this->Delta;
    }

    void AddValue(double value)
    {
      int idx = vtkMath::Floor((value - this->Min) / this->Delta);
      if (idx == this->Size)
      {
        --idx;
      }
      if (this->Inverted)
      {
        idx = this->Size - idx - 1;
      }
      if (idx >= 0 && idx < this->Size)
      {
        this->TotalValues++;
        this->Values[idx]++;
      }
      else if (value == static_cast<T>(this->Min))
      {
        // Manage type troncature
        this->TotalValues++;
        this->Values[0]++;
      }
      else
      {
        cout << "Try to add value out of the histogran range: " << value << " Range: [" << this->Min
             << ", " << (this->Min + this->Delta * this->Size) << "]" << endl;
      }
    }

    // Description:
    // Find the selected bar corresponding to the nth element in the histogram.
    // As result, we return the number of elements found before the selected bar
    // as well as providing the scalar range and the index of that bar.
    vtkIdType GetNewRange(vtkIdType nthElement, vtkIdType& selectedBarIdx, double* scalarRange)
    {
      if (nthElement >= this->TotalValues)
      {
        scalarRange[0] = this->Min;
        scalarRange[1] = this->Min + this->Delta * this->Size;
        selectedBarIdx = this->Size - 1;
        return this->TotalValues;
      }
      vtkIdType nbElementsFounds = 0;
      selectedBarIdx = 0;
      while (nbElementsFounds + this->Values[selectedBarIdx] < nthElement)
      {
        nbElementsFounds += this->Values[selectedBarIdx++];
      }
      if (this->Inverted)
      {
        scalarRange[1] = this->Min + this->Delta * (this->Size - selectedBarIdx);
        scalarRange[0] = scalarRange[1] - this->Delta;
      }
      else
      {
        scalarRange[0] = this->Min + this->Delta * selectedBarIdx;
        scalarRange[1] = scalarRange[0] + this->Delta;
      }
      return nbElementsFounds;
    }

    void Merge(const Histogram& other)
    {
      if (this->Min != other.Min || this->Delta != other.Delta || Size != other.Size)
      {
        // Throw exception because not compatible histogram
        cout << "ERROR: Histogram::Merge not compatible histogram !" << endl;
      }
      for (int i = 0; i < this->Size; i++)
      {
        this->TotalValues += other.Values[i];
        this->Values[i] += other.Values[i];
      }
    }

    void UpdateTotalValues()
    {
      this->TotalValues = 0;
      for (int i = 0; i < this->Size; i++)
      {
        this->TotalValues += Values[i];
      }
    }

    vtkIdType GetNumberOfElements(int lowerIndex, int upperIndex)
    {
      vtkIdType nbElements = 0;
      if (lowerIndex == -1 || upperIndex == -1 || lowerIndex >= this->Size)
      {
        return 0;
      }
      upperIndex = (upperIndex > this->Size) ? this->Size : upperIndex;
      for (int i = lowerIndex; i < upperIndex; ++i)
      {
        nbElements += this->Values[i];
      }
      return nbElements;
    }

    void Print()
    {
      cout << "Histo: " << endl
           << " - NbElements: " << this->TotalValues << endl
           << " - Min: " << this->Min << endl
           << " - Delta: " << this->Delta << endl
           << " - Size: " << this->Size << endl;
      double min = this->Min;
      for (int i = 0; i < this->Size; i++)
      {
        if (this->Values[i] > 0)
        {
          cout << " - [" << min << ", ";
          min += this->Delta;
          cout << min << "]: " << this->Values[i] << endl;
        }
        else
        {
          min += this->Delta;
        }
      }
    }

    void CopyTo(Histogram& other)
    {
      other.Inverted = this->Inverted;
      other.Delta = this->Delta;
      other.Min = this->Min;
      other.Size = this->Size;
      other.TotalValues = this->TotalValues;
      if (other.Values)
      {
        delete[] other.Values;
        other.Values = 0;
      }
      other.Values = new vtkIdType[this->Size];
      for (int i = 0; i < this->Size; i++)
      {
        other.Values[i] = this->Values[i];
      }
    }

    void ClearHistogramValues()
    {
      this->TotalValues = 0;
      if (!this->Values)
      {
        this->Values = new vtkIdType[this->Size];
      }
      for (int i = 0; i < this->Size; i++)
      {
        this->Values[i] = 0;
      }
    }
  };
  class SortableArrayItem
  {
  public:
    T Value;
    vtkIdType OriginalIndex;

    static bool Descendent(const SortableArrayItem& a, const SortableArrayItem& b)
    {
      if (a.Value == b.Value)
      {
        // Need to differentiate the same scalar value in some way
        // otherwise those values will be removed in the sorting process.
        return a.OriginalIndex < b.OriginalIndex;
      }
      return a.Value < b.Value;
    }

    static bool Ascendent(const SortableArrayItem& a, const SortableArrayItem& b)
    {
      if (a.Value == b.Value)
      {
        // Need to differentiate the same scalar value in some way
        // otherwise those values will be removed in the sorting process.
        return a.OriginalIndex > b.OriginalIndex;
      }
      return a.Value > b.Value;
    }

    bool operator<(const SortableArrayItem& other) const
    {
      if (this->Value == other.Value)
      {
        // Need to differentiate the same scalar value in some way
        // otherwise those values will be removed in the sorting process.
        return this->OriginalIndex < other.OriginalIndex;
      }
      return this->Value < other.Value;
    }

    bool operator>(const SortableArrayItem& other) const
    {
      if (this->Value == other.Value)
      {
        // Need to differentiate the same scalar value in some way
        // otherwise those values will be removed in the sorting process.
        return this->OriginalIndex > other.OriginalIndex;
      }
      return this->Value > other.Value;
    }

    SortableArrayItem& operator=(const SortableArrayItem& other)
    {
      if (this != &other) // make sure not the same object
      {
        this->Value = other.Value;
        this->OriginalIndex = other.OriginalIndex;
      }
      return *this; // Return ref for multiple assignment
    }
  };
  class ArraySorter
  {
  public:
    Histogram* Histo;
    SortableArrayItem* Array;
    vtkIdType ArraySize;

    ArraySorter()
    {
      this->Array = 0;
      this->Histo = 0;
    }

    ~ArraySorter() { this->Clear(); }

    void Clear()
    {
      if (this->Array)
      {
        delete[] this->Array;
        this->Array = 0;
      }
      if (this->Histo)
      {
        delete this->Histo;
        this->Histo = 0;
      }
    }
    void FillArray(vtkIdType numTuples)
    {
      // Clear memory if needed
      this->Clear();

      // Allocate memory and fill the structure
      this->ArraySize = numTuples;
      this->Array = new SortableArrayItem[this->ArraySize];

      // Fill the sortable array
      for (vtkIdType i = 0; i < this->ArraySize; ++i)
      {
        this->Array[i].OriginalIndex = i;
        this->Array[i].Value = 0;
      }
    }

    void Update(T* dataPtr, vtkIdType numTuples, int numComponents, int selectedComponent,
      vtkIdType histogramSize, double* scalarRange, bool reverseOrder)
    {
      // Clear memory if needed
      this->Clear();

      if (numComponents == 1 && selectedComponent < 0)
      {
        selectedComponent = 0; // We can not compute magnitude on scalar value
      }

      // Allocate memory and fill the structure
      this->Histo = new Histogram(histogramSize);
      this->Histo->Inverted = reverseOrder;
      this->Histo->SetScalarRange(scalarRange);
      this->ArraySize = numTuples;
      this->Array = new SortableArrayItem[this->ArraySize];

      // Fill the sortable array
      for (vtkIdType i = 0; i < this->ArraySize; ++i)
      {
        this->Array[i].OriginalIndex = i;
        double value = 0;
        double tmp;
        if (selectedComponent < 0)
        {
          // Compute magnitude
          for (int k = 0; k < numComponents; k++)
          {
            tmp = static_cast<double>(dataPtr[k + i * numComponents]);
            value += tmp * tmp;
          }
          value = sqrt(value) / sqrt(static_cast<double>(numComponents));
          this->Array[i].Value = static_cast<T>(value);
        }
        else
        {
          this->Array[i].Value = dataPtr[selectedComponent + i * numComponents];
          value = static_cast<double>(this->Array[i].Value);
        }
        this->Histo->AddValue(value);
      }

      // Sort it
      if (reverseOrder)
      {
        std::sort(this->Array, this->Array + this->ArraySize, SortableArrayItem::Ascendent);
      }
      else
      {
        std::sort(this->Array, this->Array + this->ArraySize, SortableArrayItem::Descendent);
      }
    }

    void SortProcessId(vtkIdType* dataPtr, vtkIdType numTuples, vtkIdType histogramSize,
      double* scalarRange, bool reverseOrder)
    {
      // Clear memory if needed
      this->Clear();

      // Allocate memory and fill the structure
      this->Histo = new Histogram(histogramSize);
      this->Histo->Inverted = reverseOrder;
      this->Histo->SetScalarRange(scalarRange);
      this->ArraySize = numTuples;
      this->Array = new SortableArrayItem[this->ArraySize];

      // Fill the sortable array
      for (vtkIdType i = 0; i < this->ArraySize; ++i)
      {
        this->Array[i].OriginalIndex = i;
        this->Array[i].Value = static_cast<T>(dataPtr[i]);
        double value = static_cast<double>(this->Array[i].Value);
        this->Histo->AddValue(value);
      }

      // Sort it
      if (reverseOrder)
      {
        std::sort(this->Array, this->Array + this->ArraySize, SortableArrayItem::Ascendent);
      }
      else
      {
        std::sort(this->Array, this->Array + this->ArraySize, SortableArrayItem::Descendent);
      }
    }
  };

public:
  Internals()
  {
    // Only used for testing
    this->LocalSorter = 0;
    this->GlobalHistogram = 0;
    this->Debug = false;
  }

  Internals(vtkTable* input, vtkDataArray* dataToSort, vtkMultiProcessController* controller)
  {
    // Default values
    this->SelectedComponent = 0;
    this->NeedToBuildCache = true;
    this->DataToSort = dataToSort;

    this->InputMTime = input->GetMTime();

    if (dataToSort) // Might be NULL
    {
      this->DataMTime = dataToSort->GetMTime();
    }

    // Get MPI objects
    this->MPI = controller->GetCommunicator();
    this->NumProcs = controller->GetNumberOfProcesses();
    this->Me = controller->GetLocalProcessId();

    // Create internal objects
    this->LocalSorter = new ArraySorter();
    this->GlobalHistogram = new Histogram(HISTOGRAM_SIZE);
  }

  ~Internals() override
  {
    if (this->LocalSorter)
      delete this->LocalSorter;
    if (this->GlobalHistogram)
      delete this->GlobalHistogram;
  }

  // --------------------------------------------------------------------------
  bool IsSortable() override
  {
    // See if one process is able to sort the table,
    // if not then just say NOT sortable
    int localCanSort = (this->DataToSort == NULL) ? 0 : 1;
    int globalCanSort;
    this->MPI->AllReduce(&localCanSort, &globalCanSort, 1, vtkCommunicator::MAX_OP);
    if (globalCanSort == 0)
    {
      return false;
    }

    // Communication buffer
    double localRange[2] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };

    // Get the local information
    if (this->DataToSort && this->DataToSort->GetNumberOfTuples() > 0)
    {
      this->DataToSort->GetRange(localRange, this->SelectedComponent);
    }

    // Gather the array range to build a common scale histogram
    this->MPI->AllReduce(&localRange[0], &this->CommonRange[0], 1, vtkCommunicator::MIN_OP);
    this->MPI->AllReduce(&localRange[1], &this->CommonRange[1], 1, vtkCommunicator::MAX_OP);

    // Make sure that the range stay in the original min/max of the type
    // in case of magnitude.
    // CAUTION: As some process may not have DataToSort due to block
    //          distribution, we MUST globaly agree on a ratio.
    // 3 case:
    //    - scalar    => no ratio (=1)
    //    - magniture => ratio    (>0)
    //    - no data   => ratio ? MUST be overridden by other

    double localRatio = 1;
    double globalRatio;
    if (this->DataToSort && this->SelectedComponent == -1 &&
      this->DataToSort->GetNumberOfComponents() > 1)
    {
      localRatio = sqrt(static_cast<double>(this->DataToSort->GetNumberOfComponents()));
    }
    else if (this->DataToSort == NULL)
    {
      localRatio = 0;
    }
    this->MPI->AllReduce(&localRatio, &globalRatio, 1, vtkCommunicator::MAX_OP);

    // Apply the common ratio
    this->CommonRange[0] /= globalRatio;
    this->CommonRange[1] /= globalRatio;

    double delta = (this->CommonRange[1] - this->CommonRange[0]);
    delta *= delta;
    bool sortable = delta > FLT_EPSILON;

    // Extend range with epsilon to make sure that a computed magnitude will fit in
    this->CommonRange[0] -= FLT_EPSILON;
    this->CommonRange[1] += FLT_EPSILON;

    return sortable;
  }

  // --------------------------------------------------------------------------
  int BuildCache(bool sortableArray, bool invertOrder)
  {
    // We are building the cache so no need to build it next time
    this->NeedToBuildCache = false;

    // Communication buffer
    vtkIdType* bufferHistogramValues = new vtkIdType[this->NumProcs * HISTOGRAM_SIZE];

    // Is there something to sort ???
    if (!sortableArray)
    {
      // Keep the same order as the local one because all the values are equals
      if (this->DataToSort)
      {
        this->LocalSorter->FillArray(this->DataToSort->GetNumberOfTuples());
      }
    }
    else
    {
      if (this->DataToSort)
      {
        // Sort and build local histogram
        this->LocalSorter->Update(static_cast<T*>(this->DataToSort->GetVoidPointer(0)),
          this->DataToSort->GetNumberOfTuples(), this->DataToSort->GetNumberOfComponents(),
          this->SelectedComponent, HISTOGRAM_SIZE, this->CommonRange, invertOrder);
      }
      else
      {
        // Build empty local histogram
        this->LocalSorter->Clear();
        this->LocalSorter->Histo = new Histogram(HISTOGRAM_SIZE);
        this->LocalSorter->Histo->SetScalarRange(this->CommonRange);
        this->LocalSorter->Histo->Inverted = invertOrder;
      }

      // Initialize the global histogram with same range
      this->LocalSorter->Histo->CopyRangeTo(*this->GlobalHistogram);

      // Make sure that the global histogram is properly initialized
      this->GlobalHistogram->ClearHistogramValues();
      this->GlobalHistogram->Inverted = invertOrder;

      // Send local histogram and receive every histogram from everybody
      this->MPI->AllGather(this->LocalSorter->Histo->Values, bufferHistogramValues, HISTOGRAM_SIZE);

      // Merge all histogram into the GlobalHistogram
      for (int idx = 0, nb = this->NumProcs * HISTOGRAM_SIZE; idx < nb; ++idx)
      {
        this->GlobalHistogram->TotalValues += bufferHistogramValues[idx];
        this->GlobalHistogram->Values[idx % HISTOGRAM_SIZE] += bufferHistogramValues[idx];
      }
    }

    delete[] bufferHistogramValues;
    return 1;
  }

  // --------------------------------------------------------------------------
  // The sorting is based on processId and the current order
  int Extract(vtkTable* input, vtkTable* output, vtkIdType block, vtkIdType blockSize,
    bool revertOrder) override
  {
    // ------------------------------------------------------------------------
    // Make sure that the Cache is built
    //    This will sort the local array, that's why we don't want to do it
    //    at each execution. Specially when we only change the requested block.
    // ------------------------------------------------------------------------
    if (this->NeedToBuildCache)
    {
      this->BuildCache(false, revertOrder);
    }

    // Build empty local table with empty arrays so they stay in the same order
    vtkSmartPointer<vtkTable> localResult;
    localResult.TakeReference(NewSubsetTable(input, NULL, 0, blockSize));

    // Get the array size of each processes
    vtkIdType* tableSizes = new vtkIdType[this->NumProcs];
    vtkIdType nbElems = input->GetNumberOfRows();
    this->MPI->AllGather(&nbElems, tableSizes, 1);

    // Get local idx based on the global one
    vtkIdType localOffset = block * blockSize;
    if (revertOrder)
    {
      for (int i = this->NumProcs - 1; this->Me < i; i--)
      {
        localOffset -= tableSizes[i];
      }
    }
    else
    {
      for (int i = 0; i < this->Me; i++)
      {
        localOffset -= tableSizes[i];
      }
    }

    // Extract the subset
    vtkIdType localSize = vtkMath::Min(tableSizes[this->Me], blockSize);
    if (localOffset < 0)
    {
      localSize = vtkMath::Max(static_cast<vtkIdType>(0),
        vtkMath::Min(localOffset + vtkMath::Max(tableSizes[this->Me], blockSize), blockSize));
      localOffset = 0;
    }
    else if (localOffset >= tableSizes[this->Me])
    {
      localOffset = 0;
      localSize = 0;
    }

    localResult.TakeReference(
      this->NewSubsetTable(input, this->LocalSorter, localOffset, localSize));

    // Free array used for MPI exchange
    delete[] tableSizes;

    // ------------------------------------------------------------------------
    // Find the process that will merge all subset table
    // ------------------------------------------------------------------------
    int mergePid = GetMergingProcessId(localResult.GetPointer());

    // ------------------------------------------------------------------------
    // Send or receive pieces
    // ------------------------------------------------------------------------
    if (this->Me == mergePid)
    {
      // Reset to correct range
      this->CommonRange[0] = 0;
      this->CommonRange[1] = this->NumProcs;

      // Add local vtkOriginalProcessIds array
      if (this->NumProcs > 1)
      {
        vtkSmartPointer<vtkIdTypeArray> processIdArray = vtkSmartPointer<vtkIdTypeArray>::New();
        processIdArray->SetName("vtkOriginalProcessIds");
        processIdArray->SetNumberOfComponents(1);
        processIdArray->Allocate(blockSize);
        vtkIdType processId = this->Me;
        for (vtkIdType idx = 0; idx < localResult->GetNumberOfRows(); idx++)
        {
          processIdArray->InsertNextTuple1(processId);
        }
        localResult->GetRowData()->AddArray(processIdArray);
      }

      // Receive from the others
      vtkSmartPointer<vtkTable> tmp = vtkSmartPointer<vtkTable>::New();
      for (int i = 0; i < this->NumProcs; i++)
      {
        if (i == mergePid)
          continue;

        this->MPI->Receive(tmp.GetPointer(), i, VTK_TABLE_EXCHANGE_TAG);
        this->MergeTable(i, tmp.GetPointer(), localResult.GetPointer(), blockSize);
      }

      // Sort new table/array
      vtkDataArray* subsetArray =
        vtkDataArray::SafeDownCast(localResult->GetColumnByName("vtkOriginalProcessIds"));

      if (subsetArray)
      {
        ArraySorter sorter;
        this->CommonRange[0] = 0;
        this->CommonRange[1] = this->NumProcs;
        // ProcessId array is not the same type of T
        sorter.SortProcessId(static_cast<vtkIdType*>(subsetArray->GetVoidPointer(0)),
          subsetArray->GetNumberOfTuples(), HISTOGRAM_SIZE, this->CommonRange, revertOrder);

        localResult.TakeReference(this->NewSubsetTable(
          localResult.GetPointer(), &sorter, 0, localResult->GetNumberOfRows()));
      }

      // Add extra information such as structured indices, block number...
      this->DecorateTable(input, localResult.GetPointer(), mergePid);

      // ShallowCopy it to the output
      output->ShallowCopy(localResult.GetPointer());
    }
    else
    {
      // Send to process mergePid
      this->MPI->Send(localResult.GetPointer(), mergePid, VTK_TABLE_EXCHANGE_TAG);

      // Add meta data of mergePid
      this->DecorateTable(input, 0, mergePid);
    }
    return 1;
  }
  // --------------------------------------------------------------------------
  int Compute(vtkTable* input, vtkTable* output, vtkIdType block, vtkIdType blockSize,
    bool revertOrder) override
  {
    // ------------------------------------------------------------------------
    // Make sure that the Cache is built
    //    This will sort the local array, that's why we don't want to do it
    //    at each execution. Specially when we only change the requested block.
    // ------------------------------------------------------------------------
    if (this->NeedToBuildCache)
    {
      this->BuildCache(true, revertOrder);
    }

    // ------------------------------------------------------------------------
    // Search for lower bound
    // ------------------------------------------------------------------------
    vtkIdType nbElementsToRemoveFromHead = 0;
    vtkIdType localOffset = 0;
    vtkIdType nbElementsInBar = 0;
    this->SearchGlobalIndexLocation((block * blockSize), this->LocalSorter->Histo,
      this->GlobalHistogram, nbElementsToRemoveFromHead, localOffset, nbElementsInBar);

    // ------------------------------------------------------------------------
    // Search for upper bound
    // ------------------------------------------------------------------------
    vtkIdType upperOffset = 0;
    vtkIdType globalUpperOffset = 0;
    vtkIdType searchIdx = (this->GlobalHistogram->TotalValues < (block + 1) * blockSize)
      ? this->GlobalHistogram->TotalValues
      : ((block + 1) * blockSize);
    searchIdx--; // It is not a size it is an index (so -1)

    this->SearchGlobalIndexLocation(searchIdx, this->LocalSorter->Histo, this->GlobalHistogram,
      globalUpperOffset, upperOffset, nbElementsInBar);

    // We have to include our searched index (so +1)
    vtkIdType localSize = (upperOffset + nbElementsInBar) - localOffset + 1;

    // ------------------------------------------------------------------------
    // Build local subset table
    // ------------------------------------------------------------------------
    vtkSmartPointer<vtkTable> localSubset;
    localSubset.TakeReference(
      this->NewSubsetTable(input, this->LocalSorter, localOffset, localSize));

    // ------------------------------------------------------------------------
    // Find the process that will merge all subset table
    // ------------------------------------------------------------------------
    int mergePid = GetMergingProcessId(localSubset.GetPointer());

    // ------------------------------------------------------------------------
    // Add local meta-data in case of merge process
    // ------------------------------------------------------------------------
    if (this->NumProcs > 1 && this->Me == mergePid)
    {
      vtkSmartPointer<vtkIdTypeArray> processIdArray = vtkSmartPointer<vtkIdTypeArray>::New();
      processIdArray->SetName("vtkOriginalProcessIds");
      processIdArray->SetNumberOfComponents(1);
      processIdArray->Allocate((blockSize < localSize) ? localSize : blockSize);
      for (vtkIdType idx = 0; idx < localSubset->GetNumberOfRows(); idx++)
      {
        processIdArray->InsertNextTuple1(mergePid);
      }
      localSubset->GetRowData()->AddArray(processIdArray);
    }

    // ------------------------------------------------------------------------
    // Send local subset array to process mergePid
    // ------------------------------------------------------------------------
    if (this->Me != mergePid)
    {
      this->MPI->Send(localSubset.GetPointer(), mergePid, VTK_TABLE_EXCHANGE_TAG);
    }

    // ------------------------------------------------------------------------
    // Merging procedure only on process mergePid
    // ------------------------------------------------------------------------
    if (this->Me == mergePid)
    {
      vtkSmartPointer<vtkTable> tmp = vtkSmartPointer<vtkTable>::New();
      for (int i = 0; i < this->NumProcs; i++)
      {
        if (i == mergePid)
          continue;

        this->MPI->Receive(tmp.GetPointer(), i, VTK_TABLE_EXCHANGE_TAG);
        this->MergeTable(i, tmp.GetPointer(), localSubset.GetPointer(), blockSize);
      }

      // Sort new table/array
      if (!this->DataToSort)
      {
        // This mean that no output can be provided
        // cout << "ERROR the merge process have no DataArray." << endl;
        return 1;
      }
      vtkDataArray* subsetArray =
        vtkDataArray::SafeDownCast(localSubset->GetColumnByName(this->DataToSort->GetName()));

      if (!subsetArray)
      {
        vtkSortedTableStreamer::PrintInfo(localSubset.GetPointer());
      }

      ArraySorter sorter;
      sorter.Update(static_cast<T*>(subsetArray->GetVoidPointer(0)),
        subsetArray->GetNumberOfTuples(), subsetArray->GetNumberOfComponents(),
        this->SelectedComponent, HISTOGRAM_SIZE, this->CommonRange, revertOrder);

      // trim it (remove head and tail that don't belong to the result)
      localSubset.TakeReference(this->NewSubsetTable(
        localSubset.GetPointer(), &sorter, nbElementsToRemoveFromHead, blockSize));

      // Add extra information such as structured indices, block number...
      this->DecorateTable(input, localSubset.GetPointer(), mergePid);

      // ShallowCopy it to the output
      output->ShallowCopy(localSubset.GetPointer());
    }
    else
    {
      // Ask other processes to provide metadata for table decoration
      this->DecorateTable(input, NULL, mergePid);
    }

    return 1;
  }

  // --------------------------------------------------------------------------
  // nbGlobalToSkip is the number of elements that should be skipped at the end
  // if you exactly want to reach the searchedGlobalIndex.
  // localOffset is the corresponding local index in the sorted table to the
  // global index of (searchedGlobalIndex - nbGlobalToSkip)
  // nbInLocalBar is the local number of elements that are available
  // in the histogram bar where the searchedGlobalIndex has been found.
  // nbInLocalBar is used when you want to get an upper bound that
  // will include the searchedGlobalIndex.
  void SearchGlobalIndexLocation(vtkIdType searchedGlobalIndex, Histogram* localHistogram,
    Histogram* globalHistogram, vtkIdType& nbGlobalToSkip, vtkIdType& localOffset,
    vtkIdType& nbInLocalBar)
  {
    // Communication buffer
    vtkIdType* bufferHistogramValues = new vtkIdType[this->NumProcs * HISTOGRAM_SIZE];

    // Local internal working var
    vtkIdType histogramBarIdx = -1;
    double currentRange[2];
    vtkIdType idx, idxEnd;

    // Setup initial histogram range and values
    Histogram _globalHistogram;
    Histogram _localHistogram;
    localHistogram->CopyTo(_localHistogram);
    globalHistogram->CopyTo(_globalHistogram);

    // Reset local offsets
    localOffset = 0;
    nbGlobalToSkip = searchedGlobalIndex;

    do
    {
      nbGlobalToSkip -= _globalHistogram.GetNewRange(nbGlobalToSkip, histogramBarIdx, currentRange);

      localOffset += _localHistogram.GetNumberOfElements(0, histogramBarIdx);
      nbInLocalBar = _localHistogram.GetNumberOfElements(histogramBarIdx, histogramBarIdx + 1);

      // Based on a narrowed range, build local histogram to get closer to the
      // real index of the object
      _localHistogram.SetScalarRange(currentRange);
      _localHistogram.ClearHistogramValues();

      idxEnd = localOffset + nbInLocalBar;
      for (idx = localOffset; idx < idxEnd; ++idx)
      {
        _localHistogram.AddValue(this->LocalSorter->Array[idx].Value);
      }

      // Exchange local histo with everyone
      this->MPI->AllGather(_localHistogram.Values, bufferHistogramValues, HISTOGRAM_SIZE);

      // Build global histogram
      _globalHistogram.SetScalarRange(currentRange);
      _globalHistogram.ClearHistogramValues();

      idxEnd = this->NumProcs * HISTOGRAM_SIZE;
      for (idx = 0; idx < idxEnd; ++idx)
      {
        _globalHistogram.TotalValues += bufferHistogramValues[idx];
        _globalHistogram.Values[idx % HISTOGRAM_SIZE] += bufferHistogramValues[idx];
      }
    } while (nbGlobalToSkip > 0 && _globalHistogram.CanBeReduced());

    delete[] bufferHistogramValues;
  }

  // --------------------------------------------------------------------------
  static vtkTable* NewSubsetTable(
    vtkTable* srcTable, ArraySorter* sorter, vtkIdType offset, vtkIdType size)
  {
    vtkTable* subTable = vtkTable::New();

    // Loop on all column of the table
    for (vtkIdType colIdx = 0; colIdx < srcTable->GetNumberOfColumns(); ++colIdx)
    {
      vtkAbstractArray* srcArray = srcTable->GetColumn(colIdx);

      // Manage subset items
      vtkAbstractArray* subArray = srcArray->NewInstance();
      subArray->SetNumberOfComponents(srcArray->GetNumberOfComponents());
      subArray->SetName(srcArray->GetName());
      subArray->Allocate(size * srcArray->GetNumberOfComponents());
      if (auto sinfo = srcArray->GetInformation())
      {
        subArray->CopyInformation(sinfo);
      }

      vtkIdType max = size + offset;
      if (sorter != NULL && sorter->Array != NULL)
      {
        max = (max > sorter->ArraySize) ? sorter->ArraySize : max;
        for (vtkIdType idx = offset; idx < max; ++idx)
        {
          if (subArray->InsertNextTuple(sorter->Array[idx].OriginalIndex, srcArray) == -1)
          {
            cout << "ERROR NewSubsetTable::InsertNextTuple is not working." << endl;
          }
        }
      }
      else
      {
        max = (max > srcTable->GetNumberOfRows()) ? srcTable->GetNumberOfRows() : max;
        for (vtkIdType idx = offset; idx < max; ++idx)
        {
          if (subArray->InsertNextTuple(idx, srcArray) == -1)
          {
            cout << "ERROR NewSubsetTable::InsertNextTuple is not working." << endl;
          }
        }
      }
      subTable->GetRowData()->AddArray(subArray);
      subArray->FastDelete();
    }

    // Return the new subset vtkTable
    return subTable;
  }

  // --------------------------------------------------------------------------
  void SetSelectedComponent(int newValue) override
  {
    if (this->SelectedComponent != newValue)
    {
      this->InvalidateCache();
      this->SelectedComponent = newValue;
    }
  }

  // --------------------------------------------------------------------------
  void InvalidateCache() override { this->NeedToBuildCache = true; }

  // --------------------------------------------------------------------------
  bool IsInvalid(vtkTable* input, vtkDataArray* dataToProcess) override
  {
    return !dataToProcess || input->GetMTime() != this->InputMTime ||
      dataToProcess->GetMTime() != this->DataMTime;
  }

  // --------------------------------------------------------------------------
  void DecorateTable(vtkTable* input, vtkTable* output, int mergePid)
  {
    // Check if it is a structured grid
    if (input->GetFieldData()->GetArray("STRUCTURED_DIMENSIONS"))
    {
      int localDimensions[3] = { 0, 0, 0 };
      int* dimensions = new int[3 * this->NumProcs];
      vtkIntArray::SafeDownCast(input->GetFieldData()->GetArray("STRUCTURED_DIMENSIONS"))
        ->GetTypedTuple(0, localDimensions);

      this->MPI->Gather(localDimensions, dimensions, 3, mergePid);

      if (output)
      {
        vtkIdTypeArray* structuredIndices = vtkIdTypeArray::New();
        structuredIndices->SetNumberOfComponents(3);
        structuredIndices->Allocate(output->GetNumberOfRows() * 3);
        structuredIndices->SetName("Structured Coordinates");

        vtkIdTypeArray* idsArray =
          vtkIdTypeArray::SafeDownCast(output->GetColumnByName("vtkOriginalIndices"));
        vtkIdTypeArray* pidsArray =
          vtkIdTypeArray::SafeDownCast(output->GetColumnByName("vtkOriginalProcessIds"));

        for (vtkIdType idx = 0; idx < output->GetNumberOfRows(); idx++)
        {
          vtkIdType id = idsArray->GetValue(idx);
          vtkIdType pid = pidsArray ? pidsArray->GetValue(idx) : 0;

          //          if(pid < 0 || pid >= this->NumProcs)
          //            {
          //            cout << "Error invalid pid " << pid << " at idx " << idx << " mergePid " <<
          //            mergePid << " me " << this->Me << endl;
          //            WaitForGDB();
          //            }

          structuredIndices->InsertNextTuple3((id % dimensions[3 * pid]),
            (id / dimensions[3 * pid]) % dimensions[3 * pid + 1],
            (id / (dimensions[3 * pid] * dimensions[3 * pid + 1])));
        }
        output->GetRowData()->AddArray(structuredIndices);
        structuredIndices->FastDelete();
      }
      delete[] dimensions;
    }
  }
  // --------------------------------------------------------------------------
  bool TestInternalClasses() override
  {
    cout << "vtkSortedTableStreamer::TestInternalClasses()" << endl;

    vtkSmartPointer<vtkTable> input = vtkSmartPointer<vtkTable>::New();
    vtkSmartPointer<vtkDoubleArray> dataA = vtkSmartPointer<vtkDoubleArray>::New();
    dataA->SetName("A");
    dataA->SetNumberOfComponents(1);
    vtkSmartPointer<vtkDoubleArray> dataB = vtkSmartPointer<vtkDoubleArray>::New();
    dataB->SetName("B");
    dataB->SetNumberOfComponents(3);

    // Fill data with values
    for (int i = 0; i < 2048; i++) // 2048 is bigger than histogram
    {
      dataA->InsertNextTuple1(vtkMath::Random());
      dataB->InsertNextTuple3(vtkMath::Random(), vtkMath::Random(), vtkMath::Random());
    }

    input->GetRowData()->AddArray(dataA.GetPointer());
    input->GetRowData()->AddArray(dataB.GetPointer());

    Histogram histPart1(100);
    Histogram histPart2(100);
    Histogram histMerge(100);
    histPart1.SetScalarRange(dataA->GetRange());
    histPart2.SetScalarRange(dataA->GetRange());
    histMerge.SetScalarRange(dataA->GetRange());
    for (vtkIdType i = 0; i < dataA->GetNumberOfTuples(); i++)
    {
      if (i < 1024)
      {
        histPart1.AddValue(dataA->GetValue(i));
      }
      else
      {
        histPart2.AddValue(dataA->GetValue(i));
      }
    }

    // Make sure that no values have been skipped while adding them
    if (histPart1.TotalValues + histPart2.TotalValues != dataA->GetNumberOfTuples())
    {
      cout << "Invalid number of elements in the histogram. Expected " << dataA->GetNumberOfTuples()
           << " and got " << (histPart1.TotalValues + histPart2.TotalValues) << endl;
      return false;
    }

    // Make sure that no values have been skipped while merging histo
    histMerge.Merge(histPart1);
    histMerge.Merge(histPart2);
    if (histMerge.TotalValues != dataA->GetNumberOfTuples())
    {
      cout << "Invalid number of elements in the histogram. Expected " << dataA->GetNumberOfTuples()
           << " and got " << (histMerge.TotalValues) << endl;
      return false;
    }

    cout << "Histogram ok" << endl;

    // Try to sort array
    ArraySorter sortedArray;
    sortedArray.Update(static_cast<T*>(dataA->GetVoidPointer(0)), dataA->GetNumberOfTuples(),
      dataA->GetNumberOfComponents(), 0, 100, dataA->GetRange(), false);

    double min = dataA->GetRange()[0];
    double max = dataA->GetRange()[1];

    if (sortedArray.ArraySize != dataA->GetNumberOfTuples())
    {
      cout << "Invalid sorted array size. Expected " << dataA->GetNumberOfTuples() << " and got "
           << sortedArray.ArraySize << endl;
      return false;
    }

    if (sortedArray.Array[0].Value != min)
    {
      cout << "The min is not the first element in the array. Expected: " << min << " and got "
           << sortedArray.Array[0].Value << endl;
      return false;
    }

    if (sortedArray.Array[sortedArray.ArraySize - 1].Value != max)
    {
      cout << "The max is not the first element in the array. Expected: " << max << " and got "
           << sortedArray.Array[sortedArray.ArraySize - 1].Value << endl;
      return false;
    }

    // Reserse order
    sortedArray.Update(static_cast<T*>(dataA->GetVoidPointer(0)), dataA->GetNumberOfTuples(),
      dataA->GetNumberOfComponents(), 0, 100, dataA->GetRange(), true);

    if (sortedArray.ArraySize != dataA->GetNumberOfTuples())
    {
      cout << "Invalid sorted array size. Expected " << dataA->GetNumberOfTuples() << " and got "
           << sortedArray.ArraySize << endl;
      return false;
    }

    if (sortedArray.Array[0].Value != max)
    {
      cout << "The max is not the first element in the array. Expected: " << max << " and got "
           << sortedArray.Array[0].Value << endl;
      return false;
    }

    if (sortedArray.Array[sortedArray.ArraySize - 1].Value != min)
    {
      cout << "The min is not the first element in the array. Expected: " << min << " and got "
           << sortedArray.Array[sortedArray.ArraySize - 1].Value << endl;
      return false;
    }

    cout << "ArraySorter ok [" << dataA->GetRange()[0] << ", " << dataA->GetRange()[1] << "]"
         << endl;

    return true;
  }
  // --------------------------------------------------------------------------
  int GetMergingProcessId(vtkTable* localTable)
  {
    if (this->NumProcs == 1)
      return 0;
    vtkIdType* tableSizes = new vtkIdType[this->NumProcs];
    vtkIdType localTableSize = localTable ? localTable->GetNumberOfRows() : 0;
    this->MPI->AllGather(&localTableSize, tableSizes, 1);
    vtkIdType maxSize = 0;
    int winner = 0;
    for (int pid = 0; pid < this->NumProcs; pid++)
    {
      if (maxSize < tableSizes[pid])
      {
        maxSize = tableSizes[pid];
        winner = pid;
      }
    }
    delete[] tableSizes;
    return winner;
  }
  // --------------------------------------------------------------------------
private:
  vtkMTimeType InputMTime;    // Keep the original input MTime
  vtkMTimeType DataMTime;     // Keep the original data MTime
  vtkDataArray* DataToSort;   // DataArray to sort
  ArraySorter* LocalSorter;   // Local ArraySorter based on global range
  Histogram* GlobalHistogram; // Globaly merged Histogram based on global range
  double CommonRange[2];      // Scalar range used across processes
  int Me;                     // Current process ID
  int NumProcs;               // Number of processes involved
  vtkCommunicator* MPI;       // MPI communicator to send/receive/gather
  int SelectedComponent;      // Component used to sort array
  bool NeedToBuildCache;
  bool Debug;

  const static int VTK_TABLE_EXCHANGE_TAG = 50;
  // HISTOGRAM_SIZE could be computed dynamically based on the type of the
  // array to sort but to make sure that unsigned char won't be distributed
  // correctly we set the histogram size to be their max number of element
  // type.
  // Maybe make some test on huge cluster to see which histogram size is
  // the best.
  const static int HISTOGRAM_SIZE = 256;
};
//****************************************************************************
vtkStandardNewMacro(vtkSortedTableStreamer);
vtkCxxSetObjectMacro(vtkSortedTableStreamer, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkSortedTableStreamer::vtkSortedTableStreamer()
{
  this->InvertOrder = 0;
  this->Controller = 0;
  this->SetNumberOfInputPorts(1);
  this->ColumnToSort = 0;
  this->SetColumnToSort("");
  this->Block = 0;
  this->BlockSize = 1024;
  this->Internal = 0;
  this->SelectedComponent = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkSortedTableStreamer::~vtkSortedTableStreamer()
{
  this->SetColumnToSort(0);
  this->SetController(0);
  if (this->Internal)
  {
    delete this->Internal;
    this->Internal = 0;
  }
}

//----------------------------------------------------------------------------
int vtkSortedTableStreamer::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkTable> vtkSortedTableStreamer::MergeBlocks(vtkCompositeDataSet* cd)
{
  auto result = vtkSmartPointer<vtkTable>::New();

  // Iterate over the input to build a single vtkTable to process
  vtkCompositeDataIterator* iter = cd->NewIterator();
  int blockIdx = 0;
  const vtkIdType allocationSize = cd->GetNumberOfElements(vtkDataObject::ROW);
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); blockIdx++, iter->GoToNextItem())
  {
    if (auto other = vtkTable::SafeDownCast(iter->GetCurrentDataObject()))
    {
      InternalsBase::MergeTable(-1, other, result.GetPointer(), allocationSize);
    }
    else if (auto dobj = iter->GetCurrentDataObject())
    {
      vtkWarningMacro(
        "Incompatible data type in the multiblock: " << dobj->GetClassName() << " " << blockIdx);
    }
  }
  iter->Delete();
  return result;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkUnsignedIntArray> vtkSortedTableStreamer::GenerateCompositeIndexArray(
  vtkCompositeDataSet* cd, vtkIdType maxSize)
{
  vtkNew<vtkUnsignedIntArray> compositeIndex;
  compositeIndex->SetName("vtkCompositeIndexArray");

  vtkCompositeDataIterator* iter = cd->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    auto table = vtkTable::SafeDownCast(iter->GetCurrentDataObject());
    if (!table)
    {
      continue;
    }

    auto metadata = iter->GetCurrentMetaData();
    const bool is_amr_info = metadata->Has(vtkSelectionNode::HIERARCHICAL_LEVEL()) &&
      metadata->Has(vtkSelectionNode::HIERARCHICAL_INDEX());
    const int num_components = is_amr_info ? 2 : 1;
    if (compositeIndex->GetNumberOfTuples() == 0)
    {
      compositeIndex->SetNumberOfComponents(num_components);
      compositeIndex->Allocate(num_components * maxSize);
    }
    assert(num_components == compositeIndex->GetNumberOfComponents());

    if (is_amr_info)
    {
      const unsigned int tuple[2] = { static_cast<unsigned int>(
                                        metadata->Get(vtkSelectionNode::HIERARCHICAL_LEVEL())),
        static_cast<unsigned int>(metadata->Get(vtkSelectionNode::HIERARCHICAL_INDEX())) };
      for (vtkIdType cc = 0, max = table->GetNumberOfRows(); cc < max; ++cc)
      {
        compositeIndex->InsertNextTypedTuple(tuple);
      }
    }
    else if (metadata->Has(vtkSelectionNode::COMPOSITE_INDEX()))
    {
      const unsigned int composite_index = metadata->Get(vtkSelectionNode::COMPOSITE_INDEX());
      for (vtkIdType cc = 0, max = table->GetNumberOfRows(); cc < max; ++cc)
      {
        compositeIndex->InsertNextTypedTuple(&composite_index);
      }
    }
  }
  iter->Delete();
  return compositeIndex;
}

//----------------------------------------------------------------------------
std::pair<vtkSmartPointer<vtkStringArray>, vtkSmartPointer<vtkIdTypeArray> >
vtkSortedTableStreamer::GenerateBlockNameArray(vtkCompositeDataSet* input, vtkIdType maxSize)
{
  vtkNew<vtkIdTypeArray> nameIndices;
  nameIndices->Allocate(maxSize);
  nameIndices->SetName("vtkBlockNameIndices");

  std::map<std::string, vtkIdType> nameMap;

  if (auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(input))
  {
    for (unsigned int cc = 0, max = pdc->GetNumberOfPartitionedDataSets(); cc < max; ++cc)
    {
      std::string name = "dataset_" + std::to_string(cc);
      if (pdc->HasMetaData(cc) && pdc->GetMetaData(cc)->Has(vtkCompositeDataSet::NAME()))
      {
        name = pdc->GetMetaData(cc)->Get(vtkCompositeDataSet::NAME());
      }
      vtkIdType index = 0;
      auto iter = nameMap.find(name);
      if (iter == nameMap.end())
      {
        index = static_cast<vtkIdType>(nameMap.size());
        nameMap[name] = index;
      }
      else
      {
        index = iter->second;
      }

      if (auto ds = pdc->GetPartitionedDataSet(cc))
      {
        const vtkIdType count = ds->GetNumberOfElements(vtkDataObject::ROW);
        for (vtkIdType jj = 0; jj < count; ++jj)
        {
          nameIndices->InsertNextValue(index);
        }
      }
    }
  }
  else
  {
    auto iter = input->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      auto table = vtkTable::SafeDownCast(iter->GetCurrentDataObject());
      if (table)
      {
        std::string name = "dataset_" + std::to_string(iter->GetCurrentFlatIndex());
        if (iter->HasCurrentMetaData() &&
          iter->GetCurrentMetaData()->Has(vtkCompositeDataSet::NAME()))
        {
          name = iter->GetCurrentMetaData()->Get(vtkCompositeDataSet::NAME());
        }

        vtkIdType index = 0;
        auto name_iter = nameMap.find(name);
        if (name_iter == nameMap.end())
        {
          index = static_cast<vtkIdType>(nameMap.size());
          nameMap[name] = index;
        }
        else
        {
          index = name_iter->second;
        }

        const vtkIdType count = table->GetNumberOfRows();
        for (vtkIdType cc = 0; cc < count; ++cc)
        {
          nameIndices->InsertNextValue(index);
        }
      }
    }
    iter->Delete();
  }

  vtkNew<vtkStringArray> names;
  names->SetName("vtkBlockNames");
  names->SetNumberOfTuples(static_cast<vtkIdType>(nameMap.size()));
  for (const auto& pair : nameMap)
  {
    names->SetValue(pair.second, pair.first);
  }

  return std::pair<vtkSmartPointer<vtkStringArray>, vtkSmartPointer<vtkIdTypeArray> >{ names,
    nameIndices };
}

//----------------------------------------------------------------------------
int vtkSortedTableStreamer::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Manage multiblock dataset by merging data into a single vtkTable
  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkSmartPointer<vtkTable> input = vtkTable::GetData(inputVector[0]);

  bool orderInverted = this->InvertOrder > 0;

  // Convert a composite dataset into a vtkTable input.
  if (auto inputCD = vtkCompositeDataSet::SafeDownCast(inputDO))
  {
    input = this->MergeBlocks(inputCD);
    if (input->GetColumnByName("vtkCompositeIndexArray") == nullptr)
    {
      auto array = this->GenerateCompositeIndexArray(inputCD, input->GetNumberOfRows());
      input->GetRowData()->AddArray(array);
    }
    if (input->GetColumnByName("vtkBlockNameIndices") == nullptr)
    {
      // add name array.
      auto array_pair = this->GenerateBlockNameArray(inputCD, input->GetNumberOfRows());
      if (array_pair.first && array_pair.second)
      {
        input->GetRowData()->AddArray(array_pair.second);
        input->GetFieldData()->AddArray(array_pair.first);
      }
    }
  }

  // Get input data
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkTable* output = vtkTable::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataArray* arrayToProcess = this->GetDataArrayToProcess(input);

  // --------------------------------------------------------------------------
  // Caution: Because this filter can be used behind a cell/point extractor
  // based on a selection, the input can be empty and arrayToProcess can be NULL.
  // And this can happen on all processes if the selection do not select a
  // single point/cell.
  // --------------------------------------------------------------------------

  // Delete internal object if the input has change (table or array to sort)
  if (this->Internal && this->Internal->IsInvalid(input, arrayToProcess))
  {
    delete this->Internal;
    this->Internal = 0;
  }

  // Make sure that an internal object is available
  this->CreateInternalIfNeeded(input, arrayToProcess);
  int realComponent =
    (!arrayToProcess) ? 0 : this->GetSelectedComponent() % arrayToProcess->GetNumberOfComponents();
  this->Internal->SetSelectedComponent(realComponent);

  // Manage custom case where sorting occur on a virtual array (process id)
  if (!this->Internal->IsSortable() ||
    (this->GetColumnToSort() && (strcmp("vtkOriginalProcessIds", this->GetColumnToSort()) == 0)))
  {
    this->Internal->Extract(input, output, this->Block, this->BlockSize, orderInverted);
  }
  else
  {
    this->Internal->Compute(input, output, this->Block, this->BlockSize, orderInverted);
  }

  if (auto names = input->GetFieldData()->GetAbstractArray("vtkBlockNames"))
  {
    output->GetFieldData()->AddArray(names);
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkSortedTableStreamer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Sorting column: " << (this->ColumnToSort ? this->ColumnToSort : "(none)")
     << endl;
}

//----------------------------------------------------------------------------
const char* vtkSortedTableStreamer::GetColumnNameToSort()
{
  return this->GetColumnToSort();
}
//----------------------------------------------------------------------------
void vtkSortedTableStreamer::SetColumnNameToSort(const char* columnName)
{
  this->SetColumnToSort(columnName);
  if (strcmp("vtkOriginalProcessIds", this->GetColumnToSort()) != 0)
  {
    if (this->Internal)
    {
      delete this->Internal;
      this->Internal = 0;
    }
  }
}
//----------------------------------------------------------------------------
void vtkSortedTableStreamer::SetInvertOrder(int newValue)
{
  bool changed = this->InvertOrder != newValue;
  bool removeInternal = changed && strcmp("vtkOriginalProcessIds", this->GetColumnToSort()) != 0;

  if (removeInternal && this->Internal)
  {
    delete this->Internal;
    this->Internal = 0;
  }

  if (changed)
  {
    this->InvertOrder = newValue;
    this->Modified();
  }
}
//----------------------------------------------------------------------------
vtkDataArray* vtkSortedTableStreamer::GetDataArrayToProcess(vtkTable* input)
{
  // Get a default array to sort just in case
  vtkDataArray* requestedArray = 0;

  if (this->GetColumnToSort())
  {
    requestedArray = vtkDataArray::SafeDownCast(input->GetColumnByName(this->GetColumnToSort()));
  }
  return requestedArray;
}

//----------------------------------------------------------------------------
void vtkSortedTableStreamer::CreateInternalIfNeeded(vtkTable* input, vtkDataArray* data)
{
  if (!this->Internal)
  {
    if (data)
    {
      switch (data->GetDataType())
      {
        vtkTemplateMacro(
          this->Internal = new Internals<VTK_TT>(input, data, this->GetController()););
        default:
          vtkErrorMacro("Array type not supported: " << data->GetClassName());
      }
    }
    else
    {
      // Provide an empty data
      this->Internal = new Internals<double>(input, 0, this->GetController());
    }
  }
}
//----------------------------------------------------------------------------
void vtkSortedTableStreamer::PrintInfo(vtkTable* input)
{
  ostringstream stream;
  stream << "Process " << vtkMultiProcessController::GetGlobalController()->GetLocalProcessId()
         << endl
         << " - Table as " << input->GetNumberOfRows() << " rows and "
         << input->GetNumberOfColumns() << " columns" << endl
         << " - Column names:";
  for (int i = 0; i < input->GetNumberOfColumns(); i++)
  {
    stream << " " << input->GetColumn(i)->GetName();
  }
  stream << endl;

  cout << stream.str().c_str();
}
//----------------------------------------------------------------------------
bool vtkSortedTableStreamer::TestInternalClasses()
{
  Internals<double>* base = new Internals<double>();
  bool result = base->TestInternalClasses();
  delete base;
  return result;
}
