/*=========================================================================

  Program:   ParaView
  Module:    vtkGmshMetaReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// -------------------------------------------------------------------
// ParaViewGmshReaderPlugin - Copyright (C) 2015 Cenaero
//
// See the Copyright.txt and License.txt files provided
// with ParaViewGmshReaderPlugin for license information.
//
// -------------------------------------------------------------------

#include "vtkGmshMetaReader.h"

#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkFieldData.h"
#include "vtkGmshReader.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include <vtksys/SystemTools.hxx>

#include <cmath>
#include <map>
#include <sstream>
#include <string>

//-----------------------------------------------------------------------------
struct vtkGmshMetaReaderInternal
{
  struct TimeStepInfo
  {
    int GeomIndex;
    int FieldIndex;
    double TimeValue;

    TimeStepInfo()
      : GeomIndex(-1)
      , FieldIndex(-1)
      , TimeValue(0.0)
    {
    }
  };

  typedef std::map<int, TimeStepInfo> TimeStepInfoMapType;
  TimeStepInfoMapType TimeStepInfoMap;
  typedef std::map<int, vtkSmartPointer<vtkUnstructuredGrid> > CachedGridsMapType;
  CachedGridsMapType CachedGrids;
  std::map<std::string, std::string> ArrayNameMap;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkGmshMetaReader);

//----------------------------------------------------------------------------
vtkGmshMetaReader::vtkGmshMetaReader()
{
  this->FileName = nullptr;

  this->TimeStepIndex = 0;
  this->ActualTimeStep = 0;

  this->SetNumberOfInputPorts(0);

  this->Parser = nullptr;

  this->Internal = new vtkGmshMetaReaderInternal;

  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;

  // Setup the selection callback to modify this object when an array
  // selection is changed.
  this->SelectionObserver->SetCallback(&vtkGmshMetaReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->PointDataArraySelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);

  this->AdaptationLevel = 1;
  this->AdaptationTolerance = -0.0001;
}

//----------------------------------------------------------------------------
vtkGmshMetaReader::~vtkGmshMetaReader()
{
  this->SetFileName(nullptr);

  this->PointDataArraySelection->RemoveObserver(this->SelectionObserver);

  if (this->Parser)
  {
    this->Parser->Delete();
    this->Parser = nullptr;
  }

  delete this->Internal;
}

//----------------------------------------------------------------------------
int vtkGmshMetaReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  // get the data object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  this->ActualTimeStep = this->TimeStepIndex;

  int tsLength = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  double* steps = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  // Check if a particular time was requested.
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    // Get the requested time step. We only support requests of a single time
    // step in this reader right now
    double requestedTimeSteps = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    double timeValue = requestedTimeSteps;

    // find the first time value larger than requested time value
    this->ActualTimeStep = tsLength - 1;
    for (int cnt = 0; cnt < tsLength - 1 && steps[cnt] < timeValue; cnt++)
    {
      if (steps[cnt] >= timeValue)
      {
        this->ActualTimeStep = cnt;
        break;
      }
    }
  }

  if (this->ActualTimeStep > this->TimeStepRange[1])
  {
    vtkErrorMacro("Timestep index too large.");
    return 0;
  }

  // get the current piece being requested
  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numProcPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  // this is actually # of proc
  vtkDebugMacro("numProcPieces (i.e. # of proc) = " << numProcPieces);

  if (!this->Parser)
  {
    vtkErrorMacro("No parser was created. Cannot read file");
    return 0;
  }

  vtkPVXMLElement* rootElement = this->Parser->GetRootElement();

  int numFiles;
  int numPieces;

  if (!rootElement->GetScalarAttribute("number_of_files", &numFiles) &&
    !rootElement->GetScalarAttribute("number_of_partitioned_msh_files", &numFiles))
  {
    numFiles = 1;
  }

  // This variables will hopefully be used once a distrbuted Gmsh file format becomes available
  // For now, this variable must be equal to numFiles
  // if (!rootElement->GetScalarAttribute("number_of_pieces", &numPieces)) {
  numPieces = numFiles;

  this->Reader->SetNumPieces(numPieces);
  this->Reader->SetNumFiles(numFiles);
  this->Reader->SetAdaptInfo(this->AdaptationLevel, this->AdaptationTolerance);

  vtkMultiBlockDataSet* output =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  output->SetNumberOfBlocks(1);

  vtkNew<vtkMultiPieceDataSet> multiPieceDataSet;
  multiPieceDataSet->SetNumberOfPieces(numPieces);
  output->SetBlock(0, multiPieceDataSet.Get());

  const char* filePathPattern = nullptr;
  const char* geometryPath = nullptr;

  unsigned int numElements = rootElement->GetNumberOfNestedElements();
  for (unsigned int i = 0; i < numElements; i++)
  {
    vtkPVXMLElement* nested = rootElement->GetNestedElement(i);

    if (strcmp("GeometryInfo", nested->GetName()) == 0)
    {
      geometryPath = nested->GetAttribute("path");
    }

    if (strcmp("FieldInfo", nested->GetName()) == 0)
    {
      unsigned int numElements2 = nested->GetNumberOfNestedElements();
      for (unsigned int j = 0; j < numElements2; j++)
      {
        // Several field file patterns are allowed
        vtkPVXMLElement* nested2 = nested->GetNestedElement(j);
        if (strcmp("PathList", nested2->GetName()) == 0)
        {
          filePathPattern = nested2->GetAttribute("path");
          if (!filePathPattern)
          {
            vtkErrorMacro("No field path was provided");
          }
          else
          {
            const std::string& arrayName = this->Internal->ArrayNameMap[filePathPattern];
            if (this->GetPointArrayStatus(arrayName.c_str()))
            {
              this->Reader->SetFieldInfoPath(filePathPattern);
            }
          }
        }
      }
    }
  }

  if (!geometryPath)
  {
    vtkErrorMacro("No geometry pattern was specified. Cannot load file");
    return 0;
  }

  int numPiecesPerFile = numPieces / numFiles;

  if (numPieces % numFiles != 0)
  {
    vtkErrorMacro(
      "The mesh pieces are not uniformally spread in the msh files: numPieces%numFiles = "
      << numPieces % numFiles);
  }

  // now loop over all of the files that need to be loaded
  for (int loadingPiece = piece; loadingPiece < numPieces; loadingPiece += numProcPieces)
  {
    int timeStep = this->Internal->TimeStepInfoMap[this->ActualTimeStep].FieldIndex;
    int fileID = static_cast<int>(std::floor(loadingPiece / numPiecesPerFile)) + 1;
    int partID = loadingPiece + 1;

    this->Reader->SetTimeStep(timeStep);
    this->Reader->SetFileID(fileID);
    this->Reader->SetPartID(partID);

    vtkDebugMacro("vtkGmshMetaReader::RequestData() in loop, piece="
      << piece << ", loadingPiece+1=" << loadingPiece + 1 << ", numPieces=" << numPieces
      << ", fileID=" << fileID << ", numProcPieces=" << numProcPieces << ", timeStep=" << timeStep);

    std::string geomFName(geometryPath);
    std::string pIdentifier;

    pIdentifier = "[partID]";
    this->Reader->ReplaceAllStringPattern(geomFName, pIdentifier, fileID);

    // Test the old format with 6 digits with zero padding (000001, etc)
    if (fileID < 1e7)
    {
      pIdentifier = "[zeroPadPartID]";
      std::ostringstream paddedFileID;
      paddedFileID << std::setw(6) << std::setfill('0') << fileID;
      this->Reader->ReplaceAllStringPattern(geomFName, pIdentifier, paddedFileID.str());
    }

    pIdentifier = "[step]";
    this->Reader->ReplaceAllStringPattern(
      geomFName, pIdentifier, this->Internal->TimeStepInfoMap[this->ActualTimeStep].GeomIndex);

    // Returns the directory path specified in the xml file without the geom file name
    std::string gpath = vtksys::SystemTools::GetFilenamePath(geomFName);

    if (gpath.empty() || !vtksys::SystemTools::FileIsFullPath(gpath.c_str()))
    {
      // Returns the directory path of the xml file
      std::string path = vtksys::SystemTools::GetFilenamePath(this->FileName);
      if (!path.empty())
      {
        geomFName = path + '/' + geomFName;
      }
    }

    this->Reader->SetGeometryFileName(geomFName.c_str());
    this->Reader->SetXMLFileName(this->FileName);
    this->Reader->Update();

    vtkNew<vtkUnstructuredGrid> copy;
    copy->ShallowCopy(this->Reader->GetOutput());
    multiPieceDataSet->SetPiece(loadingPiece, copy.Get());
  }

  if (steps)
  {
    output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), steps[this->ActualTimeStep]);
  }

  // clean up data
  this->Reader->ClearFieldInfo();

  return 1;
}

//----------------------------------------------------------------------------
namespace
{
// Convert the fileName to a human readable name for the GUI
std::string FormatArrayName(const std::string& fileName)
{
  std::string outFieldName = fileName;
  std::size_t s = outFieldName.find_last_of('/');
  if (s != std::string::npos)
  {
    outFieldName = outFieldName.substr(s + 1);
  }
  // Typically, time steps and iterations are sperated from the main name with dots or under scores.
  // Split the name of the file based on these characters.
  s = outFieldName.find(".");
  if (s != std::string::npos)
  {
    outFieldName = outFieldName.substr(0, s);
  }
  s = outFieldName.find("_");
  if (s != std::string::npos)
  {
    outFieldName = outFieldName.substr(0, s);
  }
  return outFieldName;
}
}

//----------------------------------------------------------------------------
int vtkGmshMetaReader::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  this->Internal->ArrayNameMap.clear();
  this->Internal->TimeStepInfoMap.clear();
  this->Reader->ClearFieldInfo();

  if (this->Parser)
  {
    this->Parser->Delete();
    this->Parser = nullptr;
  }

  if (!this->FileName)
  {
    vtkErrorMacro("FileName has to be specified.");
    return 0;
  }

  vtkNew<vtkPVXMLParser> parser;
  parser->SetFileName(this->FileName);

  if (!parser->Parse())
  {
    vtkErrorMacro("Unable to parse Gmsh file!");
    return 0;
  }

  vtkPVXMLElement* rootElement = parser->GetRootElement();
  if (!rootElement)
  {
    vtkErrorMacro("Unable to parse Gmsh file!");
    return 0;
  }

  if (strcmp(rootElement->GetName(), "GmshMetaFile") != 0)
  {
    vtkErrorMacro("This is not a Gmsh metafile.");
    return 0;
  }

  this->Parser = parser.Get();
  this->Parser->Register(this);

  int numTimeSteps = 1;
  int hasTimeValues = 0;

  unsigned int numElements = rootElement->GetNumberOfNestedElements();
  for (unsigned int i = 0; i < numElements; i++)
  {
    vtkPVXMLElement* nested = rootElement->GetNestedElement(i);
    if (strcmp("TimeSteps", nested->GetName()) == 0)
    {
      if (!nested->GetScalarAttribute("number_of_steps", &numTimeSteps))
      {
        numTimeSteps = 1;
      }

      int autoGen;
      if (!nested->GetScalarAttribute("auto_generate_indices", &autoGen))
      {
        autoGen = 0;
      }

      int indexIncr;
      if (!nested->GetScalarAttribute("increment_index_by", &indexIncr))
      {
        indexIncr = 1;
      }

      int startIndex;
      if (!nested->GetScalarAttribute("start_index", &startIndex))
      {
        startIndex = 0;
      }

      double startValue = 0.;
      if (nested->GetScalarAttribute("start_value", &startValue))
      {
        hasTimeValues = 1;
      }

      double valueIncr = 1. * indexIncr;
      if (nested->GetScalarAttribute("increment_value_by", &valueIncr))
      {
        hasTimeValues = 1;
      }

      if (autoGen)
      {
        for (int j = 0; j < numTimeSteps; j++)
        {
          vtkGmshMetaReaderInternal::TimeStepInfo& info = this->Internal->TimeStepInfoMap[j];
          info.GeomIndex = startIndex;
          info.FieldIndex = startIndex;
          info.TimeValue = startValue;
          startIndex += indexIncr;
          startValue += valueIncr;
        }
      }

      unsigned int numElements2 = nested->GetNumberOfNestedElements();
      for (unsigned int j = 0; j < numElements2; j++)
      {
        vtkPVXMLElement* nested2 = nested->GetNestedElement(j);
        if (strcmp("TimeStep", nested2->GetName()) == 0)
        {
          int index;
          if (nested2->GetScalarAttribute("index", &index))
          {
            if ((index + 1) > numTimeSteps)
            {
              numTimeSteps = index + 1;
            }
            vtkGmshMetaReaderInternal::TimeStepInfo& info = this->Internal->TimeStepInfoMap[index];
            int gIdx;
            if (nested2->GetScalarAttribute("geometry_index", &gIdx))
            {
              info.GeomIndex = gIdx;
            }
            int fIdx;
            if (nested2->GetScalarAttribute("field_index", &fIdx))
            {
              info.FieldIndex = fIdx;
            }
            double val;
            if (nested2->GetScalarAttribute("value", &val))
            {
              info.TimeValue = val;
              hasTimeValues = 1;
            }
          }
        }
      }
      break;
    }
    if (strcmp("FieldInfo", nested->GetName()) == 0)
    {
      unsigned int numElements2 = nested->GetNumberOfNestedElements();
      for (unsigned int j = 0; j < numElements2; j++)
      {
        // Several field file paths are allowed
        vtkPVXMLElement* nested2 = nested->GetNestedElement(j);
        if (strcmp("PathList", nested2->GetName()) == 0)
        {
          const char* filePathPattern = nested2->GetAttribute("path");
          if (!filePathPattern)
          {
            vtkErrorMacro("No field path was provided");
          }
          else
          {
            std::string fileName = ::FormatArrayName(filePathPattern);
            this->Internal->ArrayNameMap[filePathPattern] = fileName;
            this->PointDataArraySelection->AddArray(fileName.c_str());
          }
        }
      }
    }
  }

  this->AdaptationLevelInfo = 0;
  this->AdaptationToleranceInfo = -0.0001;
  for (unsigned int i = 0; i < numElements; i++)
  {
    vtkPVXMLElement* nested = rootElement->GetNestedElement(i);
    if (strcmp("AdaptView", nested->GetName()) == 0)
    {
      nested->GetScalarAttribute("adaptation_level", &this->AdaptationLevelInfo);
      nested->GetScalarAttribute("adaptation_tol", &this->AdaptationToleranceInfo);
      break;
    }
  }
  this->Reader->SetAdaptInfo(this->AdaptationLevelInfo, this->AdaptationToleranceInfo);

  // Make sure all indices are there
  for (int tidx = 1; tidx < numTimeSteps; tidx++)
  {
    vtkGmshMetaReaderInternal::TimeStepInfoMapType::iterator iter =
      this->Internal->TimeStepInfoMap.find(tidx);
    if (iter == this->Internal->TimeStepInfoMap.end())
    {
      vtkErrorMacro("Missing timestep, index=" << tidx);
      return 0;
    }
  }

  if (hasTimeValues)
  {
    std::vector<double> timeSteps(numTimeSteps);
    for (int tidx = 0; tidx < numTimeSteps; tidx++)
    {
      timeSteps[tidx] = this->Internal->TimeStepInfoMap[tidx].TimeValue;
    }
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timeSteps[0], numTimeSteps);
    double timeRange[2];
    timeRange[0] = timeSteps[0];
    timeRange[1] = timeSteps[numTimeSteps - 1];
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }

  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = numTimeSteps - 1;

  vtkInformation* info = outputVector->GetInformationObject(0);
  info->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  return 1;
}

//-----------------------------------------------------------------------------
bool vtkGmshMetaReader::CanReadFile(const char* filename)
{
  vtkNew<vtkPVXMLParser> parser;
  parser->SuppressErrorMessagesOn();
  parser->SetFileName(filename);

  // Make sure we can parse the XML metafile.
  if (!parser->Parse())
  {
    return false;
  }

  // Make sure the XML file has a root element and it is of the right tag.
  vtkPVXMLElement* rootElement = parser->GetRootElement();
  if (!rootElement || strcmp(rootElement->GetName(), "GmshMetaFile") != 0)
  {
    return false;
  }

  // The file clearly is supposed to be a Gmsh file.
  return true;
}

//----------------------------------------------------------------------------
void vtkGmshMetaReader::DisableAllPointArrays()
{
  this->PointDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
void vtkGmshMetaReader::EnableAllPointArrays()
{
  this->PointDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
int vtkGmshMetaReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkGmshMetaReader::GetPointArrayName(int index)
{
  return this->PointDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkGmshMetaReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkGmshMetaReader::SetPointArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->PointDataArraySelection->EnableArray(name);
  }
  else
  {
    this->PointDataArraySelection->DisableArray(name);
  }
}

//----------------------------------------------------------------------------
void vtkGmshMetaReader::SelectionModifiedCallback(
  vtkObject*, unsigned long, void* clientdata, void*)
{
  static_cast<vtkGmshMetaReader*>(clientdata)->Modified();
}

//----------------------------------------------------------------------------
void vtkGmshMetaReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "TimeStepIndex: " << this->TimeStepIndex << endl;
  os << indent << "TimeStepRange: " << this->TimeStepRange[0] << " " << this->TimeStepRange[1]
     << endl;
  os << indent << "AdaptationLevel: " << this->AdaptationLevel << endl;
  os << indent << "AdaptationTolerance: " << this->AdaptationTolerance << endl;
  this->Reader->PrintSelf(os, indent.GetNextIndent());
}
