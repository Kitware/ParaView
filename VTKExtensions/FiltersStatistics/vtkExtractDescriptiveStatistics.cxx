// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2025 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkExtractDescriptiveStatistics.h"

#include "vtkDataAssembly.h"
#include "vtkDataAssemblyVisitor.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkStatisticalModel.h"
#include "vtkStringArray.h"
#include "vtkStringFormatter.h"
#include "vtkTable.h"

#include "vtksys/SystemTools.hxx"

VTK_ABI_NAMESPACE_BEGIN

namespace
{

void AddModelToAssembly(vtkPartitionedDataSetCollection* out, vtkStatisticalModel* model,
  vtkDataAssembly* assy, int rootAssyNode)
{
  if (!model || model->IsEmpty())
  {
    return;
  }
  std::string params = model->GetAlgorithmParameters() ? model->GetAlgorithmParameters() : "";
  if (params.find("vtkDescriptiveStatistics") != 0)
  {
    vtkWarningWithObjectMacro(model, "Expected descriptive statistics, got " << params << ".");
    return;
  }
  if (model->GetNumberOfTables(vtkStatisticalModel::TableType::Learned) != 1 ||
    model->GetNumberOfTables(vtkStatisticalModel::TableType::Derived) != 1)
  {
    vtkErrorWithObjectMacro(model, "Learned or derived tables were missing.");
    return;
  }

  unsigned int nextPartition = out->GetNumberOfPartitionedDataSets();

  // We expect 1 or 2 tables: 1 learned table and 0 or 1 derived tables.
  vtkTable* learned = model->GetTable(vtkStatisticalModel::TableType::Learned, 0);
  vtkTable* derived = model->GetTable(vtkStatisticalModel::TableType::Derived, 0);
  vtkNew<vtkTable> summary;
  auto* var = learned->GetColumnByName("Variable");
  vtkNew<vtkStringArray> att;
  att->DeepCopy(var);
  att->SetName("Attribute");
  summary->AddColumn(att);
  summary->AddColumn(learned->GetColumnByName("Minimum"));
  summary->AddColumn(learned->GetColumnByName("Mean"));
  summary->AddColumn(learned->GetColumnByName("Maximum"));
  summary->AddColumn(derived->GetColumnByName("Standard Deviation"));
  summary->AddColumn(derived->GetColumnByName("Variance"));
  summary->AddColumn(derived->GetColumnByName("Skewness"));
  summary->AddColumn(derived->GetColumnByName("Kurtosis"));
  summary->AddColumn(learned->GetColumnByName("Cardinality"));
  summary->AddColumn(derived->GetColumnByName("Sum"));
  unsigned int dsidx = nextPartition++;
  out->SetPartition(dsidx, 0, summary);
  out->GetMetaData(dsidx)->Set(vtkCompositeDataSet::NAME(), assy->GetNodeName(rootAssyNode));
  assy->AddDataSetIndex(rootAssyNode, dsidx);
}

class ModelExtractor : public vtkDataAssemblyVisitor
{
public:
  static ModelExtractor* New();
  vtkTypeMacro(ModelExtractor, vtkDataAssemblyVisitor);

  vtkPartitionedDataSetCollection* In;
  vtkPartitionedDataSetCollection* Out;
  vtkDataAssembly* AssemblyOut;

  void Initialize(vtkPartitionedDataSetCollection* pdc, vtkPartitionedDataSetCollection* out,
    vtkDataAssembly* resultAssy)
  {
    this->In = pdc;
    this->Out = out;
    this->AssemblyOut = resultAssy;
  }

  void Visit(int nodeId) override
  {
    auto indices = this->GetCurrentDataSetIndices();
    int parentNode = nodeId;
    int midIdx = indices.size() > 1 ? 0 : -1;
    for (const auto& index : indices)
    {
      auto* pd = this->In->GetPartitionedDataSet(index);
      if (!pd || pd->GetNumberOfPartitions() == 0)
      {
        continue;
      }
      unsigned int np = pd->GetNumberOfPartitions();
      if (np > 1 && midIdx < 0)
      {
        midIdx = 0;
      }
      // Loop over partitions. If any is a statistical model, add its tables.
      for (unsigned int ii = 0; ii < np; ++ii)
      {
        if (auto* model = vtkStatisticalModel::SafeDownCast(pd->GetPartitionAsDataObject(ii)))
        {
          if (midIdx >= 0)
          {
            std::string midNodeName = vtk::to_string(midIdx++);
            parentNode = this->AssemblyOut->AddNode(midNodeName.c_str(), nodeId);
          }
          AddModelToAssembly(this->Out, model, this->AssemblyOut, parentNode);
        }
      }
    }
  }
};

} // anonymous namespace

vtkStandardNewMacro(vtkExtractDescriptiveStatistics);
vtkStandardNewMacro(ModelExtractor);

vtkExtractDescriptiveStatistics::vtkExtractDescriptiveStatistics() = default;

vtkExtractDescriptiveStatistics::~vtkExtractDescriptiveStatistics()
{
  this->SetController(nullptr);
}

void vtkExtractDescriptiveStatistics::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int vtkExtractDescriptiveStatistics::FillInputPortInformation(int port, vtkInformation* info)
{
  (void)port;
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkStatisticalModel");
  return 1;
}

int vtkExtractDescriptiveStatistics::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inInfoVec, vtkInformationVector* outInfoVec)
{
  auto* model = vtkStatisticalModel::GetData(inInfoVec[0]);
  auto* pdc = vtkPartitionedDataSetCollection::GetData(inInfoVec[0]);
  auto* out = vtkPartitionedDataSetCollection::GetData(outInfoVec);

  if (pdc && (pdc->GetNumberOfPartitionedDataSets() == 0 || !pdc->GetDataAssembly()))
  {
    // Empty input ⇒ empty output
    out->Initialize();
    return 1;
  }

  if (this->Controller && this->Controller->GetLocalProcessId() > 0)
  {
    // Only rank 0 should output tables; otherwise the spreadsheet view will display
    // multiple copies of the same model statistics.
    out->Initialize();
    return 1;
  }

  vtkNew<vtkDataAssembly> resultAssy;

  if (model)
  {
    AddModelToAssembly(out, model, resultAssy, /*root node of model*/ 0);
  }
  else if (pdc)
  {
    // Copy the input's structure so all node IDs match, but remove dataset references:
    auto* assemblyIn = pdc->GetDataAssembly();
    resultAssy->DeepCopy(assemblyIn);
    resultAssy->RemoveAllDataSetIndices(0, /*traverse_subtree*/ true);
    // Create a visitor for the input collection.
    vtkNew<ModelExtractor> extractor;
    extractor->Initialize(pdc, out, resultAssy);
    // Add new node IDs for statistical models by traversing the input:
    assemblyIn->Visit(extractor);
  }
  else
  {
    auto* data = vtkDataObject::GetData(inInfoVec[0]);
    vtkErrorMacro("Unhandled input type \"" << (data ? data->GetClassName() : "null") << "\".");
    return 0;
  }
  out->SetDataAssembly(resultAssy);

  return 1;
}

VTK_ABI_NAMESPACE_END
