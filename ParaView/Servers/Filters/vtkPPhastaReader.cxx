/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPPhastaReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPPhastaReader.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkPhastaReader.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include <vtksys/SystemTools.hxx>

#include <vtkstd/map>

struct vtkPPhastaReaderInternal
{
  struct TimeStepInfo
  {
    int GeomIndex;
    int FieldIndex;
    double TimeValue;

    TimeStepInfo() : GeomIndex(-1), FieldIndex(-1), TimeValue(0.0)
      {
      }
  };

  typedef vtkstd::map<int, TimeStepInfo> TimeStepInfoMapType;
  TimeStepInfoMapType TimeStepInfoMap;
};

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkPPhastaReader, "1.1");
vtkStandardNewMacro(vtkPPhastaReader);

//----------------------------------------------------------------------------
vtkPPhastaReader::vtkPPhastaReader()
{
  this->FileName = 0;

  this->TimeStepIndex = 0;

  this->Reader = vtkPhastaReader::New();

  this->SetNumberOfInputPorts(0);

  this->Parser = 0;

  this->Internal = new vtkPPhastaReaderInternal;

  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;
}

//----------------------------------------------------------------------------
vtkPPhastaReader::~vtkPPhastaReader()
{
  this->Reader->Delete();
  this->SetFileName(0);

  if (this->Parser)
    {
    this->Parser->Delete();
    }

  delete this->Internal;
}

//----------------------------------------------------------------------------
int vtkPPhastaReader::RequestData(vtkInformation*,
                                vtkInformationVector**,
                                vtkInformationVector* outputVector)
{
  // get the data object
  vtkInformation *outInfo = 
    outputVector->GetInformationObject(0);

  if (this->TimeStepIndex > this->TimeStepRange[1])
    {
    vtkErrorMacro("Timestep index too large.");
    return 0;
    }

  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // get the current piece being requested
  int piece = 
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());

  vtkDebugMacro( << "Current piece: " << piece );

  if (!this->Parser)
    {
    vtkErrorMacro("No parser was created. Cannot read file");
    return 0;
    }

  vtkPVXMLElement* rootElement = this->Parser->GetRootElement();
  
  int numPieces;
  if (!rootElement->GetScalarAttribute("number_of_pieces", &numPieces))
    {
    numPieces = 1;
    }

  // More processors than pieces. Return with empty output.
  if (piece >= numPieces)
    {
    return 1;
    }

  const char* geometryPattern = 0;
  int geomHasPiece, geomHasTime;
  const char* fieldPattern = 0;
  int fieldHasPiece, fieldHasTime;

  unsigned int numElements = rootElement->GetNumberOfNestedElements();
  for (unsigned int i=0; i<numElements; i++)
    {
    vtkPVXMLElement* nested = rootElement->GetNestedElement(i);

    if (strcmp("GeometryFileNamePattern", nested->GetName()) == 0)
      {
      geometryPattern = nested->GetAttribute("pattern");
      if (!nested->GetScalarAttribute("has_piece_entry", &geomHasPiece))
        {
        geomHasPiece = 0;
        }
      if (!nested->GetScalarAttribute("has_time_entry", &geomHasTime))
        {
        geomHasTime = 0;
        }
      }

    if (strcmp("FieldFileNamePattern", nested->GetName()) == 0)
      {
      fieldPattern = nested->GetAttribute("pattern");
      if (!nested->GetScalarAttribute("has_piece_entry", &fieldHasPiece))
        {
        fieldHasPiece = 0;
        }
      if (!nested->GetScalarAttribute("has_time_entry", &fieldHasTime))
        {
        fieldHasTime = 0;
        }
      }
    }

  if (!geometryPattern)
    {
    vtkErrorMacro("No geometry pattern was specified. Cannot load file");
    return 0;
    }

  if (!fieldPattern)
    {
    vtkErrorMacro("No field pattern was specified. Cannot load file");
    return 0;
    }

  char* geom_name = new char [ strlen(geometryPattern) + 60 ];
  char* field_name = new char [ strlen(fieldPattern) + 60 ];

  if (geomHasTime && geomHasPiece)
    {
    sprintf(geom_name, 
            geometryPattern, 
            this->Internal->TimeStepInfoMap[this->TimeStepIndex].GeomIndex, 
            piece+1);
    }
  else if (geomHasPiece)
    {
    sprintf(geom_name, geometryPattern, piece+1);
    }
  else if (geomHasTime)
    {
    sprintf(geom_name, 
            geometryPattern, 
            this->Internal->TimeStepInfoMap[this->TimeStepIndex].GeomIndex);
    }
  else
    {
    strcpy(geom_name, geometryPattern);
    }

  if (fieldHasTime && fieldHasPiece)
    {
    sprintf(field_name, 
            fieldPattern, 
            this->Internal->TimeStepInfoMap[this->TimeStepIndex].FieldIndex,
            piece+1);
    }
  else if (fieldHasPiece)
    {
    sprintf(field_name, fieldPattern, piece+1);
    }
  else if (fieldHasTime)
    {
    sprintf(field_name, 
            fieldPattern, 
            this->Internal->TimeStepInfoMap[this->TimeStepIndex].FieldIndex);
    }
  else
    {
    strcpy(geom_name, fieldPattern);
    }

  ostrstream geomFName;
  vtkstd::string gpath = vtksys::SystemTools::GetFilenamePath(geom_name);
  if (gpath.empty() || !vtksys::SystemTools::FileIsFullPath(gpath.c_str()))
    {
    vtkstd::string path = vtksys::SystemTools::GetFilenamePath(this->FileName);
    if (!path.empty())
      {
      geomFName << path.c_str() << "/";
      }
    }
  geomFName << geom_name << ends;
  this->Reader->SetGeometryFileName(geomFName.str());
  delete[] geomFName.str();

  ostrstream fieldFName;
  vtkstd::string fpath = vtksys::SystemTools::GetFilenamePath(field_name);
  if (fpath.empty() || !vtksys::SystemTools::FileIsFullPath(fpath.c_str()))
    {
    vtkstd::string path = vtksys::SystemTools::GetFilenamePath(this->FileName);
    if (!path.empty())
      {
      fieldFName << path.c_str() << "/";
      }
    }
  fieldFName << field_name << ends;
  this->Reader->SetFieldFileName(fieldFName.str());
  delete[] fieldFName.str();

  this->Reader->Update();

  output->ShallowCopy(this->Reader->GetOutput());

  delete [] geom_name;
  delete [] field_name;

  this->Parser->Delete();
  this->Parser = 0;

  return 1;
}

//----------------------------------------------------------------------------
int vtkPPhastaReader::RequestInformation(vtkInformation*, 
                                       vtkInformationVector**, 
                                       vtkInformationVector* outputVector)
{ 
  this->Internal->TimeStepInfoMap.clear();

  vtkInformation *outInfo = 
    outputVector->GetInformationObject(0);

  if (!this->FileName)
    {
    vtkErrorMacro("FileName has to be specified.");
    return 0;
    }

  if (this->Parser)
    {
    vtkWarningMacro("No parser should exist.");
    this->Parser->Delete();
    this->Parser = 0;
    }
  
  vtkSmartPointer<vtkPVXMLParser> parser = 
    vtkSmartPointer<vtkPVXMLParser>::New();

  parser->SetFileName(this->FileName);
  if (!parser->Parse())
    {
    return 0;
    }

  vtkPVXMLElement* rootElement = parser->GetRootElement();
  if (!rootElement)
    {
    vtkErrorMacro("Cannot parse file.");
    return 0;
    }

  if (strcmp(rootElement->GetName(), "PhastaMetaFile") != 0)
    {
    vtkErrorMacro("This is not a phasta metafile.");
    return 0;
    }

  this->Parser = parser;
  parser->Register(this);

  int numTimeSteps=1;
  int hasTimeValues = 0;

  unsigned int numElements = rootElement->GetNumberOfNestedElements();
  for (unsigned int i=0; i<numElements; i++)
    {
    vtkPVXMLElement* nested = rootElement->GetNestedElement(i);
    if (strcmp("TimeSteps", nested->GetName()) == 0)
      {
      if (!nested->GetScalarAttribute("number_of_steps", &numTimeSteps))
        {
        numTimeSteps = 1;
        }
      int autoGen;
      int indexIncr;
      int startIndex;
      if (!nested->GetScalarAttribute("auto_generate_indices", &autoGen))
        {
        autoGen = 0;
        }
      if (!nested->GetScalarAttribute("increment_index_by", &indexIncr))
        {
        indexIncr = 1;
        }
      if (!nested->GetScalarAttribute("start_index", &startIndex))
        {
        startIndex = 0;
        }
      if (autoGen)
        {
        for (int j=0; j<numTimeSteps; j++)
          {
          vtkPPhastaReaderInternal::TimeStepInfo& info = 
            this->Internal->TimeStepInfoMap[j];
          info.GeomIndex = startIndex;
          info.FieldIndex = startIndex;
          startIndex += indexIncr;
          }
        }

      unsigned int numElements2 = nested->GetNumberOfNestedElements();
      for (unsigned int j=0; j<numElements2; j++)
        {
        vtkPVXMLElement* nested2 = nested->GetNestedElement(j);
        if (strcmp("TimeStep", nested2->GetName()) == 0)
          {
          int index;
          if (nested2->GetScalarAttribute("index", &index))
            {
            if ( (index+1) > numTimeSteps )
              {
              numTimeSteps = index+1;
              }
            vtkPPhastaReaderInternal::TimeStepInfo& info =
              this->Internal->TimeStepInfoMap[index];
            int gIdx;
            if (nested2->GetScalarAttribute("geometry_index", 
                                             &gIdx))
              {
              info.GeomIndex = gIdx;
              }
            int fIdx;
            if (nested2->GetScalarAttribute("field_index", 
                                             &fIdx))
              {
              info.FieldIndex = fIdx;
              }
            double val;
            if (nested2->GetScalarAttribute("value", 
                                            &val))
              {
              info.TimeValue = val;
              hasTimeValues = 1;
              }
            }
          }
        }
      break;
      }
    }
  

  int tidx;
  // Make sure all indices are there
  for (tidx=1; tidx<numTimeSteps; tidx++)
    {
    vtkPPhastaReaderInternal::TimeStepInfoMapType::iterator iter =
      this->Internal->TimeStepInfoMap.find(tidx);
    if (iter == this->Internal->TimeStepInfoMap.end())
      {
      vtkErrorMacro("Missing timestep, index=" << tidx);
      return 0;
      }
    }

  if (hasTimeValues)
    {
    double* timeSteps = new double[numTimeSteps];
    for (tidx=0; tidx<numTimeSteps; tidx++)
      {
      timeSteps[tidx] = this->Internal->TimeStepInfoMap[tidx].TimeValue;
      }
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), 
                 timeSteps, 
                 numTimeSteps);
    delete[] timeSteps;
    }

  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = numTimeSteps-1;

  vtkInformation* info = outputVector->GetInformationObject(0);
  info->Set(
    vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);

  return 1;
}

//----------------------------------------------------------------------------
void vtkPPhastaReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FileName: " 
     << (this->FileName?this->FileName:"(none)")
     << endl;
  os << indent << "TimeStepIndex: " << this->TimeStepIndex << endl;
  os << indent << "TimeStepRange: " 
     << this->TimeStepRange[0] << " " << this->TimeStepRange[1]
     << endl;
}

