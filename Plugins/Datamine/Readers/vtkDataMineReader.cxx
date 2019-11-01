// .NAME DataMineReader.cxx
// Read DataMine binary files for single objects.
// point, perimeter (polyline), wframe<points/triangle>
// With or without properties (each property name < 8 characters)
// The binary file reading is done by 'dmfile.cxx'
//     99-04-12: Written by Jeremy Maccelari, visualn@iafrica.com

#include "vtkDataMineReader.h"
#include "PointMap.h"
#include "PropertyStorage.h"
#include "dmfile.h"

#include "vtkCallbackCommand.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCleanPolyData.h"
#include "vtkDataArraySelection.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkStringArray.h"
#include "vtkTriangleFilter.h"

vtkStandardNewMacro(vtkDataMineReader);

// Constructor
vtkDataMineReader::vtkDataMineReader()
{
  this->FileName = NULL;
  this->PropertyCount = 0;

  this->SetNumberOfInputPorts(0);
  this->CellDataArraySelection = vtkDataArraySelection::New();
  // Setup the selection callback to modify this object when an array
  // selection is changed.
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&vtkDataMineReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->CellDataArraySelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);
}

// --------------------------------------
// Destructor
vtkDataMineReader::~vtkDataMineReader()
{
  this->SetFileName(0);

  // deleting object variables
  if (this->CellDataArraySelection != NULL)
  {
    this->CellDataArraySelection->RemoveObserver(this->SelectionObserver);
    this->CellDataArraySelection->Delete();
  }
  this->SelectionObserver->Delete();
}

// --------------------------------------
int vtkDataMineReader::CanRead(const char* fname, FileTypes type)
{
  // okay we have to un constant the char* so that TDMFile can use it
  // TDMFile does not modify the fname so this is 'okay'
  if (fname == nullptr || fname[0] == '\0' || strcmp(fname, " ") == 0)
  {
    return 0;
  }

  char* tmpName = const_cast<char*>(fname);

  // load the File
  TDMFile* dmFile = new TDMFile();
  dmFile->LoadFileHeader(tmpName);

  // Get File Type
  FileTypes filetype = dmFile->GetFileType();

  // check the type of the file, since we only support wireframes ( points / triangles )
  int result = (filetype == type);

  delete dmFile;
  return result;
}

// --------------------------------------
int vtkDataMineReader::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector*)
{
  // update property list
  this->UpdateDataSelection();
  return 1;
}

// --------------------------------------
int vtkDataMineReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  // Internal Storage Creation
  // we create it here because we only need these datastores
  // while we create the output data, and we cannot trust them between fires of
  // the pipeline
  this->PointMapping = nullptr;
  this->Properties = new PropertyStorage();

  // time to read the point file first, and store it in a vtkPoints object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPolyData* preClean = vtkPolyData::New();
  vtkPoints* points = vtkPoints::New();
  vtkCellArray* cells = vtkCellArray::New();

  this->Read(points, cells);
  preClean->SetPoints(points);

  switch (this->CellMode)
  {
    case VTK_VERTEX:
      preClean->SetVerts(cells);
      break;
    case VTK_LINE:
      preClean->SetLines(cells);
      break;
    case VTK_POLYGON:
      preClean->SetPolys(cells);
      break;
    default:
      preClean->SetVerts(cells);
      break;
  }

  // clean up
  points->Delete();
  cells->Delete();

  this->Properties->PushToDataSet(preClean);

  // delete Internal storage used only for creating the output
  delete this->Properties;
  delete this->PointMapping;
  this->PointMapping = nullptr;

  // clean preclean and copy to output
  this->CleanData(preClean, output);
  preClean->Delete();

  return 1;
}

// --------------------------------------
void vtkDataMineReader::CleanData(vtkPolyData* preClean, vtkPolyData* output)
{
  // duplicate point infest datamine files, so we need to clean it
  // we make the poly cleaner only check perfectly matched points
  vtkCleanPolyData* cleanData = vtkCleanPolyData::New();
  cleanData->SetInputDataObject(preClean);
  cleanData->ToleranceIsAbsoluteOn();   // don't use bounding box fraction
  cleanData->SetAbsoluteTolerance(0.0); // only merge perfect matching points
  cleanData->ConvertLinesToPointsOff();
  cleanData->ConvertPolysToLinesOff();
  cleanData->ConvertStripsToPolysOff();

  vtkTriangleFilter* cleanCells = vtkTriangleFilter::New();
  cleanCells->SetInputConnection(cleanData->GetOutputPort());
  cleanCells->Update();

  // cleanup
  output->ShallowCopy(cleanCells->GetOutput());
  cleanData->Delete();
  cleanCells->Delete();
}

// --------------------------------------
void vtkDataMineReader::ParseProperties(Data* values)
{
  this->Properties->AddValues(values);
}

// --------------------------------------
bool vtkDataMineReader::AddProperty(
  char* varname, const int& pos, const bool& numeric, int numRecords)
{
  int status = this->GetCellArrayStatus(varname);
  this->Properties->AddProperty(varname, numeric, pos, status, numRecords);
  return (status == 0);
}
// --------------------------------------
void vtkDataMineReader::SegmentProperties(const int& records)
{
  // look back at the last n(records) values and segment them
  // have to confirm the property was selected to be segmentable
  this->Properties->Segment(records);
}

// --------------------------------------
void vtkDataMineReader::UpdateDataSelection()
{
  // first step is to grab from the file the header information
  TDMFile* dmFile = new TDMFile();

  // properties are located on the triangles
  if (!dmFile->LoadFileHeader(this->GetFileName()))
  {
    return;
  }

  char* varname = new char[256];
  this->PropertyCount = dmFile->nVars;
  for (int i = 0; i < dmFile->nVars; i++)
  {
    dmFile->Vars[i].GetName(varname);
    if (!this->CellDataArraySelection->ArrayExists(varname))
    {
      this->CellDataArraySelection->AddArray(varname);
      this->CellDataArraySelection->DisableArray(varname);
    }
  }

  delete[] varname;
  delete dmFile;
  this->SetupOutputInformation(this->GetOutputPortInformation(0));
}

// --------------------------------------
void vtkDataMineReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";
}

//----------------------------------------------------------------------------
void vtkDataMineReader::SelectionModifiedCallback(
  vtkObject*, unsigned long, void* clientdata, void*)
{
  static_cast<vtkDataMineReader*>(clientdata)->Modified();
}
//----------------------------------------------------------------------------
int vtkDataMineReader::GetCellArrayStatus(const char* name)
{
  // if 'name' not found, it is treated as 'disabled'
  return (this->CellDataArraySelection->ArrayIsEnabled(name));
}
//----------------------------------------------------------------------------
// Called from within EnableAllArrays()
void vtkDataMineReader::SetCellArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->CellDataArraySelection->EnableArray(name);
  }
  else
  {
    this->CellDataArraySelection->DisableArray(name);
  }
}
//----------------------------------------------------------------------------
// Modified FROM vtkXMLReader.cxx
int vtkDataMineReader::SetFieldDataInfo(vtkDataArraySelection* CellDAS, int association,
  int numTuples, vtkInformationVector*(&infoVector))
{
  if (!CellDAS)
  {
    return 1;
  }

  int i, activeFlag;
  const char* name;

  vtkInformation* info = NULL;

  if (!infoVector)
  {
    infoVector = vtkInformationVector::New();
  }

  /** Part 2 - process data for each array/property **/
  // Cycle through each data array - CellDAS entry.
  activeFlag = 0;
  for (i = 0; i < CellDAS->GetNumberOfArrays(); i++)
  {
    info = vtkInformation::New();

    info->Set(vtkDataObject::FIELD_ASSOCIATION(), association);
    info->Set(vtkDataObject::FIELD_NUMBER_OF_TUPLES(), numTuples);

    name = CellDAS->GetArrayName(i);
    info->Set(vtkDataObject::FIELD_NAME(), name);
    info->Set(vtkDataObject::FIELD_ARRAY_TYPE(), 1);
    info->Set(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS(), 1);

    activeFlag |= 1 << i;
    info->Set(vtkDataObject::FIELD_ACTIVE_ATTRIBUTE(), activeFlag);
    infoVector->Append(info);
    info->Delete();
  }
  return 1;
}
//----------------------------------------------------------------------------
// Modified FROM vtkXMLDataReader.cxx
void vtkDataMineReader::SetupOutputInformation(vtkInformation* outInfo)
{
  // CellDataArraySelection is already prepared.  Don't need SetDataArraySelection()
  vtkInformationVector* infoVector = nullptr;
  // Setup the Field Information for the Cell data
  if (!this->SetFieldDataInfo(this->CellDataArraySelection, vtkDataObject::FIELD_ASSOCIATION_CELLS,
        this->PropertyCount, infoVector))
  {
    vtkErrorMacro("Error return from SetFieldDataInfo().");
    return;
  }
  if (infoVector)
  {
    outInfo->Set(vtkDataObject::CELL_DATA_VECTOR(), infoVector);
    infoVector->Delete();
  }
  else
  {
    vtkErrorMacro("Error infoVector NOT SET IN outInfo.");
  }
}

//----------------------------------------------------------------------------
int vtkDataMineReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkDataMineReader::GetCellArrayName(int index)
{
  return this->CellDataArraySelection->GetArrayName(index);
}
