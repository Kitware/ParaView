/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPEnSightReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "vtkPEnSightReader.h"

#include "vtkDataArrayCollection.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIdListCollection.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include "vtksys/FStream.hxx"

typedef std::vector<vtkPEnSightReader::vtkPEnSightReaderCellIds*> vtkPEnSightReaderCellIdsTypeBase;
class vtkPEnSightReaderCellIdsType : public vtkPEnSightReaderCellIdsTypeBase
{
};

namespace
{
void cleanup(vtkPEnSightReaderCellIdsType* foo)
{
  if (!foo)
  {
    return;
  }
  for (vtkPEnSightReaderCellIdsType::iterator iter = foo->begin(); iter != foo->end(); ++iter)
  {
    delete *iter;
  }
}
}

//----------------------------------------------------------------------------
vtkPEnSightReader::vtkPEnSightReader()
{
  this->MeasuredFileName = NULL;
  this->MatchFileName = NULL;

  this->IS = NULL;

  this->VariableMode = -1;

  this->UnstructuredPartIds = vtkIdList::New();
  this->StructuredPartIds = vtkIdList::New();
  this->CellIds = NULL;

  this->PointIds = NULL;

  this->VariableFileNames = NULL;
  this->ComplexVariableFileNames = NULL;

  this->VariableDescriptions = NULL;
  this->ComplexVariableDescriptions = NULL;

  this->VariableTimeSetIds = vtkIdList::New();
  this->ComplexVariableTimeSetIds = vtkIdList::New();
  this->VariableFileSetIds = vtkIdList::New();
  this->ComplexVariableFileSetIds = vtkIdList::New();

  this->TimeSetFileNameNumbers = vtkIdListCollection::New();
  this->TimeSetsWithFilenameNumbers = vtkIdList::New();
  this->TimeSets = vtkDataArrayCollection::New();
  this->FileSetFileNameNumbers = vtkIdListCollection::New();
  this->FileSetsWithFilenameNumbers = vtkIdList::New();
  this->FileSetNumberOfSteps = vtkIdListCollection::New();

  this->TimeSetIds = vtkIdList::New();
  this->FileSets = vtkIdList::New();

  this->GeometryTimeSet = 1;
  this->GeometryFileSet = 1;
  this->MeasuredTimeSet = 1;
  this->MeasuredFileSet = 1;

  this->UseTimeSets = 0;
  this->UseFileSets = 0;

  this->GeometryTimeValue = -1;
  this->MeasuredTimeValue = -1;

  this->NumberOfGeometryParts = 0;

  this->NumberOfMeasuredPoints = 0;

  this->InitialRead = 1;
  this->NumberOfNewOutputs = 0;

  // -2 is the default starting value
  this->MultiProcessLocalProcessId = -2;
  this->MultiProcessNumberOfProcesses = -2;

  this->GhostLevels = 0;
}

//----------------------------------------------------------------------------
vtkPEnSightReader::~vtkPEnSightReader()
{
  int i;

  if (this->CellIds)
  {
    cleanup(this->CellIds);
    delete this->CellIds;
    this->CellIds = NULL;
  }

  if (this->PointIds)
  {
    cleanup(this->PointIds);
    delete this->PointIds;
    this->PointIds = NULL;
  }

  if (this->MeasuredFileName)
  {
    delete[] this->MeasuredFileName;
    this->MeasuredFileName = NULL;
  }
  if (this->MatchFileName)
  {
    delete[] this->MatchFileName;
    this->MatchFileName = NULL;
  }

  if (this->NumberOfVariables > 0)
  {
    for (i = 0; i < this->NumberOfVariables; i++)
    {
      delete[] this->VariableFileNames[i];
    }
    delete[] this->VariableFileNames;
    this->VariableFileNames = NULL;
  }

  if (this->NumberOfComplexVariables > 0)
  {
    for (i = 0; i < this->NumberOfComplexVariables * 2; i++)
    {
      delete[] this->ComplexVariableFileNames[i];
    }
    delete[] this->ComplexVariableFileNames;
    this->ComplexVariableFileNames = NULL;
  }

  this->UnstructuredPartIds->Delete();
  this->UnstructuredPartIds = NULL;
  this->StructuredPartIds->Delete();
  this->StructuredPartIds = NULL;

  this->VariableTimeSetIds->Delete();
  this->VariableTimeSetIds = NULL;
  this->ComplexVariableTimeSetIds->Delete();
  this->ComplexVariableTimeSetIds = NULL;
  this->VariableFileSetIds->Delete();
  this->VariableFileSetIds = NULL;
  this->ComplexVariableFileSetIds->Delete();
  this->ComplexVariableFileSetIds = NULL;

  this->TimeSetFileNameNumbers->Delete();
  this->TimeSetFileNameNumbers = NULL;
  this->TimeSetsWithFilenameNumbers->Delete();
  this->TimeSetsWithFilenameNumbers = NULL;
  this->TimeSets->Delete();
  this->TimeSets = NULL;
  this->FileSetFileNameNumbers->Delete();
  this->FileSetFileNameNumbers = NULL;
  this->FileSetsWithFilenameNumbers->Delete();
  this->FileSetsWithFilenameNumbers = NULL;
  this->FileSetNumberOfSteps->Delete();
  this->FileSetNumberOfSteps = NULL;

  this->TimeSetIds->Delete();
  this->TimeSets = NULL;
  this->FileSets->Delete();
  this->FileSets = NULL;

  this->ActualTimeValue = 0.0;
}

//----------------------------------------------------------------------------
int vtkPEnSightReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkDebugMacro("In execute ");

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet* output =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int tsLength = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  double* steps = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  this->ActualTimeValue = this->TimeValue;

  // Ghost Levels are only supported for Structured Datae
  this->GhostLevels =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  // Check if a particular time was requested by the pipeline.
  // This overrides the ivar.
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()) && tsLength > 0)
  {
    // Get the requested time step. We only support requests of a single time
    // step in this reader right now
    double requestedTimeStep = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    // find the first time value larger than requested time value
    // this logic could be improved
    int cnt = 0;
    while (cnt < tsLength - 1 && steps[cnt] < requestedTimeStep)
    {
      cnt++;
    }
    this->ActualTimeValue = steps[cnt];
  }

  cout << "Executing with: " << this->ActualTimeValue << endl;

  int i, timeSet, fileSet, timeStep, timeStepInFile, fileNum;
  vtkDataArray* times;
  vtkIdList *numStepsList, *filenameNumbers;
  float newTime;
  int numSteps;
  char* fileName;
  int filenameNum;

  if (!this->CaseFileRead)
  {
    vtkErrorMacro("error reading case file");
    return 0;
  }

  this->NumberOfNewOutputs = 0;
  this->NumberOfGeometryParts = 0;
  if (this->GeometryFileName)
  {
    timeStep = timeStepInFile = 1;
    fileNum = 1;
    fileName = new char[strlen(this->GeometryFileName) + 10];
    strcpy(fileName, this->GeometryFileName);

    if (this->UseTimeSets)
    {
      timeSet = this->TimeSetIds->IsId(this->GeometryTimeSet);
      if (timeSet >= 0)
      {
        times = this->TimeSets->GetItem(timeSet);
        this->GeometryTimeValue = times->GetComponent(0, 0);
        for (i = 1; i < times->GetNumberOfTuples(); i++)
        {
          newTime = times->GetComponent(i, 0);
          if (newTime <= this->ActualTimeValue && newTime > this->GeometryTimeValue)
          {
            this->GeometryTimeValue = newTime;
            timeStep++;
            timeStepInFile++;
          }
        }
        if (this->TimeSetFileNameNumbers->GetNumberOfItems() > 0)
        {
          int collectionNum = this->TimeSetsWithFilenameNumbers->IsId(this->GeometryTimeSet);
          if (collectionNum > -1)
          {
            filenameNumbers = this->TimeSetFileNameNumbers->GetItem(collectionNum);
            filenameNum = filenameNumbers->GetId(timeStep - 1);
            if (!this->UseFileSets)
            {
              this->ReplaceWildcards(fileName, filenameNum);
            }
          }
        }

        // There can only be file sets if there are also time sets.
        if (this->UseFileSets)
        {
          fileSet = this->FileSets->IsId(this->GeometryFileSet);
          numStepsList =
            static_cast<vtkIdList*>(this->FileSetNumberOfSteps->GetItemAsObject(fileSet));

          if (timeStep > numStepsList->GetId(0))
          {
            numSteps = numStepsList->GetId(0);
            timeStepInFile -= numSteps;
            fileNum = 2;
            for (i = 1; i < numStepsList->GetNumberOfIds(); i++)
            {
              numSteps += numStepsList->GetId(i);
              if (timeStep > numSteps)
              {
                fileNum++;
                timeStepInFile -= numStepsList->GetId(i);
              }
            }
          }
          if (this->FileSetFileNameNumbers->GetNumberOfItems() > 0)
          {
            int collectionNum = this->FileSetsWithFilenameNumbers->IsId(this->GeometryFileSet);
            if (collectionNum > -1)
            {
              filenameNumbers = this->FileSetFileNameNumbers->GetItem(collectionNum);
              filenameNum = filenameNumbers->GetId(fileNum - 1);
              this->ReplaceWildcards(fileName, filenameNum);
            }
          }
        }
      }
    }

    int readGeom = this->ReadGeometryFile(fileName, timeStepInFile, output);
    if (!readGeom)
    {
      vtkErrorMacro("error reading geometry file " << fileName << " " << readGeom);
      delete[] fileName;
      return 0;
    }

    delete[] fileName;
  }
  if (this->MeasuredFileName)
  {
    timeStep = timeStepInFile = 1;
    fileNum = 1;
    fileName = new char[strlen(this->MeasuredFileName) + 10];
    strcpy(fileName, this->MeasuredFileName);

    if (this->UseTimeSets)
    {
      timeSet = this->TimeSetIds->IsId(this->MeasuredTimeSet);
      if (timeSet >= 0)
      {
        times = this->TimeSets->GetItem(timeSet);
        this->MeasuredTimeValue = times->GetComponent(0, 0);
        for (i = 1; i < times->GetNumberOfTuples(); i++)
        {
          newTime = times->GetComponent(i, 0);
          if (newTime <= this->ActualTimeValue && newTime > this->MeasuredTimeValue)
          {
            this->MeasuredTimeValue = newTime;
            timeStep++;
            timeStepInFile++;
          }
        }
        if (this->TimeSetFileNameNumbers->GetNumberOfItems() > 0)
        {
          int collectionNum = this->TimeSetsWithFilenameNumbers->IsId(this->MeasuredTimeSet);
          if (collectionNum > -1)
          {
            filenameNumbers = this->TimeSetFileNameNumbers->GetItem(collectionNum);
            filenameNum = filenameNumbers->GetId(timeStep - 1);
            if (!this->UseFileSets)
            {
              this->ReplaceWildcards(fileName, filenameNum);
            }
          }
        }

        // There can only be file sets if there are also time sets.
        if (this->UseFileSets)
        {
          fileSet = this->FileSets->IsId(this->MeasuredFileSet);
          numStepsList =
            static_cast<vtkIdList*>(this->FileSetNumberOfSteps->GetItemAsObject(fileSet));

          if (timeStep > numStepsList->GetId(0))
          {
            numSteps = numStepsList->GetId(0);
            timeStepInFile -= numSteps;
            fileNum = 2;
            for (i = 1; i < numStepsList->GetNumberOfIds(); i++)
            {
              numSteps += numStepsList->GetId(i);
              if (timeStep > numSteps)
              {
                fileNum++;
                timeStepInFile -= numStepsList->GetId(i);
              }
            }
          }
          if (this->FileSetFileNameNumbers->GetNumberOfItems() > 0)
          {
            int collectionNum = this->FileSetsWithFilenameNumbers->IsId(this->MeasuredFileSet);
            if (collectionNum > -1)
            {
              filenameNumbers = this->FileSetFileNameNumbers->GetItem(fileSet);
              filenameNum = filenameNumbers->GetId(fileNum - 1);
              this->ReplaceWildcards(fileName, filenameNum);
            }
          }
        }
      }
    }
    if (!this->ReadMeasuredGeometryFile(fileName, timeStepInFile, output))
    {
      vtkErrorMacro("error reading measured geometry file");
      delete[] fileName;
      return 0;
    }
    delete[] fileName;
  }

  if ((this->NumberOfVariables + this->NumberOfComplexVariables) > 0)
  {
    if (!this->ReadVariableFiles(output))
    {
      vtkErrorMacro("error reading variable files");
      return 0;
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPEnSightReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkDebugMacro("In execute information");
  this->CaseFileRead = this->ReadCaseFile();

  // Convert time steps to one sorted and uniquefied list.
  std::vector<double> timeValues;
  if (this->GetTimeSets())
  {
    int numItems = this->GetTimeSets()->GetNumberOfItems();
    for (int i = 0; i < numItems; i++)
    {
      vtkDataArray* array = this->GetTimeSets()->GetItem(i);
      if (array)
      {
        vtkIdType numTuples = array->GetNumberOfTuples();
        for (vtkIdType j = 0; j < numTuples; j++)
        {
          timeValues.push_back(array->GetComponent(j, 0));
        }
      }
    }
  }
  if (timeValues.size() > 0)
  {
    std::sort(timeValues.begin(), timeValues.end());
    std::vector<double> uniqueTimeValues(
      timeValues.begin(), std::unique(timeValues.begin(), timeValues.end()));
    int numTimeValues = static_cast<int>(uniqueTimeValues.size());
    if (numTimeValues > 0)
    {
      vtkInformation* outInfo = outputVector->GetInformationObject(0);
      // TEST
      // TEST
      outInfo->Set(
        vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &uniqueTimeValues[0], numTimeValues);
      double timeRange[2];
      timeRange[0] = uniqueTimeValues[0];
      timeRange[1] = uniqueTimeValues[numTimeValues - 1];
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
    }
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  return this->CaseFileRead;
}

//----------------------------------------------------------------------------
int vtkPEnSightReader::ReadCaseFileGeometry(char* line)
{
  char subLine[256];
  int timeSet, fileSet, lineRead;

  // There will definitely be a "model" line.  There may also be "measured"
  // and "match" lines.
  lineRead = this->ReadNextDataLine(line);
  while (lineRead && (strncmp(line, "m", 1) == 0 || strncmp(line, "boundary:", 9) == 0 ||
                       strncmp(line, "rigid_body:", 11) == 0))
  {
    if (strncmp(line, "model:", 6) == 0)
    {
      if (sscanf(line, " %*s %d%*[ \t]%d%*[ \t]%s", &timeSet, &fileSet, subLine) == 3)
      {
        this->GeometryTimeSet = timeSet;
        this->GeometryFileSet = fileSet;
        this->SetGeometryFileName(subLine);
        vtkDebugMacro(<< this->GetGeometryFileName());
      }
      else if (sscanf(line, " %*s %d%*[ \t]%s", &timeSet, subLine) == 2)
      {
        this->GeometryTimeSet = timeSet;
        this->SetGeometryFileName(subLine);
        vtkDebugMacro(<< this->GetGeometryFileName());
      }
      else if (sscanf(line, " %*s %s", subLine) == 1)
      {
        this->SetGeometryFileName(subLine);
        vtkDebugMacro(<< this->GetGeometryFileName());
      }
    }
    else if (strncmp(line, "measured:", 9) == 0)
    {
      if (sscanf(line, " %*s %d%*[ \t]%d%*[ \t]%s", &timeSet, &fileSet, subLine) == 3)
      {
        this->MeasuredTimeSet = timeSet;
        this->MeasuredFileSet = fileSet;
        this->SetMeasuredFileName(subLine);
        vtkDebugMacro(<< this->GetMeasuredFileName());
      }
      else if (sscanf(line, " %*s %d%*[ \t]%s", &timeSet, subLine) == 2)
      {
        this->MeasuredTimeSet = timeSet;
        this->SetMeasuredFileName(subLine);
        vtkDebugMacro(<< this->GetMeasuredFileName());
      }
      else if (sscanf(line, " %*s %s", subLine) == 1)
      {
        this->SetMeasuredFileName(subLine);
        vtkDebugMacro(<< this->GetMeasuredFileName());
      }
    }
    else if (strncmp(line, "match:", 6) == 0)
    {
      sscanf(line, " %*s %s", subLine);
      this->SetMatchFileName(subLine);
      vtkDebugMacro(<< this->GetMatchFileName());
    }
    lineRead = this->ReadNextDataLine(line);
    // the other possibilities: boundaries & rigid_body are just ignored
  }

  return lineRead;
}

//----------------------------------------------------------------------------
int vtkPEnSightReader::ReadCaseFileVariable(char* line)
{
  char subLine[256], subLine2[256];
  int timeSet, fileSet, lineRead;

  this->NumberOfScalarsPerNode = 0;
  this->NumberOfVectorsPerNode = 0;
  this->NumberOfTensorsSymmPerNode = 0;
  this->NumberOfScalarsPerElement = 0;
  this->NumberOfVectorsPerElement = 0;
  this->NumberOfTensorsSymmPerElement = 0;
  this->NumberOfScalarsPerMeasuredNode = 0;
  this->NumberOfVectorsPerMeasuredNode = 0;
  this->NumberOfComplexScalarsPerNode = 0;
  this->NumberOfComplexVectorsPerNode = 0;
  this->NumberOfComplexScalarsPerElement = 0;
  this->NumberOfComplexVectorsPerElement = 0;

  lineRead = this->ReadNextDataLine(line);
  while (lineRead && strncmp(line, "FORMAT", 6) != 0 && strncmp(line, "GEOMETRY", 8) != 0 &&
    strncmp(line, "VARIABLE", 8) != 0 && strncmp(line, "TIME", 4) != 0 &&
    strncmp(line, "FILE", 4) != 0)
  {
    if (strncmp(line, "constant", 8) == 0)
    {
      vtkDebugMacro(<< line);
    }
    else if (strncmp(line, "scalar", 6) == 0)
    {
      sscanf(line, " %*s %*s %s", subLine);
      if (strcmp(subLine, "node:") == 0)
      {
        vtkDebugMacro("scalar per node");
        this->VariableMode = vtkPEnSightReader::SCALAR_PER_NODE;
        if (sscanf(line, " %*s %*s %*s %d %d %s", &timeSet, &fileSet, subLine) == 3)
        {
          this->VariableTimeSetIds->InsertNextId(timeSet);
          this->VariableFileSetIds->InsertNextId(fileSet);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*d %*d %*s %s", subLine);
        }
        else if (sscanf(line, " %*s %*s %*s %d %s", &timeSet, subLine) == 2)
        {
          this->VariableTimeSetIds->InsertNextId(timeSet);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*d %*s %s", subLine);
        }
        else if (sscanf(line, " %*s %*s %*s %s", subLine) == 1)
        {
          this->VariableTimeSetIds->InsertNextId(1);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*s %s", subLine);
        }
        this->AddVariableType();
        this->NumberOfScalarsPerNode++;
      }
      else if (strcmp(subLine, "element:") == 0)
      {
        vtkDebugMacro("scalar per element");
        this->VariableMode = vtkPEnSightReader::SCALAR_PER_ELEMENT;
        if (sscanf(line, " %*s %*s %*s %d %d %s", &timeSet, &fileSet, subLine) == 3)
        {
          this->VariableTimeSetIds->InsertNextId(timeSet);
          this->VariableFileSetIds->InsertNextId(fileSet);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*d %*d %*s %s", subLine);
        }
        else if (sscanf(line, " %*s %*s %*s %d %s", &timeSet, subLine) == 2)
        {
          this->VariableTimeSetIds->InsertNextId(timeSet);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*d %*s %s", subLine);
        }
        else if (sscanf(line, " %*s %*s %*s %s", subLine) == 1)
        {
          this->VariableTimeSetIds->InsertNextId(1);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*s %s", subLine);
        }
        this->AddVariableType();
        this->NumberOfScalarsPerElement++;
      }
      else if (strcmp(subLine, "measured") == 0)
      {
        vtkDebugMacro("scalar per measured node");
        this->VariableMode = vtkPEnSightReader::SCALAR_PER_MEASURED_NODE;
        if (sscanf(line, " %*s %*s %*s %*s %d %d %s", &timeSet, &fileSet, subLine) == 3)
        {
          this->VariableTimeSetIds->InsertNextId(timeSet);
          this->VariableFileSetIds->InsertNextId(fileSet);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s", subLine);
        }
        else if (sscanf(line, " %*s %*s %*s %*s %d %s", &timeSet, subLine) == 2)
        {
          this->VariableTimeSetIds->InsertNextId(timeSet);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*s %*d %*s %s", subLine);
        }
        else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
        {
          this->VariableTimeSetIds->InsertNextId(1);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*s %*s %s", subLine);
        }
        this->AddVariableType();
        this->NumberOfScalarsPerMeasuredNode++;
      }
      this->AddVariableFileName(subLine);
      this->NumberOfVariables++;
    }
    else if (strncmp(line, "vector", 6) == 0)
    {
      sscanf(line, " %*s %*s %s", subLine);
      if (strcmp(subLine, "node:") == 0)
      {
        vtkDebugMacro("vector per node");
        this->VariableMode = vtkPEnSightReader::VECTOR_PER_NODE;
        if (sscanf(line, " %*s %*s %*s %d %d %s", &timeSet, &fileSet, subLine) == 3)
        {
          this->VariableTimeSetIds->InsertNextId(timeSet);
          this->VariableFileSetIds->InsertNextId(fileSet);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*d %*d %*s %s", subLine);
        }
        else if (sscanf(line, " %*s %*s %*s %d %s", &timeSet, subLine) == 2)
        {
          this->VariableTimeSetIds->InsertNextId(timeSet);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*d %*s %s", subLine);
        }
        else if (sscanf(line, " %*s %*s %*s %s", subLine) == 1)
        {
          this->VariableTimeSetIds->InsertNextId(1);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*s %s", subLine);
        }
        this->AddVariableType();
        this->NumberOfVectorsPerNode++;
      }
      else if (strcmp(subLine, "element:") == 0)
      {
        vtkDebugMacro("vector per element");
        this->VariableMode = vtkPEnSightReader::VECTOR_PER_ELEMENT;
        if (sscanf(line, " %*s %*s %*s %d %d %s", &timeSet, &fileSet, subLine) == 3)
        {
          this->VariableTimeSetIds->InsertNextId(timeSet);
          this->VariableFileSetIds->InsertNextId(fileSet);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*d %*d %*s %s", subLine);
        }
        else if (sscanf(line, " %*s %*s %*s %d %s", &timeSet, subLine) == 2)
        {
          this->VariableTimeSetIds->InsertNextId(timeSet);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*d %*s %s", subLine);
        }
        else if (sscanf(line, " %*s %*s %*s %s", subLine) == 1)
        {
          this->VariableTimeSetIds->InsertNextId(1);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*s %s", subLine);
        }
        this->AddVariableType();
        this->NumberOfVectorsPerElement++;
      }
      else if (strcmp(subLine, "measured") == 0)
      {
        vtkDebugMacro("vector per measured node");
        this->VariableMode = vtkPEnSightReader::VECTOR_PER_MEASURED_NODE;
        if (sscanf(line, " %*s %*s %*s %*s %d %d %s", &timeSet, &fileSet, subLine) == 3)
        {
          this->VariableTimeSetIds->InsertNextId(timeSet);
          this->VariableFileSetIds->InsertNextId(fileSet);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s", subLine);
        }
        else if (sscanf(line, " %*s %*s %*s %*s %d %s", &timeSet, subLine) == 2)
        {
          this->VariableTimeSetIds->InsertNextId(timeSet);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*s %*d %*s %s", subLine);
        }
        else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
        {
          this->VariableTimeSetIds->InsertNextId(1);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*s %*s %s", subLine);
        }
        this->AddVariableType();
        this->NumberOfVectorsPerMeasuredNode++;
      }
      this->AddVariableFileName(subLine);
      this->NumberOfVariables++;
    }
    else if (strncmp(line, "tensor", 6) == 0)
    {
      // According to EnSight documentation tensor entry should be of the form:
      // tensor symm per node/element
      // but it seems like you can also find:
      // tensor per node/element
      // Let handle this case here:
      char symm[10];
      char per[10];
      if (sscanf(line, " %*s %s %s %s", symm, per, subLine) != 3)
      {
        vtkErrorMacro("Error while reading: " << line);
      }
      if (!(strcmp(symm, "symm") == 0 && strcmp(per, "per") == 0))
      {
        if (sscanf(line, " %*s %s %s", per, subLine) != 2)
        {
          vtkErrorMacro("Error while reading: " << line);
        }
        if (strcmp(per, "per") == 0)
        {
          // Not valid file but seems alright, only 'symm' is missing
          vtkWarningMacro("Looks almost like a valid case file, continuing");
        }
        else
        {
          vtkErrorMacro("Trouble reading: " << line);
        }
      }
      if (strcmp(subLine, "node:") == 0)
      {
        vtkDebugMacro("tensor symm per node");
        this->VariableMode = vtkPEnSightReader::TENSOR_SYMM_PER_NODE;
        if (sscanf(line, " %*s %*s %*s %*s %d %d %s", &timeSet, &fileSet, subLine) == 3)
        {
          this->VariableTimeSetIds->InsertNextId(timeSet);
          this->VariableFileSetIds->InsertNextId(fileSet);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s", subLine);
        }
        else if (sscanf(line, " %*s %*s %*s %*s %d %s", &timeSet, subLine) == 2)
        {
          this->VariableTimeSetIds->InsertNextId(timeSet);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*s %*d %*s %s", subLine);
        }
        else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
        {
          this->VariableTimeSetIds->InsertNextId(1);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*s %*s %s", subLine);
        }
        this->AddVariableType();
        this->NumberOfTensorsSymmPerNode++;
      }
      else if (strcmp(subLine, "element:") == 0)
      {
        vtkDebugMacro("tensor symm per element");
        this->VariableMode = vtkPEnSightReader::TENSOR_SYMM_PER_ELEMENT;
        if (sscanf(line, " %*s %*s %*s %*s %d %d %s", &timeSet, &fileSet, subLine) == 3)
        {
          this->VariableTimeSetIds->InsertNextId(timeSet);
          this->VariableFileSetIds->InsertNextId(fileSet);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s", subLine);
        }
        else if (sscanf(line, " %*s %*s %*s %*s %d %s", &timeSet, subLine) == 2)
        {
          this->VariableTimeSetIds->InsertNextId(timeSet);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*s %*d %*s %s", subLine);
        }
        else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
        {
          this->VariableTimeSetIds->InsertNextId(1);
          this->AddVariableDescription(subLine);
          sscanf(line, " %*s %*s %*s %*s %*s %s", subLine);
        }
        this->AddVariableType();
        this->NumberOfTensorsSymmPerElement++;
      }
      else
      {
        vtkErrorMacro("Unknown type, faulty line was:" << line);
      }
      this->AddVariableFileName(subLine);
      this->NumberOfVariables++;
    }
    else if (strncmp(line, "complex", 6) == 0)
    {
      sscanf(line, " %*s %s", subLine);
      if (strcmp(subLine, "scalar") == 0)
      {
        sscanf(line, " %*s %*s %*s %s", subLine);
        if (strcmp(subLine, "node:") == 0)
        {
          vtkDebugMacro("complex scalar per node");
          this->VariableMode = vtkPEnSightReader::COMPLEX_SCALAR_PER_NODE;
          if (sscanf(line, " %*s %*s %*s %*s %d %d %s", &timeSet, &fileSet, subLine) == 3)
          {
            this->ComplexVariableTimeSetIds->InsertNextId(timeSet);
            this->ComplexVariableFileSetIds->InsertNextId(fileSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s %s", subLine, subLine2);
          }
          else if (sscanf(line, " %*s %*s %*s %*s %d %s", &timeSet, subLine) == 2)
          {
            this->ComplexVariableTimeSetIds->InsertNextId(timeSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*s %s %s", subLine, subLine2);
          }
          else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
          {
            this->ComplexVariableTimeSetIds->InsertNextId(1);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*s %s %s", subLine, subLine2);
          }
          this->AddVariableType();
          this->NumberOfComplexScalarsPerNode++;
        }
        else if (strcmp(subLine, "element:") == 0)
        {
          vtkDebugMacro("complex scalar per element");
          this->VariableMode = vtkPEnSightReader::COMPLEX_SCALAR_PER_ELEMENT;
          if (sscanf(line, " %*s %*s %*s %*s %d %d %s", &timeSet, &fileSet, subLine) == 3)
          {
            this->ComplexVariableTimeSetIds->InsertNextId(timeSet);
            this->ComplexVariableFileSetIds->InsertNextId(fileSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s %s", subLine, subLine2);
          }
          else if (sscanf(line, " %*s %*s %*s %*s %d %s", &timeSet, subLine) == 2)
          {
            this->ComplexVariableTimeSetIds->InsertNextId(timeSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*s %s %s", subLine, subLine2);
          }
          else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
          {
            this->ComplexVariableTimeSetIds->InsertNextId(1);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*s %s %s", subLine, subLine2);
          }
          this->AddVariableType();
          this->NumberOfComplexScalarsPerElement++;
        }
      }
      else if (strcmp(subLine, "vector") == 0)
      {
        sscanf(line, " %*s %*s %*s %s", subLine);
        if (strcmp(subLine, "node:") == 0)
        {
          vtkDebugMacro("complex vector per node");
          this->VariableMode = vtkPEnSightReader::COMPLEX_VECTOR_PER_NODE;
          if (sscanf(line, " %*s %*s %*s %*s %d %d %s", &timeSet, &fileSet, subLine) == 3)
          {
            this->ComplexVariableTimeSetIds->InsertNextId(timeSet);
            this->ComplexVariableFileSetIds->InsertNextId(fileSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s %s", subLine, subLine2);
          }
          else if (sscanf(line, " %*s %*s %*s %*s %d %s", &timeSet, subLine) == 2)
          {
            this->ComplexVariableTimeSetIds->InsertNextId(timeSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*s %s %s", subLine, subLine2);
          }
          else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
          {
            this->ComplexVariableTimeSetIds->InsertNextId(1);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*s %s %s", subLine, subLine2);
          }
          this->AddVariableType();
          this->NumberOfComplexVectorsPerNode++;
        }
        else if (strcmp(subLine, "element:") == 0)
        {
          vtkDebugMacro("complex vector per element");
          this->VariableMode = vtkPEnSightReader::COMPLEX_VECTOR_PER_ELEMENT;
          if (sscanf(line, " %*s %*s %*s %*s %d %d %s", &timeSet, &fileSet, subLine) == 3)
          {
            this->ComplexVariableTimeSetIds->InsertNextId(timeSet);
            this->ComplexVariableFileSetIds->InsertNextId(fileSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s %s", subLine, subLine2);
          }
          else if (sscanf(line, " %*s %*s %*s %*s %d %s", &timeSet, subLine) == 2)
          {
            this->ComplexVariableTimeSetIds->InsertNextId(timeSet);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*s %s %s", subLine, subLine2);
          }
          else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
          {
            this->ComplexVariableTimeSetIds->InsertNextId(1);
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*s %s %s", subLine, subLine2);
          }
          this->AddVariableType();
          this->NumberOfComplexVectorsPerElement++;
        }
      }
      this->AddVariableFileName(subLine, subLine2);
      this->NumberOfComplexVariables++;
    }
    else
    {
      vtkErrorMacro("invalid VARIABLE line: " << line);
      delete this->IS;
      this->IS = NULL;
      return 0;
    }
    lineRead = this->ReadNextDataLine(line);
  }

  // Fill data array selection objects with these arrays.
  this->SetDataArraySelectionSetsFromVariables();
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkPEnSightReader::ReadCaseFileTime(char* line)
{
  char formatLine[256];
  char subLine[256];
  int timeSet, numTimeSteps, i, filenameNum, increment, lineRead;
  float timeStep;

  // found TIME section
  int firstTimeStep = 1;

  this->UseTimeSetsOn();
  int lineScanResult = 0;
  lineRead = this->ReadNextDataLine(line);
  while (lineRead && strncmp(line, "FORMAT", 6) != 0 && strncmp(line, "GEOMETRY", 8) != 0 &&
    strncmp(line, "VARIABLE", 8) != 0 && strncmp(line, "TIME", 4) != 0 &&
    strncmp(line, "FILE", 4) != 0)
  {
    // 'time set: <int>' --- to obtain timeSet, an index
    lineScanResult = sscanf(line, "%*s %s %d", subLine, &timeSet);
    if (lineScanResult != 2 || strncmp(line, "time", 4) != 0 || strcmp(subLine, "set:") != 0)
    {
      vtkErrorMacro("Error with vtkEnSightReader: 'time set' not found!!!");
      return 0;
    }
    this->TimeSetIds->InsertNextId(timeSet);

    // 'number of steps: <int>' --- to obtain numTimeSteps
    // i.e.., the number of 'filename numbers' and equally that of 'time values'
    if (this->ReadNextDataLine(line) == 0)
    {
      vtkErrorMacro("Error with vtkEnSightReader: 'number of steps' not found!!!");
      return 0;
    }

    lineScanResult = sscanf(line, "%*s %*s %s %d", subLine, &numTimeSteps);
    if (lineScanResult != 2 || strncmp(line, "number", 6) != 0 || strcmp(subLine, "steps:") != 0)
    {
      vtkErrorMacro("Error with vtkEnSightReader: 'number of steps' not found!!!");
      return 0;
    }

    // 'filename numbers:' ==or the combination of the following two lines==
    // 'filename start number: <int>'
    // 'filename increment: <int>'
    // --- to obtain a sequence of filenameNum(s) which might span multiple lines
    if (this->ReadNextDataLine(line) == 0)
    {
      vtkErrorMacro("Error with vtkEnSightReader: 'filename ......' not found!!!");
      return 0;
    }

    if (strncmp(line, "filename", 8) == 0)
    {
      vtkIdList* filenameNumbers = vtkIdList::New();
      this->TimeSetsWithFilenameNumbers->InsertNextId(timeSet);

      if (sscanf(line, "%*s %s", subLine) != 1)
      {
        vtkErrorMacro("Error with vtkEnSightReader: 'filename ......' not found!!!");
        return 0;
      }

      // 'filename numbers:'
      if (strncmp(subLine, "numbers", 7) == 0)
      {
        // Filename numbers may be provided on the line(s) following
        // 'filename numbers:', as is usually the case --- not "inline". Thus we need
        // to go to the FIRST line that indeed contains filename numbers.
        if (sscanf(line, "%*s %*s %d", &filenameNum) != 1)
        {
          // not "inline"
          if (this->ReadNextDataLine(line) == 0)
          {
            vtkErrorMacro("Error with vtkEnSightReader: filename numbers not found!!!");
            return 0;
          }
          // access the sub-strings from the very beginning
          strcpy(formatLine, "");
          strcpy(subLine, "");
        }
        else
        {
          // "inline" ----> skip the first two sub-strings: 'filename numbers:'
          strcpy(formatLine, "%*s %*s ");
          strcpy(subLine, "%*s %*s ");
        }

        for (i = 0; i < numTimeSteps; i++)
        {
          strcat(formatLine, "%d ");

          // More lines might be needed to provide the remaining filename numbers
          // and then formatLine and subLine need to be updated. 'while' is used here
          // instead of 'if' in case of any invalid filename numbers.
          while (sscanf(line, formatLine, &filenameNum) != 1)
          {
            if (this->ReadNextDataLine(line) == 0)
            {
              vtkErrorMacro("Error with vtkEnSightReader: insufficient filename numbers!!!");
              return 0;
            }

            // in case of insufficient filename numbers
            if (strncmp(line, "filename start", 14) == 0 ||
              strncmp(line, "filename increment", 18) == 0 ||
              strncmp(line, "time values", 11) == 0 || strncmp(line, "time set", 8) == 0 ||
              strncmp(line, "FILE", 4) == 0)
            {
              vtkErrorMacro("Error with vtkEnSightReader: insufficient filename numbers!!!");
              return 0;
            }

            // to access a new line
            strcpy(formatLine, "%d ");
            strcpy(subLine, "");
          }

          filenameNumbers->InsertNextId(filenameNum);
          strcat(subLine, "%*d ");
          strcpy(formatLine, subLine);
        }
      }
      else
      // subLine == "start" ----> 'filename start number: <int>' followed by
      // 'filename increment: <int>'
      {
        if (strcmp(subLine, "start") != 0 || sscanf(line, "%*s %*s %*s %d", &filenameNum) != 1)
        {
          vtkErrorMacro("Error with vtkEnSightReader: 'filename start number' not found!!!");
          return 0;
        }

        if (this->ReadNextDataLine(line) == 0)
        {
          vtkErrorMacro("Error with vtkEnSightReader: 'filename increment' not found!!!");
          return 0;
        }

        lineScanResult = sscanf(line, "%*s %s %d", subLine, &increment);
        if (lineScanResult != 2 || strcmp(subLine, "increment:") != 0)
        {
          vtkErrorMacro("Error with vtkEnSightReader: 'filename increment' not found!!!");
          return 0;
        }

        for (i = 0; i < numTimeSteps; i++)
        {
          filenameNumbers->InsertNextId(filenameNum + i * increment);
        }
      }
      this->TimeSetFileNameNumbers->AddItem(filenameNumbers);
      filenameNumbers->Delete();

      // To ignore redundant filename numbers, just if any,
      // and check if 'time values' are subsequently provided as expected.
      do
      {
        lineRead = this->ReadNextDataLine(line);
      } while (lineRead != 0 && strncmp(line, "time values", 11) != 0);

      if (lineRead == 0)
      {
        vtkErrorMacro("Error with vtkEnSightReader: 'time values' not found!!!");
        return 0;
      }
    }

    // 'time values:' --- to obtain timeStep(s)
    vtkFloatArray* timeValues = vtkFloatArray::New();
    timeValues->SetNumberOfComponents(1);
    timeValues->SetNumberOfTuples(numTimeSteps);

    // Time values may be provided on the line(s) following  'time values:',
    // as is usually the case --- not "inline". Thus we need to go to the
    // FIRST line that indeed contains time values.
    if (sscanf(line, "%*s %*s %f", &timeStep) != 1)
    {
      // not "inline"
      if (this->ReadNextDataLine(line) == 0)
      {
        vtkErrorMacro("Error with vtkEnSightReader: time values not found!!!");
        return 0;
      }
      // access the sub-strings from the very beginning
      strcpy(formatLine, "");
      strcpy(subLine, "");
    }
    else
    {
      // "inline" ----> skip the first two sub-strings: 'time values:'
      strcpy(formatLine, "%*s %*s ");
      strcpy(subLine, "%*s %*s ");
    }

    for (i = 0; i < numTimeSteps; i++)
    {
      strcat(formatLine, "%f ");

      // More lines might be needed to provide the remaining time values
      // and then formatLine and subLine need to be updated. 'while' is used
      // here instead of 'if' in case of any invalid time values.
      while (sscanf(line, formatLine, &timeStep) != 1)
      {
        if (this->ReadNextDataLine(line) == 0)
        {
          vtkErrorMacro("Error with vtkEnSightReader: insufficient time values!!!");
          return 0;
        }

        // in case of insufficient time values
        if (strncmp(line, "time set", 8) == 0 || strncmp(line, "FILE", 4) == 0)
        {
          vtkErrorMacro("Error with vtkEnSightReader: insufficient time values!!!");
          return 0;
        }

        // to access a new line
        strcpy(formatLine, "%f ");
        strcpy(subLine, "");
      }

      timeValues->SetComponent(i, 0, timeStep);
      strcat(subLine, "%*f ");
      strcpy(formatLine, subLine);

      // init min and max only upon the access to the FIRST 'timeStep'
      if (firstTimeStep)
      {
        this->MinimumTimeValue = timeStep;
        this->MaximumTimeValue = timeStep;
        firstTimeStep = 0;
        // Set this as default TimeValue.
        if (!this->TimeValueInitialized)
        {
          this->SetTimeValue(timeStep);
        }
      }
      else
      {
        if (timeStep < this->MinimumTimeValue)
        {
          this->MinimumTimeValue = timeStep;
        }
        else if (timeStep > this->MaximumTimeValue)
        {
          this->MaximumTimeValue = timeStep;
        }
      }
    }
    this->TimeSets->AddItem(timeValues);
    timeValues->Delete();

    // The following line MUST *NOT* be modified as it enables the loop
    // "while ( lineReadResult != 0 && strncmp(line, "FILE", 4) != 0 )".
    // It also enables the check of redundant time-step values.
    lineRead = this->ReadNextDataLine(line);

    // To ignore redundant time-step values, if any, to fix bug #0007091
    while (lineRead != 0 && strncmp(line, "time set", 8) != 0 && strncmp(line, "FILE", 4) != 0 &&
      strncmp(line, "FORMAT", 6) != 0 && strncmp(line, "GEOMETRY", 8) != 0 &&
      strncmp(line, "VARIABLE", 8) != 0 && strncmp(line, "TIME", 4) != 0)
    {
      lineRead = this->ReadNextDataLine(line);
    }
  }

  return lineRead;
}

//----------------------------------------------------------------------------
int vtkPEnSightReader::ReadCaseFileFile(char* line)
{
  int fileSet, numTimeSteps, filenameNum, lineRead;

  this->UseFileSetsOn();
  lineRead = this->ReadNextDataLine(line);
  while (lineRead && strncmp(line, "FORMAT", 6) != 0 && strncmp(line, "GEOMETRY", 8) != 0 &&
    strncmp(line, "VARIABLE", 8) != 0 && strncmp(line, "TIME", 4) != 0 &&
    strncmp(line, "FILE", 4) != 0)
  {
    vtkIdList* filenameNums = vtkIdList::New();
    vtkIdList* numSteps = vtkIdList::New();
    sscanf(line, "%*s %*s %d", &fileSet);
    this->FileSets->InsertNextId(fileSet);
    lineRead = this->ReadNextDataLine(line);
    if (strncmp(line, "filename", 8) == 0)
    {
      this->FileSetsWithFilenameNumbers->InsertNextId(fileSet);
      while (lineRead != 0 && strncmp(line, "filename", 8) == 0)
      {
        sscanf(line, "%*s %*s %d", &filenameNum);
        filenameNums->InsertNextId(filenameNum);
        this->ReadNextDataLine(line);
        sscanf(line, "%*s %*s %*s %d", &numTimeSteps);
        numSteps->InsertNextId(numTimeSteps);
        lineRead = this->ReadNextDataLine(line);
      }
      this->FileSetFileNameNumbers->AddItem(filenameNums);
    }
    else
    {
      sscanf(line, "%*s %*s %*s %d", &numTimeSteps);
      numSteps->InsertNextId(numTimeSteps);
      lineRead = this->ReadNextDataLine(line);
    }

    this->FileSetNumberOfSteps->AddItem(numSteps);

    filenameNums->Delete();
    numSteps->Delete();
  }

  return lineRead;
}

//----------------------------------------------------------------------------
int vtkPEnSightReader::ReadCaseFile()
{
  char line[256];
  char subLine[256];
  int stringRead;
  int i;
  int ret;
  vtkDebugMacro("In vtkPEnSightReader::ReadCaseFile");

  // Initialize
  //
  if (!this->CaseFileName)
  {
    vtkErrorMacro("A CaseFileName must be specified.");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += this->CaseFileName;
    vtkDebugMacro("full path to case file: " << sfilename.c_str());
  }
  else
  {
    sfilename = this->CaseFileName;
  }

  this->IS = new vtksys::ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = NULL;
    return 0;
  }

  this->TimeSets->RemoveAllItems();

  for (i = 0; i < this->NumberOfVariables; i++)
  {
    delete[] this->VariableFileNames[i];
    this->VariableFileNames[i] = NULL;
    delete[] this->VariableDescriptions[i];
    this->VariableDescriptions[i] = NULL;
  }
  delete[] this->VariableFileNames;
  this->VariableFileNames = NULL;
  delete[] this->VariableDescriptions;
  this->VariableDescriptions = NULL;
  delete[] this->VariableTypes;
  this->VariableTypes = NULL;

  for (i = 0; i < this->NumberOfComplexVariables; i++)
  {
    delete[] this->ComplexVariableFileNames[2 * i];
    this->ComplexVariableFileNames[2 * i] = NULL;
    delete[] this->ComplexVariableFileNames[2 * i + 1];
    this->ComplexVariableFileNames[2 * i + 1] = NULL;
    delete[] this->ComplexVariableDescriptions[i];
    this->ComplexVariableDescriptions[i] = NULL;
  }
  delete[] this->ComplexVariableFileNames;
  this->ComplexVariableFileNames = NULL;
  delete[] this->ComplexVariableDescriptions;
  this->ComplexVariableDescriptions = NULL;
  delete[] this->ComplexVariableTypes;
  this->ComplexVariableTypes = NULL;

  this->NumberOfVariables = 0;
  this->NumberOfComplexVariables = 0;

  this->ReadNextDataLine(line);

  if (strncmp(line, "FORMAT", 6) == 0)
  {
    // found the FORMAT section
    vtkDebugMacro("*** FORMAT section");
    this->ReadNextDataLine(line);

    stringRead = sscanf(line, " %*s %*s %s", subLine);
    if (stringRead == 1)
    {
      if (strcmp(subLine, "gold") == 0 && strcmp(this->GetClassName(), "vtkEnSight6Reader") == 0)
      {
        // The class is vtkEnSight6Reader, but the case file says "gold".
        vtkErrorMacro("This is not an EnSight6 file.");
        delete this->IS;
        this->IS = NULL;
        return 0;
      }
    }
    else
    {
      if (strcmp(this->GetClassName(), "vtkEnSightGoldReader") == 0)
      {
        // The class is vtkEnSightGoldReader, but the case file does
        // not say "gold".
        vtkErrorMacro("This is not an EnSight Gold file.");
        delete this->IS;
        this->IS = NULL;
        return 0;
      }
    }
  }

  ret = this->ReadNextDataLine(line);
  // We know how many lines to read in the FORMAT section, so we haven't read
  // the "GEOMETRY" line yet.
  while (ret)
  {
    if (strncmp(line, "GEOMETRY", 8) == 0)
    {
      // found the GEOMETRY section
      vtkDebugMacro("*** GEOMETRY section");
      ret = this->ReadCaseFileGeometry(line);
    }
    else if (strncmp(line, "VARIABLE", 8) == 0)
    {
      // found the VARIABLE section
      vtkDebugMacro(<< "*** VARIABLE section");
      ret = this->ReadCaseFileVariable(line);
    }
    else if (strncmp(line, "TIME", 4) == 0)
    {
      // found TIME section
      // 'TIME' section includes the following sub-sections
      // 'time set: <int>'
      // 'number of steps: <int>'
      // 'filename numbers:' --- either inline or not
      // 'filename start number: <int>' --- 'start' may be combined with 'increment'
      // 'filename increment: <int>'    --- to serve as an alternative to 'numbers'
      // 'time values:' --- either inline or not
      vtkDebugMacro(<< "*** TIME section");
      ret = this->ReadCaseFileTime(line);
    }
    else if (strncmp(line, "FILE", 4) == 0)
    {
      // found FILE section
      vtkDebugMacro(<< "*** FILE section");
      ret = this->ReadCaseFileFile(line);
    }
  }

  delete this->IS;
  this->IS = NULL;

  // Fill data array selection objects with these arrays.
  // TODO: Segfault or not segfault ? That is the question...
  this->SetDataArraySelectionSetsFromVariables();
  return 1;
  //  return ret;
}

//----------------------------------------------------------------------------
int vtkPEnSightReader::ReadVariableFiles(vtkMultiBlockDataSet* output)
{
  int i, j;
  char description[256];
  int timeSet, fileSet, timeStep, timeStepInFile, numSteps;
  vtkDataArray* times;
  float newTime;
  vtkIdList *numStepsList, *filenameNumbers;
  int fileNum;
  int validTime, filenameNum;
  char *fileName, *fileName2;

  for (i = 0; i < this->NumberOfVariables; i++)
  {
    switch (this->VariableTypes[i])
    {
      case SCALAR_PER_NODE:
      case VECTOR_PER_NODE:
      case TENSOR_SYMM_PER_NODE:
      case SCALAR_PER_MEASURED_NODE:
      case VECTOR_PER_MEASURED_NODE:
        if (!this->GetPointArrayStatus(this->VariableDescriptions[i]))
        {
          continue;
        }
        break;
      case SCALAR_PER_ELEMENT:
      case VECTOR_PER_ELEMENT:
      case TENSOR_SYMM_PER_ELEMENT:
        if (!this->GetCellArrayStatus(this->VariableDescriptions[i]))
        {
          continue;
        }
        break;
    }

    timeStep = 0;
    timeStepInFile = 1;
    fileNum = 0;
    validTime = 1;
    fileName = new char[strlen(this->VariableFileNames[i]) + 10];
    strcpy(fileName, this->VariableFileNames[i]);
    if (this->UseTimeSets)
    {
      validTime = 0;
      timeSet = this->VariableTimeSetIds->GetId(i);
      times = this->TimeSets->GetItem(this->TimeSetIds->IsId(timeSet));
      for (j = 0; j < times->GetNumberOfTuples(); j++)
      {
        newTime = times->GetComponent(j, 0);
        if (newTime <= this->ActualTimeValue)
        {
          timeStep++;
          if (this->VariableTypes[i] == SCALAR_PER_MEASURED_NODE ||
            this->VariableTypes[i] == VECTOR_PER_MEASURED_NODE)
          {
            if (newTime >= this->MeasuredTimeValue || this->MeasuredTimeSet == -1)
            {
              validTime = 1;
            }
          }
          else if (newTime >= this->GeometryTimeValue || this->GeometryTimeSet == -1)
          {
            validTime = 1;
          }
        }
      }
      if (this->TimeSetFileNameNumbers->GetNumberOfItems() > 0 && validTime)
      {
        int collectionNum = this->TimeSetsWithFilenameNumbers->IsId(timeSet);
        if (collectionNum > -1)
        {
          filenameNumbers = this->TimeSetFileNameNumbers->GetItem(collectionNum);
          filenameNum = filenameNumbers->GetId(timeStep - 1);
          // this->ReplaceWildcards(fileName, filenameNum);
          if (!this->UseFileSets)
          {
            this->ReplaceWildcards(fileName, filenameNum);
          }
        }
      }

      // There can only be file sets if there are also time sets.
      if (this->UseFileSets)
      {
        timeStepInFile = timeStep;
        fileSet = this->VariableFileSetIds->GetId(i);
        numStepsList = static_cast<vtkIdList*>(
          this->FileSetNumberOfSteps->GetItemAsObject(this->FileSets->IsId(fileSet)));

        if (timeStep > numStepsList->GetId(0))
        {
          numSteps = numStepsList->GetId(0);
          timeStepInFile -= numSteps;
          fileNum = 1;
          for (j = 1; j < numStepsList->GetNumberOfIds(); j++)
          {
            int curNumSteps = numStepsList->GetId(j);
            numSteps += curNumSteps;
            if (timeStep > numSteps)
            {
              fileNum++;
              timeStepInFile -= curNumSteps;
            }
          }
        }
        if (this->FileSetFileNameNumbers->GetNumberOfItems() > 0 && validTime)
        {
          int collectionNum = this->FileSetsWithFilenameNumbers->IsId(fileSet);
          if (collectionNum > -1)
          {
            filenameNumbers = this->FileSetFileNameNumbers->GetItem(collectionNum);
            filenameNum = filenameNumbers->GetId(fileNum);
            this->ReplaceWildcards(fileName, filenameNum);
          }
        }
      }
    }

    if (validTime)
    {
      switch (this->VariableTypes[i])
      {
        case vtkPEnSightReader::SCALAR_PER_NODE:
          this->ReadScalarsPerNode(fileName, this->VariableDescriptions[i], timeStepInFile, output);
          break;
        case vtkPEnSightReader::SCALAR_PER_MEASURED_NODE:
          this->ReadScalarsPerNode(
            fileName, this->VariableDescriptions[i], timeStepInFile, output, 1);
          break;
        case vtkPEnSightReader::VECTOR_PER_NODE:
          this->ReadVectorsPerNode(fileName, this->VariableDescriptions[i], timeStepInFile, output);
          break;
        case vtkPEnSightReader::VECTOR_PER_MEASURED_NODE:
          this->ReadVectorsPerNode(
            fileName, this->VariableDescriptions[i], timeStepInFile, output, 1);
          break;
        case vtkPEnSightReader::TENSOR_SYMM_PER_NODE:
          this->ReadTensorsPerNode(fileName, this->VariableDescriptions[i], timeStepInFile, output);
          break;
        case vtkPEnSightReader::SCALAR_PER_ELEMENT:
          this->ReadScalarsPerElement(
            fileName, this->VariableDescriptions[i], timeStepInFile, output);
          break;
        case vtkPEnSightReader::VECTOR_PER_ELEMENT:
          this->ReadVectorsPerElement(
            fileName, this->VariableDescriptions[i], timeStepInFile, output);
          break;
        case vtkPEnSightReader::TENSOR_SYMM_PER_ELEMENT:
          this->ReadTensorsPerElement(
            fileName, this->VariableDescriptions[i], timeStepInFile, output);
          break;
      }
    }
    delete[] fileName;
  }
  for (i = 0; i < this->NumberOfComplexVariables; i++)
  {
    switch (this->ComplexVariableTypes[i])
    {
      case COMPLEX_SCALAR_PER_NODE:
      case COMPLEX_VECTOR_PER_NODE:
        if (!this->GetPointArrayStatus(this->ComplexVariableDescriptions[i]))
        {
          continue;
        }
        break;
      case COMPLEX_SCALAR_PER_ELEMENT:
      case COMPLEX_VECTOR_PER_ELEMENT:
        if (!this->GetCellArrayStatus(this->ComplexVariableDescriptions[i]))
        {
          continue;
        }
        break;
    }
    timeStep = 0;
    timeStepInFile = 1;
    fileNum = 0;
    validTime = 1;
    fileName = new char[strlen(this->ComplexVariableFileNames[2 * i]) + 10];
    strcpy(fileName, this->ComplexVariableFileNames[2 * i]);
    fileName2 = new char[strlen(this->ComplexVariableFileNames[2 * i + 1]) + 10];
    strcpy(fileName2, this->ComplexVariableFileNames[2 * i + 1]);
    if (this->UseTimeSets)
    {
      validTime = 0;
      timeSet = this->VariableTimeSetIds->GetId(i);
      times = this->TimeSets->GetItem(this->TimeSetIds->IsId(timeSet));
      for (j = 0; j < times->GetNumberOfTuples(); j++)
      {
        newTime = times->GetComponent(j, 0);
        if (newTime <= this->ActualTimeValue)
        {
          timeStep++;
          if (this->VariableTypes[i] == SCALAR_PER_MEASURED_NODE ||
            this->VariableTypes[i] == VECTOR_PER_MEASURED_NODE)
          {
            if (newTime >= this->MeasuredTimeValue)
            {
              validTime = 1;
            }
          }
          else if (newTime >= this->GeometryTimeValue)
          {
            validTime = 1;
          }
        }
      }
      if (this->TimeSetFileNameNumbers->GetNumberOfItems() > 0 && validTime)
      {
        int collectionNum = this->TimeSetsWithFilenameNumbers->IsId(timeSet);
        if (collectionNum > -1)
        {
          filenameNumbers = this->TimeSetFileNameNumbers->GetItem(collectionNum);
          filenameNum = filenameNumbers->GetId(timeStep - 1);
          this->ReplaceWildcards(fileName, filenameNum);
          this->ReplaceWildcards(fileName2, filenameNum);
        }
      }

      // There can only be file sets if there are also time sets.
      if (this->UseFileSets)
      {
        timeStepInFile = timeStep;
        fileSet = this->VariableFileSetIds->GetId(i);
        numStepsList = static_cast<vtkIdList*>(
          this->FileSetNumberOfSteps->GetItemAsObject(this->FileSets->IsId(fileSet)));

        if (timeStep > numStepsList->GetId(0))
        {
          numSteps = numStepsList->GetId(0);
          timeStepInFile -= numSteps;
          fileNum = 1;
          for (j = 1; j < numStepsList->GetNumberOfIds(); j++)
          {
            int curNumSteps = numStepsList->GetId(j);
            numSteps += curNumSteps;
            if (timeStep > numSteps)
            {
              fileNum++;
              timeStepInFile -= curNumSteps;
            }
          }
        }
        if (this->FileSetFileNameNumbers->GetNumberOfItems() > 0 && validTime)
        {
          int collectionNum = this->FileSetsWithFilenameNumbers->IsId(fileSet);
          if (collectionNum > -1)
          {
            filenameNumbers = this->FileSetFileNameNumbers->GetItem(collectionNum);
            filenameNum = filenameNumbers->GetId(timeStep - 1);
            this->ReplaceWildcards(fileName, filenameNum);
            this->ReplaceWildcards(fileName2, filenameNum);
          }
        }
      }
    }

    if (validTime)
    {
      switch (this->ComplexVariableTypes[i])
      {
        case vtkPEnSightReader::COMPLEX_SCALAR_PER_NODE:
          this->ReadScalarsPerNode(
            fileName, this->ComplexVariableDescriptions[i], timeStepInFile, output, 0, 2);
          this->ReadScalarsPerNode(
            fileName2, this->ComplexVariableDescriptions[i], timeStepInFile, output, 0, 2, 1);
          break;
        case vtkPEnSightReader::COMPLEX_VECTOR_PER_NODE:
          strcpy(description, this->ComplexVariableDescriptions[i]);
          strcat(description, "_r");
          this->ReadVectorsPerNode(fileName, description, timeStepInFile, output);
          strcpy(description, this->ComplexVariableDescriptions[i]);
          strcat(description, "_i");
          this->ReadVectorsPerNode(fileName2, description, timeStepInFile, output);
          break;
        case vtkPEnSightReader::COMPLEX_SCALAR_PER_ELEMENT:
          this->ReadScalarsPerElement(
            fileName, this->ComplexVariableDescriptions[i], timeStepInFile, output, 2);
          this->ReadScalarsPerElement(
            fileName2, this->ComplexVariableDescriptions[i], timeStepInFile, output, 2, 1);
          break;
        case vtkPEnSightReader::COMPLEX_VECTOR_PER_ELEMENT:
          strcpy(description, this->ComplexVariableDescriptions[i]);
          strcat(description, "_r");
          this->ReadVectorsPerElement(fileName, description, timeStepInFile, output);
          strcpy(description, this->ComplexVariableDescriptions[i]);
          strcat(description, "_i");
          this->ReadVectorsPerElement(fileName2, description, timeStepInFile, output);
          break;
      }
    }
    delete[] fileName;
    delete[] fileName2;
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPEnSightReader::AddVariableFileName(const char* fileName1, const char* fileName2)
{
  int size;
  int i;

  if (this->VariableMode < 8)
  {
    size = this->NumberOfVariables;

    char** newFileNameList = new char*[size]; // temporary array

    // copy file names to temporary array
    for (i = 0; i < size; i++)
    {
      newFileNameList[i] = new char[strlen(this->VariableFileNames[i]) + 1];
      strcpy(newFileNameList[i], this->VariableFileNames[i]);
      delete[] this->VariableFileNames[i];
    }
    delete[] this->VariableFileNames;

    // make room for new file name
    this->VariableFileNames = new char*[size + 1];

    // copy existing file names back to first array
    for (i = 0; i < size; i++)
    {
      this->VariableFileNames[i] = new char[strlen(newFileNameList[i]) + 1];
      strcpy(this->VariableFileNames[i], newFileNameList[i]);
      delete[] newFileNameList[i];
    }
    delete[] newFileNameList;

    // add new file name at end of first array
    this->VariableFileNames[size] = new char[strlen(fileName1) + 1];
    strcpy(this->VariableFileNames[size], fileName1);
    vtkDebugMacro(<< "file name: " << this->VariableFileNames[size]);
  }
  else
  {
    size = this->NumberOfComplexVariables;

    char** newFileNameList = new char*[2 * size]; // temporary array

    // copy file names to temporary array
    for (i = 0; i < 2 * size; i++)
    {
      newFileNameList[i] = new char[strlen(this->ComplexVariableFileNames[i]) + 1];
      strcpy(newFileNameList[i], this->ComplexVariableFileNames[i]);
      delete[] this->ComplexVariableFileNames[i];
    }
    delete[] this->ComplexVariableFileNames;

    // make room for new file name
    this->ComplexVariableFileNames = new char*[2 * (size + 1)];

    // copy existing file names back to first array
    for (i = 0; i < 2 * size; i++)
    {
      this->ComplexVariableFileNames[i] = new char[strlen(newFileNameList[i]) + 1];
      strcpy(this->ComplexVariableFileNames[i], newFileNameList[i]);
      delete[] newFileNameList[i];
    }
    delete[] newFileNameList;

    // add new file name at end of first array
    this->ComplexVariableFileNames[2 * size] = new char[strlen(fileName1) + 1];
    strcpy(this->ComplexVariableFileNames[2 * size], fileName1);
    vtkDebugMacro("real file name: " << this->ComplexVariableFileNames[2 * size]);
    this->ComplexVariableFileNames[2 * size + 1] = new char[strlen(fileName2) + 1];
    strcpy(this->ComplexVariableFileNames[2 * size + 1], fileName2);
    vtkDebugMacro("imag. file name: " << this->ComplexVariableFileNames[2 * size + 1]);
  }
}

//----------------------------------------------------------------------------
void vtkPEnSightReader::AddVariableDescription(const char* description)
{
  int size;
  int i;

  if (this->VariableMode < 8)
  {
    size = this->NumberOfVariables;

    char** newDescriptionList = new char*[size]; // temporary array

    // copy descriptions to temporary array
    for (i = 0; i < size; i++)
    {
      newDescriptionList[i] = new char[strlen(this->VariableDescriptions[i]) + 1];
      strcpy(newDescriptionList[i], this->VariableDescriptions[i]);
      delete[] this->VariableDescriptions[i];
    }
    delete[] this->VariableDescriptions;

    // make room for new description
    this->VariableDescriptions = new char*[size + 1];

    // copy existing descriptions back to first array
    for (i = 0; i < size; i++)
    {
      this->VariableDescriptions[i] = new char[strlen(newDescriptionList[i]) + 1];
      strcpy(this->VariableDescriptions[i], newDescriptionList[i]);
      delete[] newDescriptionList[i];
    }
    delete[] newDescriptionList;

    // add new description at end of first array
    this->VariableDescriptions[size] = new char[strlen(description) + 1];
    strcpy(this->VariableDescriptions[size], description);
    vtkDebugMacro("description: " << this->VariableDescriptions[size]);
  }
  else
  {
    size = this->NumberOfComplexVariables;

    char** newDescriptionList = new char*[size]; // temporary array

    // copy descriptions to temporary array
    for (i = 0; i < size; i++)
    {
      newDescriptionList[i] = new char[strlen(this->ComplexVariableDescriptions[i]) + 1];
      strcpy(newDescriptionList[i], this->ComplexVariableDescriptions[i]);
      delete[] this->ComplexVariableDescriptions[i];
    }
    delete[] this->ComplexVariableDescriptions;

    // make room for new description
    this->ComplexVariableDescriptions = new char*[size + 1];

    // copy existing descriptions back to first array
    for (i = 0; i < size; i++)
    {
      this->ComplexVariableDescriptions[i] = new char[strlen(newDescriptionList[i]) + 1];
      strcpy(this->ComplexVariableDescriptions[i], newDescriptionList[i]);
      delete[] newDescriptionList[i];
    }
    delete[] newDescriptionList;

    // add new description at end of first array
    this->ComplexVariableDescriptions[size] = new char[strlen(description) + 1];
    strcpy(this->ComplexVariableDescriptions[size], description);
    vtkDebugMacro("description: " << this->ComplexVariableDescriptions[size]);
  }
}

//----------------------------------------------------------------------------
void vtkPEnSightReader::AddVariableType()
{
  int size;
  int i;
  int* types = NULL;

  // Figure out what the size of the variable type array is.
  if (this->VariableMode < 8)
  {
    size = this->NumberOfVariables;

    types = new int[size];

    for (i = 0; i < size; i++)
    {
      types[i] = this->VariableTypes[i];
    }
    delete[] this->VariableTypes;

    this->VariableTypes = new int[size + 1];
    for (i = 0; i < size; i++)
    {
      this->VariableTypes[i] = types[i];
    }
    delete[] types;
    this->VariableTypes[size] = this->VariableMode;
    vtkDebugMacro("variable type: " << this->VariableTypes[size]);
  }
  else
  {
    size = this->NumberOfComplexVariables;

    if (size > 0)
    {
      types = new int[size];
      for (i = 0; i < size; i++)
      {
        types[i] = this->ComplexVariableTypes[i];
      }
      delete[] this->ComplexVariableTypes;
    }

    this->ComplexVariableTypes = new int[size + 1];
    for (i = 0; i < size; i++)
    {
      this->ComplexVariableTypes[i] = types[i];
    }

    if (size > 0)
    {
      delete[] types;
    }
    this->ComplexVariableTypes[size] = this->VariableMode;
    vtkDebugMacro("complex variable type: " << this->ComplexVariableTypes[size]);
  }
}

//----------------------------------------------------------------------------
int vtkPEnSightReader::GetSectionType(const char* line)
{
  if (strncmp(line, "coordinates", 5) == 0)
  {
    return vtkPEnSightReader::COORDINATES;
  }
  else if (strncmp(line, "block", 4) == 0)
  {
    return vtkPEnSightReader::BLOCK;
  }
  else if (this->GetElementType(line) != -1)
  {
    return vtkPEnSightReader::ELEMENT;
  }
  else
  {
    return -1;
  }
}

//----------------------------------------------------------------------------
int vtkPEnSightReader::GetElementType(const char* line)
{
  if (strncmp(line, "point", 5) == 0)
  {
    return vtkPEnSightReader::POINT;
  }
  else if (strncmp(line, "bar2", 4) == 0)
  {
    return vtkPEnSightReader::BAR2;
  }
  else if (strncmp(line, "bar3", 4) == 0)
  {
    return vtkPEnSightReader::BAR3;
  }
  else if (strncmp(line, "nsided", 6) == 0)
  {
    return vtkPEnSightReader::NSIDED;
  }
  else if (strncmp(line, "tria3", 5) == 0)
  {
    return vtkPEnSightReader::TRIA3;
  }
  else if (strncmp(line, "tria6", 5) == 0)
  {
    return vtkPEnSightReader::TRIA6;
  }
  else if (strncmp(line, "quad4", 5) == 0)
  {
    return vtkPEnSightReader::QUAD4;
  }
  else if (strncmp(line, "quad8", 5) == 0)
  {
    return vtkPEnSightReader::QUAD8;
  }
  else if (strncmp(line, "nfaced", 6) == 0)
  {
    return vtkPEnSightReader::NFACED;
  }
  else if (strncmp(line, "tetra4", 6) == 0)
  {
    return vtkPEnSightReader::TETRA4;
  }
  else if (strncmp(line, "tetra10", 7) == 0)
  {
    return vtkPEnSightReader::TETRA10;
  }
  else if (strncmp(line, "pyramid5", 8) == 0)
  {
    return vtkPEnSightReader::PYRAMID5;
  }
  else if (strncmp(line, "pyramid13", 9) == 0)
  {
    return vtkPEnSightReader::PYRAMID13;
  }
  else if (strncmp(line, "hexa8", 5) == 0)
  {
    return vtkPEnSightReader::HEXA8;
  }
  else if (strncmp(line, "hexa20", 6) == 0)
  {
    return vtkPEnSightReader::HEXA20;
  }
  else if (strncmp(line, "penta6", 6) == 0)
  {
    return vtkPEnSightReader::PENTA6;
  }
  else if (strncmp(line, "penta15", 7) == 0)
  {
    return vtkPEnSightReader::PENTA15;
  }
  else
  {
    return -1;
  }
}

void vtkPEnSightReader::ReplaceWildcards(char* filename, int num)
{
  int wildcardPos, numWildcards, i, j;
  char pattern[32];
  char numStr[32];
  char filenameTmp[2048];
  int len, cnt, numStrLen;
  int foundWildcard = 0;

  wildcardPos = static_cast<int>(strcspn(filename, "*"));
  numWildcards = static_cast<int>(strspn(filename + wildcardPos, "*"));

  if (numWildcards < 1)
    return;
  else if (numWildcards == 1)
    strcpy(pattern, "%d");
  else
    sprintf(pattern, "%%0%dd", numWildcards);
  sprintf(numStr, pattern, num);
  numStrLen = static_cast<int>(strlen(numStr));
  len = static_cast<int>(strlen(filename));
  cnt = 0;
  for (i = 0; i < len; i++)
  {
    if (filename[i] == '*')
    {
      if (foundWildcard == 0)
      {
        for (j = 0; j < numStrLen; j++)
          filenameTmp[cnt++] = numStr[j];
        foundWildcard = 1;
      }
    }
    else
      filenameTmp[cnt++] = filename[i];
  }
  filenameTmp[cnt] = '\0';
  strcpy(filename, filenameTmp);
}

//----------------------------------------------------------------------------
void vtkPEnSightReader::RemoveLeadingBlanks(char* line)
{
  int count = 0;
  int len = static_cast<int>(strlen(line));
  while (line[count] == ' ')
  {
    count++;
  }
  memcpy(line, line + count, len - count + 1);
}

//----------------------------------------------------------------------------
vtkPEnSightReader::vtkPEnSightReaderCellIds* vtkPEnSightReader::GetCellIds(int index, int cellType)
{
  // Check argument range.
  if (cellType < POINT || cellType >= NUMBER_OF_ELEMENT_TYPES)
  {
    vtkErrorMacro("Cell type " << cellType << " out of range.  Only " << NUMBER_OF_ELEMENT_TYPES - 1
                               << " types exist.");
    return 0;
  }
  if (index < 0 || ((this->UnstructuredPartIds->IsId(index) == -1) &&
                     (this->StructuredPartIds->IsId(index) == -1)))
  {
    vtkErrorMacro("Index " << index << " out of range.  Only "
                           << this->UnstructuredPartIds->GetNumberOfIds() << " (unstructured) or "
                           << this->StructuredPartIds->GetNumberOfIds()
                           << " (structured) IDs exist.");
    return 0;
  }

  // Create the container if necessary.
  if (!this->CellIds)
  {
    this->CellIds = new vtkPEnSightReaderCellIdsType;
  }

  // Get the index of the actual vtkIdList requested.
  unsigned int cellIdsIndex = index * NUMBER_OF_ELEMENT_TYPES + cellType;

  // Make sure the container is large enough for this index.
  if (cellIdsIndex + 1 > this->CellIds->size())
  {
    this->CellIds->resize(cellIdsIndex + 1);
  }

  // Make sure this vtkIdList exists.
  if ((*this->CellIds)[cellIdsIndex] == NULL)
  {
    // TODO: FIXME: map consumes x12 times more memory than vector
    // we need another way of store these ids...
    if (this->StructuredPartIds->IsId(index) != -1)
    {
      (*this->CellIds)[cellIdsIndex] = new vtkPEnSightReaderCellIds(IMPLICIT_STRUCTURED_MODE);
    }
    else if (this->GetMultiProcessNumberOfProcesses() > 12)
      (*this->CellIds)[cellIdsIndex] = new vtkPEnSightReaderCellIds(SPARSE_MODE);
    else
      (*this->CellIds)[cellIdsIndex] = new vtkPEnSightReaderCellIds(NON_SPARSE_MODE);
  }

  // Return the requested vtkIdList.
  return (*this->CellIds)[cellIdsIndex];
}

//----------------------------------------------------------------------------
vtkPEnSightReader::vtkPEnSightReaderCellIds* vtkPEnSightReader::GetPointIds(int index)
{
  // Check argument range.
  if (index < 0 || ((this->UnstructuredPartIds->IsId(index) == -1) &&
                     (this->StructuredPartIds->IsId(index) == -1)))
  {
    vtkErrorMacro("Index " << index << " out of range.  Only "
                           << this->UnstructuredPartIds->GetNumberOfIds() << " (unstructured) or "
                           << this->StructuredPartIds->GetNumberOfIds()
                           << " (structured) IDs exist.");
    return 0;
  }

  // Create the container if necessary.
  if (!this->PointIds)
  {
    this->PointIds = new vtkPEnSightReaderCellIdsType;
  }

  // Make sure the container is large enough for this index.
  if ((unsigned int)(index + 1) > this->PointIds->size())
  {
    this->PointIds->resize(index + 1);
  }

  // Make sure this vtkIdList exists.
  if ((*this->PointIds)[index] == NULL)
  {
    // TODO: FIXME: map consumes x12 times more memory than vector
    if (this->StructuredPartIds->IsId(index) != -1)
    {
      (*this->PointIds)[index] = new vtkPEnSightReaderCellIds(IMPLICIT_STRUCTURED_MODE);
    }
    else if (this->GetMultiProcessNumberOfProcesses() > 12)
      (*this->PointIds)[index] = new vtkPEnSightReaderCellIds(SPARSE_MODE);
    else
      (*this->PointIds)[index] = new vtkPEnSightReaderCellIds(NON_SPARSE_MODE);
  }

  // Return the requested vtkIdList.
  return (*this->PointIds)[index];
}

//----------------------------------------------------------------------------
vtkIdType vtkPEnSightReader::GetTotalNumberOfCellIds(int index)
{
  int i;
  vtkIdType result = 0;

  if (index < 0 || ((this->UnstructuredPartIds->IsId(index) == -1) &&
                     (this->StructuredPartIds->IsId(index) == -1)))
  {
    vtkErrorMacro("Index " << index << " out of range.  Only "
                           << this->UnstructuredPartIds->GetNumberOfIds() << " (unstructured) or "
                           << this->StructuredPartIds->GetNumberOfIds()
                           << " (structured) IDs exist.");
    return 0;
  }

  if (this->UnstructuredPartIds->IsId(index) != -1)
  {
    for (i = 0; i < NUMBER_OF_ELEMENT_TYPES; i++)
    {
      result += this->GetCellIds(index, i)->GetNumberOfIds();
    }
    return result;
  }
  else
  {
    return this->GetCellIds(index, 0)->GetNumberOfIds();
  }
}

vtkIdType vtkPEnSightReader::GetLocalTotalNumberOfCellIds(int index)
{
  int i;
  vtkIdType numCellIds = 0;

  if (index < 0 || ((this->UnstructuredPartIds->IsId(index) == -1) &&
                     (this->StructuredPartIds->IsId(index) == -1)))
  {
    vtkErrorMacro("Index " << index << " out of range.  Only "
                           << this->UnstructuredPartIds->GetNumberOfIds() << " (unstructured) or "
                           << this->StructuredPartIds->GetNumberOfIds()
                           << " (structured) IDs exist.");
    return 0;
  }

  if (this->UnstructuredPartIds->IsId(index) != -1)
  {
    for (i = 0; i < NUMBER_OF_ELEMENT_TYPES; i++)
    {
      int local = this->GetCellIds(index, i)->GetLocalNumberOfIds();
      numCellIds += local;
    }
    return numCellIds;
  }
  else
  {
    return this->GetCellIds(index, 0)->GetLocalNumberOfIds();
  }
}

//----------------------------------------------------------------------------
void vtkPEnSightReader::InsertVariableComponent(vtkFloatArray* array, int i, int component,
  float* content, int partId, int ensightCellType, int insertionType)
{

  vtkIdType realId;
  // realId is either an Element real Id or a Node real Id.
  if ((insertionType == SCALAR_PER_ELEMENT) || (insertionType == VECTOR_PER_ELEMENT) ||
    (insertionType == TENSOR_SYMM_PER_ELEMENT))
    realId = this->GetCellIds(partId, ensightCellType)->GetId(i);
  else
    realId = this->GetPointIds(partId)->GetId(i);

  if (realId != -1)
  {
    switch (insertionType)
    {
      case SCALAR_PER_ELEMENT:
      case SCALAR_PER_NODE:
        array->SetComponent(realId, component, *content);
        break;
      case VECTOR_PER_ELEMENT:
      case VECTOR_PER_NODE:
      case TENSOR_SYMM_PER_ELEMENT:
      case TENSOR_SYMM_PER_NODE:
        array->SetTuple(realId, content);
        break;
      default:
        break;
    }
  }
  else
  {
    // do nothing
  }
}

//----------------------------------------------------------------------------
void vtkPEnSightReader::MapToGlobalIds(
  const vtkIdType* inputIds, vtkIdType numPoints, int partId, vtkIdType* globalIds)
{
  for (vtkIdType i = 0; i < numPoints; i++)
  {
    int realId = this->GetPointIds(partId)->GetId(inputIds[i]);
    if (realId == -1)
    {
      this->GetPointIds(partId)->SetId(inputIds[i], this->LastPointId);
      globalIds[i] = this->LastPointId;
      this->LastPointId++;
    }
    else
    {
      globalIds[i] = realId;
    }
  }
}

//----------------------------------------------------------------------------
void vtkPEnSightReader::InsertNextCellAndId(vtkUnstructuredGrid* output, int vtkCellType,
  vtkIdType numPoints, vtkIdType* points, int partId, int ensightCellType, vtkIdType globalId,
  vtkIdType numElements, const std::vector<vtkIdType>& faces)
{
  // Reader is Distributed. Insert If necessary, and keep global Id trace
  // Should be based on pointIds, aka points, but for now it is based on globalId
  int mpiLocalProcessId = this->GetMultiProcessLocalProcessId();
  int mpiNumberOfProcesses = this->GetMultiProcessNumberOfProcesses();
  vtkIdType numElnts = (numElements / mpiNumberOfProcesses) + 1;
  vtkIdType begin = mpiLocalProcessId * numElnts;

  if ((globalId >= begin) && (globalId < (begin + numElnts)))
  {
    // First note the points : they will be injected later
    vtkIdType* newPoints = new vtkIdType[numPoints];
    this->MapToGlobalIds(points, numPoints, partId, newPoints);

    vtkIdType cellId;
    if (vtkCellType == VTK_POLYHEDRON)
    {
      std::vector<vtkIdType> newFaces;
      newFaces.reserve(faces.size());
      vtkIdType numFace = 0;
      // faces is sizeOfFace0, pt0, ... ptn, sizeOfFace1, pts...
      for (size_t idx = 0; idx < faces.size();)
      {
        vtkIdType faceSize = faces[idx];
        newFaces[idx] = faceSize;
        idx++;
        this->MapToGlobalIds(&faces[idx], faceSize, partId, &newFaces[idx]);
        idx += faceSize;
        numFace++;
      }
      cellId = output->InsertNextCell(vtkCellType, numPoints, newPoints, numFace, newFaces.data());
    }
    else
    {
      cellId = output->InsertNextCell(vtkCellType, numPoints, newPoints);
    }

    this->GetCellIds(partId, ensightCellType)->InsertNextId(cellId);
    delete[] newPoints;

    this->CoordinatesAtEnd = true;
    this->InjectGlobalElementIds = true;
  }
  else
  {
    // Do not insert it. Set the index to "-1".
    this->GetCellIds(partId, ensightCellType)->InsertNextId(-1);
  }
}

//----------------------------------------------------------------------------
void vtkPEnSightReader::PrepareStructuredDimensionsForDistribution(int partId, int* oldDimensions,
  int* newDimensions, int* splitDimension, int* splitDimensionBeginIndex, int ghostLevel,
  vtkUnsignedCharArray* pointGhostArray, vtkUnsignedCharArray* cellGhostArray)
{
  // Structured decomposition:
  // 1. Find the largest dimension
  // 2. Only take the 1 / NumProc of the dimension, based on the local process Id

  *splitDimension = (oldDimensions[0] > oldDimensions[1])
    ? ((oldDimensions[0] > oldDimensions[2]) ? 0 : 2)
    : ((oldDimensions[1] > oldDimensions[2]) ? 1 : 2);

  // Compute Real number of cells, points, with IJK order.

  // Cells
  int mpiLocalProcessId = this->GetMultiProcessLocalProcessId();
  int mpiNumberOfProcesses = this->GetMultiProcessNumberOfProcesses();
  int oldCellDimension = oldDimensions[*splitDimension] - 1;
  int newCellDimension = (oldCellDimension / mpiNumberOfProcesses) + 1;
  int cellBeginIndex = mpiLocalProcessId * newCellDimension;
  if ((cellBeginIndex + newCellDimension) > oldCellDimension)
    newCellDimension = oldCellDimension - cellBeginIndex;

  int oldCellDimensions[3];
  oldCellDimensions[0] = (oldDimensions[0] == 1) ? 1 : (oldDimensions[0] - 1);
  oldCellDimensions[1] = (oldDimensions[1] == 1) ? 1 : (oldDimensions[1] - 1);
  oldCellDimensions[2] = (oldDimensions[2] == 1) ? 1 : (oldDimensions[2] - 1);

  // Points
  int newPointDimension = newCellDimension + 1;
  int pointBeginIndex = cellBeginIndex;

  if (ghostLevel > 0)
  {
    // compute new limits
    int cellEndIndex = cellBeginIndex + newCellDimension;
    int pointEndIndex = pointBeginIndex + newPointDimension;

    // keep non ghost information
    int realCellBegin = cellBeginIndex;
    int realCellEnd = cellEndIndex;
    int realPointBegin = pointBeginIndex;
    int realPointEnd = pointEndIndex;

    cellBeginIndex -= ghostLevel;
    if (cellBeginIndex < 0)
      cellBeginIndex = 0;
    cellEndIndex += ghostLevel;
    if (cellEndIndex > oldCellDimension)
      cellEndIndex = oldCellDimension;
    newCellDimension = cellEndIndex - cellBeginIndex;

    newPointDimension = newCellDimension + 1;
    pointBeginIndex = cellBeginIndex;
    pointEndIndex = pointBeginIndex + newPointDimension;

    // generate ghost points/cells arrays
    int index = 0;
    for (int k = 0; k < oldDimensions[2]; k++)
    {
      for (int j = 0; j < oldDimensions[1]; j++)
      {
        for (int i = 0; i < oldDimensions[0]; i++)
        {
          int n = (*splitDimension == 0) ? i : ((*splitDimension == 1) ? j : k);
          if ((n >= pointBeginIndex) && (n < pointEndIndex))
          {
            // is it in the "ghost zones" ?
            if (n < realPointBegin)
            {
              int gl = realPointBegin - n;
              pointGhostArray->InsertNextValue(gl);
            }
            else if (n >= realPointEnd)
            {
              int gl = n - realPointEnd + 1;
              pointGhostArray->InsertNextValue(gl);
            }
            else
            {
              pointGhostArray->InsertNextValue(0);
            }
            index++;
          }
        }
      }
    }

    index = 0;
    for (int k = 0; k < oldCellDimensions[2]; k++)
    {
      for (int j = 0; j < oldCellDimensions[1]; j++)
      {
        for (int i = 0; i < oldCellDimensions[0]; i++)
        {
          int n = (*splitDimension == 0) ? i : ((*splitDimension == 1) ? j : k);
          if ((n >= cellBeginIndex) && (n < cellEndIndex))
          {
            // is it in the "ghost zones" ?
            if (n < realCellBegin)
            {
              int gl = realCellBegin - n;
              cellGhostArray->InsertNextValue(gl);
            }
            else if (n >= realCellEnd)
            {
              int gl = n - realCellEnd + 1;
              cellGhostArray->InsertNextValue(gl);
            }
            else
            {
              cellGhostArray->InsertNextValue(0);
            }
            index++;
          }
        }
      }
    }
  }

  // Unstructured compatibility.
  // We generate PointIds and CellIds arrays.
  this->GetPointIds(partId)->Reset();
  this->GetCellIds(partId, 0)->Reset();

  int dim = *splitDimension;
  newDimensions[dim] = newPointDimension;
  dim = NEXTMODULO3(dim);
  newDimensions[dim] = oldDimensions[dim];
  dim = NEXTMODULO3(dim);
  newDimensions[dim] = oldDimensions[dim];

  *splitDimensionBeginIndex = pointBeginIndex;

  // Cell Ids
  int newCellDimensions[3];
  // If point dimension is 1, then cell dimension is 1, not 0
  newCellDimensions[0] = (newDimensions[0] == 1) ? 1 : (newDimensions[0] - 1);
  newCellDimensions[1] = (newDimensions[1] == 1) ? 1 : (newDimensions[1] - 1);
  newCellDimensions[2] = (newDimensions[2] == 1) ? 1 : (newDimensions[2] - 1);

  // IMPLICIT IDS
  // Point Ids
  // As Id has been injected in this->StructuredPartIds, PointIds(partId) is in
  // Implicit Structured mode (or in single process mode)
  this->GetPointIds(partId)->SetNumberOfIds(oldDimensions[0] * oldDimensions[1] * oldDimensions[2]);
  this->GetPointIds(partId)->SetLocalNumberOfIds(
    newDimensions[0] * newDimensions[1] * newDimensions[2]);
  this->GetPointIds(partId)->SetImplicitDimensions(
    oldDimensions[0], oldDimensions[1], oldDimensions[2]);
  this->GetPointIds(partId)->SetImplicitSplitDimension(*splitDimension);
  this->GetPointIds(partId)->SetImplicitSplitDimensionBeginIndex(pointBeginIndex);
  this->GetPointIds(partId)->SetImplicitSplitDimensionEndIndex(pointBeginIndex + newPointDimension);

  this->GetCellIds(partId, 0)->SetNumberOfIds(
    oldCellDimensions[0] * oldCellDimensions[1] * oldCellDimensions[2]);
  this->GetCellIds(partId, 0)->SetLocalNumberOfIds(
    newCellDimensions[0] * newCellDimensions[1] * newCellDimensions[2]);
  this->GetCellIds(partId, 0)->SetImplicitDimensions(
    oldCellDimensions[0], oldCellDimensions[1], oldCellDimensions[2]);
  this->GetCellIds(partId, 0)->SetImplicitSplitDimension(*splitDimension);
  this->GetCellIds(partId, 0)->SetImplicitSplitDimensionBeginIndex(cellBeginIndex);
  this->GetCellIds(partId, 0)->SetImplicitSplitDimensionEndIndex(cellBeginIndex + newCellDimension);
}

//----------------------------------------------------------------------------
void vtkPEnSightReader::AddToBlock(
  vtkMultiBlockDataSet* output, unsigned int blockNo, vtkDataSet* dataset)
{
  vtkDataObject* blockDO = output->GetBlock(blockNo);
  if (blockDO)
  {
    vtkErrorMacro("Block already has a vtkDataSet assigned to it.");
    return;
  }

  output->SetBlock(blockNo, dataset);
}

//----------------------------------------------------------------------------
vtkDataSet* vtkPEnSightReader::GetDataSetFromBlock(
  vtkMultiBlockDataSet* output, unsigned int blockno)
{
  return vtkDataSet::SafeDownCast(output->GetBlock(blockno));
}

//----------------------------------------------------------------------------
void vtkPEnSightReader::SetBlockName(
  vtkMultiBlockDataSet* output, unsigned int blockNo, const char* name)
{
  output->GetMetaData(blockNo)->Set(vtkCompositeDataSet::NAME(), name);
}

//----------------------------------------------------------------------------
void vtkPEnSightReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CaseFileName: " << (this->CaseFileName ? this->CaseFileName : "(none)") << endl;
  os << indent << "FilePath: " << (this->FilePath ? this->FilePath : "(none)") << endl;
  os << indent << "NumberOfComplexScalarsPerNode: " << this->NumberOfComplexScalarsPerNode << endl;
  os << indent << "NumberOfVectorsPerElement :" << this->NumberOfVectorsPerElement << endl;
  os << indent << "NumberOfTensorsSymmPerElement: " << this->NumberOfTensorsSymmPerElement << endl;
  os << indent << "NumberOfComplexVectorsPerNode: " << this->NumberOfComplexVectorsPerNode << endl;
  os << indent << "NumberOfScalarsPerElement: " << this->NumberOfScalarsPerElement << endl;
  os << indent << "NumberOfComplexVectorsPerElement: " << this->NumberOfComplexVectorsPerElement
     << endl;
  os << indent << "NumberOfComplexScalarsPerElement: " << this->NumberOfComplexScalarsPerElement
     << endl;
  os << indent << "NumberOfTensorsSymmPerNode: " << this->NumberOfTensorsSymmPerNode << endl;
  os << indent << "NumberOfScalarsPerMeasuredNode: " << this->NumberOfScalarsPerMeasuredNode
     << endl;
  os << indent << "NumberOfVectorsPerMeasuredNode: " << this->NumberOfVectorsPerMeasuredNode
     << endl;
  os << indent << "NumberOfScalarsPerNode: " << this->NumberOfScalarsPerNode << endl;
  os << indent << "NumberOfVectorsPerNode: " << this->NumberOfVectorsPerNode << endl;
  os << indent << "TimeValue: " << this->TimeValue << endl;
  os << indent << "MinimumTimeValue: " << this->MinimumTimeValue << endl;
  os << indent << "MaximumTimeValue: " << this->MaximumTimeValue << endl;
  os << indent << "TimeSets: " << this->TimeSets << endl;
  os << indent
     << "MeasuredFileName: " << (this->MeasuredFileName ? this->MeasuredFileName : "(none)")
     << endl;
  os << indent << "MatchFileName: " << (this->MatchFileName ? this->MatchFileName : "(none)")
     << endl;
  os << indent << "UseTimeSets: " << this->UseTimeSets << endl;
  os << indent << "UseFileSets: " << this->UseFileSets << endl;
}
