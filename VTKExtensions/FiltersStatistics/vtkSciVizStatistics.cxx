#include "vtkSciVizStatistics.h"
#include "vtkSciVizStatisticsPrivate.h"

#include "vtkAlgorithm.h"
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSetAttributes.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkStatisticsAlgorithm.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <set>
#include <sstream>

vtkInformationKeyMacro(vtkSciVizStatistics, MULTIPLE_MODELS, Integer);

vtkSciVizStatistics::vtkSciVizStatistics()
{
  this->P = new vtkSciVizStatisticsP;
  this->AttributeMode = vtkDataObject::POINT;
  this->TrainingFraction = 0.1;
  this->Task = MODEL_AND_ASSESS;
  this->SetNumberOfInputPorts(2);  // data + optional model
  this->SetNumberOfOutputPorts(2); // model + assessed input
}

vtkSciVizStatistics::~vtkSciVizStatistics()
{
  delete this->P;
}

void vtkSciVizStatistics::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Task: " << this->Task << "\n";
  os << indent << "AttributeMode: " << this->AttributeMode << "\n";
  os << indent << "TrainingFraction: " << this->TrainingFraction << "\n";
}

int vtkSciVizStatistics::GetNumberOfAttributeArrays()
{
  vtkDataObject* dobj = this->GetInputDataObject(0, 0); // First input is always the leader
  if (!dobj)
  {
    return 0;
  }

  vtkFieldData* fdata = dobj->GetAttributesAsFieldData(this->AttributeMode);
  if (!fdata)
  {
    return 0;
  }

  return fdata->GetNumberOfArrays();
}

const char* vtkSciVizStatistics::GetAttributeArrayName(int n)
{
  vtkDataObject* dobj = this->GetInputDataObject(0, 0); // First input is always the leader
  if (!dobj)
  {
    return 0;
  }

  vtkFieldData* fdata = dobj->GetAttributesAsFieldData(this->AttributeMode);
  if (!fdata)
  {
    return 0;
  }

  int numArrays = fdata->GetNumberOfArrays();
  if (n < 0 || n > numArrays)
  {
    return 0;
  }

  vtkAbstractArray* arr = fdata->GetAbstractArray(n);
  if (!arr)
  {
    return 0;
  }

  return arr->GetName();
}

int vtkSciVizStatistics::GetAttributeArrayStatus(const char* arrName)
{
  return this->P->Has(arrName) ? 1 : 0;
}

void vtkSciVizStatistics::EnableAttributeArray(const char* arrName)
{
  if (arrName)
  {
    if (this->P->SetBufferColumnStatus(arrName, 1))
    {
      this->Modified();
    }
  }
}

void vtkSciVizStatistics::ClearAttributeArrays()
{
  if (this->P->ResetBuffer())
  {
    this->Modified();
  }
}

int vtkSciVizStatistics::FillInputPortInformation(int port, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  if (port == 0)
  {
    return 1;
  }
  else if (port >= 1 && port <= 2)
  {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    return 1;
  }
  return 0;
}

int vtkSciVizStatistics::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
    return 1;
  }
  return 0;
}

int vtkSciVizStatistics::ProcessRequest(
  vtkInformation* request, vtkInformationVector** input, vtkInformationVector* output)
{
  if (request && request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, input, output);
  }
  return this->Superclass::ProcessRequest(request, input, output);
}

int vtkSciVizStatistics::RequestDataObject(
  vtkInformation* vtkNotUsed(request), vtkInformationVector** input, vtkInformationVector* output)
{
  // Input 0: Data for learning/assessment.
  // If this is composite data, both outputs must be composite datasets with the same structure.
  vtkInformation* iinfo = input[0]->GetInformationObject(0);
  vtkDataObject* inData = iinfo->Get(vtkDataObject::DATA_OBJECT());
  vtkCompositeDataSet* inDataComp = vtkCompositeDataSet::SafeDownCast(inData);

  // Output 0: Model
  // The output model type must be a multiblock dataset
  vtkInformation* oinfom = output->GetInformationObject(0);
  vtkDataObject* ouModel = oinfom->Get(vtkDataObject::DATA_OBJECT());

  if (inDataComp)
  {
    vtkMultiBlockDataSet* mbModel = vtkMultiBlockDataSet::SafeDownCast(ouModel);
    if (!mbModel)
    {
      mbModel = vtkMultiBlockDataSet::New();
      oinfom->Set(vtkDataObject::DATA_OBJECT(), mbModel);
      oinfom->Set(vtkDataObject::DATA_EXTENT_TYPE(), mbModel->GetExtentType());
      mbModel->FastDelete();
    }
  }
  else
  {
    if (!ouModel || !ouModel->IsA("vtkMultiBlockDataSet"))
    {
      vtkMultiBlockDataSet* modelObj = vtkMultiBlockDataSet::New();
      oinfom->Set(vtkDataObject::DATA_OBJECT(), modelObj);
      oinfom->Set(vtkDataObject::DATA_EXTENT_TYPE(), modelObj->GetExtentType());
      modelObj->FastDelete();
    }
  }

  // Output 1: Assessed data
  // The assessed data output will always be a shallow copy of the input data.
  vtkInformation* oinfod = output->GetInformationObject(1);
  vtkDataObject* ouData = oinfod->Get(vtkDataObject::DATA_OBJECT());

  if (!ouData || !ouData->IsA(inData->GetClassName()))
  {
    ouData = inData->NewInstance();
    oinfod->Set(vtkDataObject::DATA_OBJECT(), ouData);
    // oinfod->Set( vtkDataObject::DATA_EXTENT_TYPE(), ouData->GetExtentType() );
    ouData->FastDelete();
    this->GetOutputPortInformation(1)->Set(
      vtkDataObject::DATA_EXTENT_TYPE(), ouData->GetExtentType());
  }
  return 1;
}

int vtkSciVizStatistics::RequestData(
  vtkInformation* vtkNotUsed(request), vtkInformationVector** input, vtkInformationVector* output)
{
  vtkDataObject* modelObjIn = vtkDataObject::GetData(input[1], 0);
  vtkDataObject* dataObjIn = vtkDataObject::GetData(input[0], 0);
  if (!dataObjIn)
  {
    // Silently ignore missing data.
    return 1;
  }

  if (!this->P->Buffer.size())
  {
    // Silently ignore empty requests.
    return 1;
  }

  // Get output model data and sci-viz data.
  vtkDataObject* modelObjOu = vtkDataObject::GetData(output, 0);
  vtkDataObject* dataObjOu = vtkDataObject::GetData(output, 1);
  if (!dataObjOu || !modelObjOu)
  {
    // Silently ignore missing data.
    return 1;
  }

  // Either we have a multiblock input dataset or a single data object of interest.
  int stat = 1;
  vtkCompositeDataSet* compDataObjIn = vtkCompositeDataSet::SafeDownCast(dataObjIn);
  if (compDataObjIn)
  {
    // I. Prepare output model containers
    vtkMultiBlockDataSet* ouModelRoot = vtkMultiBlockDataSet::SafeDownCast(modelObjOu);
    if (!ouModelRoot)
    {
      vtkErrorMacro(
        "Output model data object of incorrect type \"" << modelObjOu->GetClassName() << "\"");
      return 0;
    }
    // Copy the structure of the input dataset to the model output.
    // If we have input models in the proper structure, then we'll copy them into this structure
    // later.
    ouModelRoot->CopyStructure(compDataObjIn);
    ouModelRoot->GetInformation()->Set(MULTIPLE_MODELS(), 1);
  }
  else
  {
    modelObjOu->GetInformation()->Remove(MULTIPLE_MODELS());
  }

  // II. Create/update the output sci-viz data
  this->ShallowCopy(dataObjOu, dataObjIn);

  if (compDataObjIn)
  {
    // Loop over each data object of interest, calculating a model from and/or assessing it.
    vtkCompositeDataSet* compModelObjIn = vtkCompositeDataSet::SafeDownCast(modelObjIn);
    vtkCompositeDataSet* compModelObjOu = vtkCompositeDataSet::SafeDownCast(modelObjOu);
    vtkCompositeDataSet* compDataObjOu = vtkCompositeDataSet::SafeDownCast(dataObjOu);

    // We may have a single model for all blocks or one per block
    // This is too tricky to detect automagically (because a single model may be a composite
    // dataset),
    // so we'll only treat an input composite dataset as a collection of models if it is marked
    // as such. Otherwise, it is treated as a single model that is applied to each block.
    vtkDataObject* preModel =
      (compModelObjIn && compModelObjIn->GetInformation()->Has(MULTIPLE_MODELS()))
      ? 0
      : modelObjIn; // Pre-existing model. Initialize as if we have a single model.
    // Iterate over all blocks at the given hierarchy level looking for leaf nodes
    this->RequestData(compDataObjOu, compModelObjOu, compDataObjIn, compModelObjIn, preModel);
  }
  else
  {
    stat = this->RequestData(dataObjOu, modelObjOu, dataObjIn, modelObjIn);
  }

  return stat;
}

int vtkSciVizStatistics::RequestData(vtkCompositeDataSet* compDataOu,
  vtkCompositeDataSet* compModelOu, vtkCompositeDataSet* compDataIn,
  vtkCompositeDataSet* compModelIn, vtkDataObject* singleModel)
{
  if (!compDataOu || !compModelOu || !compDataIn)
  {
    vtkErrorMacro(<< "Mismatch between inputs and/or outputs."
                  << " Data in: " << compDataIn << " Model in: " << compModelIn
                  << " Data out: " << compDataOu << " Model out: " << compModelOu
                  << " Pre-existing model: " << singleModel);
    return 0;
  }

  int stat = 1;
  vtkCompositeDataIterator* inDataIter = compDataIn->NewIterator();
  vtkCompositeDataIterator* ouDataIter = compDataOu->NewIterator();
  vtkCompositeDataIterator* ouModelIter = compModelOu->NewIterator();

  // We may have a single model for all blocks or one per block
  vtkCompositeDataIterator* inModelIter = compModelIn ? compModelIn->NewIterator() : 0;
  vtkDataObject* currentModel = singleModel;

  if (vtkDataObjectTreeIterator::SafeDownCast(inDataIter))
  {
    vtkDataObjectTreeIterator* treeIter = vtkDataObjectTreeIterator::SafeDownCast(inDataIter);
    treeIter->VisitOnlyLeavesOff();
    treeIter->TraverseSubTreeOff();
  }
  // inDataIter->SkipEmptyNodesOff();

  if (vtkDataObjectTreeIterator::SafeDownCast(ouDataIter))
  {
    vtkDataObjectTreeIterator* treeIter = vtkDataObjectTreeIterator::SafeDownCast(ouDataIter);
    treeIter->VisitOnlyLeavesOff();
    treeIter->TraverseSubTreeOff();
  }
  // ouDataIter->SkipEmptyNodesOff();

  if (vtkDataObjectTreeIterator::SafeDownCast(ouModelIter))
  {
    vtkDataObjectTreeIterator* treeIter = vtkDataObjectTreeIterator::SafeDownCast(ouModelIter);
    treeIter->VisitOnlyLeavesOff();
    treeIter->TraverseSubTreeOff();
  }
  ouModelIter
    ->SkipEmptyNodesOff(); // Cannot skip since we may need to copy or create models as we go.

  if (inModelIter)
  {
    if (vtkDataObjectTreeIterator::SafeDownCast(inModelIter))
    {
      vtkDataObjectTreeIterator* treeIter = vtkDataObjectTreeIterator::SafeDownCast(inModelIter);
      treeIter->VisitOnlyLeavesOff();
      treeIter->TraverseSubTreeOff();
    }
    // inModelIter->SkipEmptyNodesOff();

    inModelIter->InitTraversal();
    currentModel = inModelIter->GetCurrentDataObject();
  }

  for (inDataIter->InitTraversal(), ouDataIter->InitTraversal(), ouModelIter->InitTraversal();
       !inDataIter->IsDoneWithTraversal();
       inDataIter->GoToNextItem(), ouDataIter->GoToNextItem(), ouModelIter->GoToNextItem())
  {
    vtkDataObject* inDataCur = inDataIter->GetCurrentDataObject();
    if (inDataCur && !inDataCur->IsA("vtkCompositeDataSet"))
    { // We have a leaf node
      vtkDataObject* ouModelCur = ouModelIter->GetCurrentDataObject();
      if (!ouModelCur)
      {
        vtkMultiBlockDataSet* mbModel = vtkMultiBlockDataSet::New();
        ouModelIter->GetDataSet()->SetDataSet(ouModelIter, mbModel);
        mbModel->Delete();
        ouModelCur = mbModel;
      }
      stat = this->RequestData(ouDataIter->GetCurrentDataObject(), ouModelCur,
        inDataIter->GetCurrentDataObject(), currentModel);
      if (!stat)
      {
        break;
      }
    }
    else if (inDataCur)
    { // Iterate over children
      stat =
        this->RequestData(vtkCompositeDataSet::SafeDownCast(ouDataIter->GetCurrentDataObject()),
          vtkCompositeDataSet::SafeDownCast(ouModelIter->GetCurrentDataObject()),
          vtkCompositeDataSet::SafeDownCast(inDataIter->GetCurrentDataObject()),
          inModelIter ? vtkCompositeDataSet::SafeDownCast(inModelIter->GetCurrentDataObject()) : 0,
          currentModel);
      if (!stat)
      {
        break;
      }
    }
    if (inModelIter)
    { // Update currentModel to point to the next input model in the tree
      inModelIter->GoToNextItem();
      currentModel = inModelIter->GetCurrentDataObject();
    }
  }
  inDataIter->Delete();
  ouDataIter->Delete();
  ouModelIter->Delete();
  if (inModelIter)
    inModelIter->Delete();

  return stat;
}

int vtkSciVizStatistics::RequestData(
  vtkDataObject* outData, vtkDataObject* outModel, vtkDataObject* inData, vtkDataObject* inModel)
{
  vtkFieldData* dataAttrIn = inData->GetAttributesAsFieldData(this->AttributeMode);
  if (!dataAttrIn)
  {
    // Silently ignore missing attributes.
    return 1;
  }

  // Create a table with all the data
  vtkNew<vtkTable> inTable;
  int stat = this->PrepareFullDataTable(inTable, dataAttrIn);
  if (stat < 1)
  { // return an error (stat=0) or success (stat=-1)
    return -stat;
  }

  // Either create or retrieve the model, depending on the task at hand
  if (this->Task != ASSESS_INPUT)
  {
    // We are creating a model by executing Learn and Derive operations on the input data
    // Create a table to hold the input data (unless the TrainingFraction is exactly 1.0)
    vtkSmartPointer<vtkTable> train = nullptr;
    vtkIdType N = inTable->GetNumberOfRows();
    vtkIdType M = this->Task == MODEL_INPUT ? N : this->GetNumberOfObservationsForTraining(inTable);
    if (M == N)
    {
      train = inTable;
      if (this->Task != MODEL_INPUT && this->TrainingFraction < 1.)
      {
        vtkWarningMacro(<< "Either TrainingFraction (" << this->TrainingFraction
                        << ") is high enough to include all observations after rounding"
                        << " or the minimum number of observations required for training is at "
                           "least the size of the entire input."
                        << " Any assessment will not be able to detect overfitting.");
      }
    }
    else
    {
      train = vtkSmartPointer<vtkTable>::New();
      this->PrepareTrainingTable(train, inTable, M);
    }

    // Calculate detailed statistical model from the input data set
    vtkMultiBlockDataSet* outModelDS = vtkMultiBlockDataSet::SafeDownCast(outModel);
    if (!outModelDS)
    {
      vtkErrorMacro("No model output dataset or incorrect type");
      stat = 0;
    }
    else
    {
      outModel->Initialize();
      stat = this->LearnAndDerive(outModelDS, train);
    }
  }
  else
  {
    // We are using an input model specified by the user
    // stat = this->FetchModel( outModel, input[1] ); // retrieves outModel from input[1]
    if (!inModel)
    {
      vtkErrorMacro("No input model");
      stat = 0;
    }
    outModel->ShallowCopy(inModel);
  }

  if (stat < 1)
  { // Exit on failure (0) or early success (-1)
    return -stat;
  }

  if (outData)
  {
    outData->ShallowCopy(inData);
  }
  if (this->Task != CREATE_MODEL && this->Task != MODEL_INPUT)
  {
    // Assess the data using the input or the just-created model
    vtkMultiBlockDataSet* outModelDS = vtkMultiBlockDataSet::SafeDownCast(outModel);
    if (!outModelDS)
    {
      vtkErrorMacro("No model output dataset or incorrect type");
      stat = 0;
    }
    else
    {
      stat = this->AssessData(inTable, outData, outModelDS);
    }
  }
  return stat ? 1 : 0;
}

int vtkSciVizStatistics::PrepareFullDataTable(vtkTable* inTable, vtkFieldData* dataAttrIn)
{
  std::set<vtkStdString>::iterator colIt;
  for (colIt = this->P->Buffer.begin(); colIt != this->P->Buffer.end(); ++colIt)
  {
    vtkAbstractArray* arr = dataAttrIn->GetAbstractArray(colIt->c_str());
    if (arr)
    {
      vtkIdType ntup = arr->GetNumberOfTuples();
      int ncomp = arr->GetNumberOfComponents();
      if (ncomp > 1)
      {
        // Create a column in the table for each component of non-scalar arrays requested.
        // FIXME: Should we add a "norm" column when arr is a vtkDataArray? It would make sense.
        std::vector<vtkAbstractArray*> comps;
        const char* compName;

        // Check component names can be used
        std::set<std::string> compCheckSet;
        bool useCompNames = true;
        for (int i = 0; i < ncomp; ++i)
        {
          compName = arr->GetComponentName(i);
          if (!compName || compCheckSet.count(compName) > 0)
          {
            useCompNames = false;
            break;
          }
          compCheckSet.emplace(compName);
        }

        for (int i = 0; i < ncomp; ++i)
        {
          std::ostringstream os;
          compName = arr->GetComponentName(i);
          os << arr->GetName() << "_";
          useCompNames ? os << compName : os << i;

          vtkAbstractArray* arrCol = vtkAbstractArray::CreateArray(arr->GetDataType());
          arrCol->SetName(os.str().c_str());
          arrCol->SetNumberOfComponents(1);
          arrCol->SetNumberOfTuples(ntup);
          comps.push_back(arrCol);
          inTable->AddColumn(arrCol);
          arrCol->FastDelete();
        }
        vtkIdType vidx = 0;
        vtkDataArray* darr = vtkDataArray::SafeDownCast(arr);
        vtkStringArray* sarr = vtkStringArray::SafeDownCast(arr);
        if (darr)
        {
          for (int i = 0; i < ncomp; ++i)
          {
            vtkDataArray::SafeDownCast(comps[i])->CopyComponent(0, darr, i);
          }
        }
        else if (sarr)
        {
          std::vector<vtkStringArray*> scomps;
          for (int i = 0; i < ncomp; ++i, ++vidx)
          {
            scomps[i] = vtkStringArray::SafeDownCast(comps[i]);
          }
          for (vtkIdType j = 0; j < ntup; ++j)
          {
            for (int i = 0; i < ncomp; ++i, ++vidx)
            {
              scomps[i]->SetValue(j, sarr->GetValue(vidx));
            }
          }
        }
        else
        {
          // Inefficient, but works for any array type.
          for (vtkIdType j = 0; j < ntup; ++j)
          {
            for (int i = 0; i < ncomp; ++i, ++vidx)
            {
              comps[i]->InsertVariantValue(j, arr->GetVariantValue(vidx));
            }
          }
        }
      }
      else
      {
        inTable->AddColumn(arr);
      }
    }
  }

  vtkIdType ncols = inTable->GetNumberOfColumns();
  if (ncols < 1)
  {
    vtkWarningMacro("Every requested array wasn't a scalar or wasn't present.");
    return -1;
  }

  return 1;
}

int vtkSciVizStatistics::PrepareTrainingTable(
  vtkTable* trainingTable, vtkTable* fullDataTable, vtkIdType M)
{
  // FIXME: this should eventually eliminate duplicate points as well as subsample...
  //        but will require the original ugrid/polydata/graph.
  std::set<vtkIdType> trainRows;
  vtkIdType N = fullDataTable->GetNumberOfRows();
  double frac = static_cast<double>(M) / static_cast<double>(N);
  for (vtkIdType i = 0; i < N; ++i)
  {
    if (vtkMath::Random() < frac)
    {
      trainRows.insert(i);
    }
  }
  // Now add or subtract entries as required.
  N = N - 1;
  while (static_cast<vtkIdType>(trainRows.size()) > M)
  {
    vtkIdType rec = static_cast<vtkIdType>(vtkMath::Random(0, N));
    trainRows.erase(rec);
  }
  while (static_cast<vtkIdType>(trainRows.size()) < M)
  {
    vtkIdType rec = static_cast<vtkIdType>(vtkMath::Random(0, N));
    trainRows.insert(rec);
  }
  // Finally, copy the subset into the training table
  trainingTable->Initialize();
  for (int i = 0; i < fullDataTable->GetNumberOfColumns(); ++i)
  {
    vtkAbstractArray* srcCol = fullDataTable->GetColumn(i);
    vtkAbstractArray* dstCol = vtkAbstractArray::CreateArray(srcCol->GetDataType());
    dstCol->SetName(srcCol->GetName());
    trainingTable->AddColumn(dstCol);
    dstCol->FastDelete();
  }
  trainingTable->SetNumberOfRows(M);
  vtkNew<vtkVariantArray> row;
  vtkIdType dstRow = 0;
  for (std::set<vtkIdType>::iterator it = trainRows.begin(); it != trainRows.end(); ++it, ++dstRow)
  {
    fullDataTable->GetRow(*it, row);
    trainingTable->SetRow(dstRow, row);
  }
  return 1;
}

vtkIdType vtkSciVizStatistics::GetNumberOfObservationsForTraining(vtkTable* observations)
{
  vtkIdType N = observations->GetNumberOfRows();
  vtkIdType M = static_cast<vtkIdType>(N * this->TrainingFraction);
  return M < 100 ? (N < 100 ? N : 100) : M;
}

void vtkSciVizStatistics::ShallowCopy(vtkDataObject* out, vtkDataObject* in)
{
  out->ShallowCopy(in);
  vtkCompositeDataSet* cdIn = vtkCompositeDataSet::SafeDownCast(in);
  vtkCompositeDataSet* cdOut = vtkCompositeDataSet::SafeDownCast(out);
  if (cdIn == nullptr || cdOut == nullptr)
  {
    return;
  }

  // this is needed since vtkCompositeDataSet::ShallowCopy() doesn't clone
  // leaf nodes, but simply passes them through.
  vtkSmartPointer<vtkCompositeDataIterator> iterOut;
  iterOut.TakeReference(cdOut->NewIterator());
  for (iterOut->InitTraversal(); !iterOut->IsDoneWithTraversal(); iterOut->GoToNextItem())
  {
    vtkSmartPointer<vtkDataObject> curDO = iterOut->GetCurrentDataObject();
    if (vtkCompositeDataSet::SafeDownCast(curDO) == nullptr && curDO != nullptr)
    {
      vtkDataObject* clone = curDO->NewInstance();
      clone->ShallowCopy(curDO);
      cdOut->SetDataSet(iterOut, clone);
      clone->FastDelete();
    }
  }
}
