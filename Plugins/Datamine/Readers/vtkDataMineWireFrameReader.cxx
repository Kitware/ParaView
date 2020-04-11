// .NAME vtkDataMineWireFrameReader.cxx
// Read DataMine binary files for single objects.
// point, perimeter (polyline), wframe<points/triangle>
// With or without properties (each property name < 8 characters)
// The binary file reading is done by 'dmfile.cxx'
//     99-04-12: Written by Jeremy Maccelari, visualn@iafrica.com

#include "vtkDataMineWireFrameReader.h"
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
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"

#include "vtksys/SystemTools.hxx"

#include <sstream>

vtkStandardNewMacro(vtkDataMineWireFrameReader);

// removed the modified declartion so that I can make SetFileName faster
// --------------------------------------
#define vtkSetStringMacroBody(propName, fname)                                                     \
  modified = 0;                                                                                    \
  if (fname == this->propName)                                                                     \
    return;                                                                                        \
  if (fname && this->propName && !strcmp(fname, this->propName))                                   \
    return;                                                                                        \
  modified = 1;                                                                                    \
  if (this->propName)                                                                              \
    delete[] this->propName;                                                                       \
  if (fname)                                                                                       \
  {                                                                                                \
    size_t fnl = strlen(fname) + 1;                                                                \
    char* dst = new char[fnl];                                                                     \
    const char* src = fname;                                                                       \
    this->propName = dst;                                                                          \
    do                                                                                             \
    {                                                                                              \
      *dst++ = *src++;                                                                             \
    } while (--fnl);                                                                               \
  }                                                                                                \
  else                                                                                             \
  {                                                                                                \
    this->propName = 0;                                                                            \
  }

// Constructor
vtkDataMineWireFrameReader::vtkDataMineWireFrameReader()
{
  this->PointFileName = NULL;
  this->TopoFileName = NULL;
  this->StopeSummaryFileName = NULL;
  this->StopeFileMap = NULL;
  this->UseStopeSummary = false;

  this->PropertyCount = -1;
  this->CellMode = VTK_POLYGON;
}

// --------------------------------------
// Destructor
vtkDataMineWireFrameReader::~vtkDataMineWireFrameReader()
{
  delete[] this->PointFileName;
  delete[] this->TopoFileName;
  delete[] this->StopeSummaryFileName;
}

// --------------------------------------
void vtkDataMineWireFrameReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// --------------------------------------
int vtkDataMineWireFrameReader::CanReadFile(const char* fname)
{
  return (this->CanRead(fname, wframepoints) || this->CanRead(fname, wframetriangle) ||
    this->CanRead(fname, stopesummary));
}

// --------------------------------------
int vtkDataMineWireFrameReader::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector*)
{
  // save on 2 calls by pre calling
  int t = this->TopoFileBad();
  int p = this->PointFileBad();
  int s = this->StopeFileBad();

  std::string ext;
  if (t)
  {
    // guess Topo File
    ext = "tr";
    this->FindAndSetFilePath(ext, false, wframetriangle);
  }
  if (p)
  {
    // guess Point File
    ext = "pt";
    this->FindAndSetFilePath(ext, false, wframepoints);
  }
  if (s)
  {
    // we have a stop access file, have to guess the other two
    ext = "sp";
    this->FindAndSetFilePath(ext, false, stopesummary);
  }

  return 1;
}
// --------------------------------------
int vtkDataMineWireFrameReader::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (this->TopoFileBad() || this->PointFileBad())
  {
    return 1;
  }

  this->StopeFileMap = nullptr;
  if (this->UseStopeSummary)
  {
    bool created = this->PopulateStopeMap(); // fill
    if (!created)
    {
      this->UseStopeSummary = false;
      vtkWarningMacro(
        << "Failed to find the Stope Column in the Stope Summary File, ignoring the file");
    }
  }

  vtkDataMineReader::RequestData(request, inputVector, outputVector);

  delete this->StopeFileMap;

  return 1;
}

// --------------------------------------
bool vtkDataMineWireFrameReader::PopulateStopeMap()
{

  TDMFile* file = new TDMFile();
  file->LoadFileHeader(this->GetStopeSummaryFileName());

  // find the Stope Porperty
  int SID = -1;
  char* varname = new char[2048]; // make it really large so we don't run the bounds
  for (int i = 0; i < file->nVars; i++)
  {
    file->Vars[i].GetName(varname);
    if (strncmp(varname, "STOPE", 5) == 0)
    {
      SID = i;
      break;
    }
  }

  // cleanup
  delete[] varname;

  if (SID == -1)
  {
    return 0;
  }

  // populate the map

  int numRecords = file->GetNumberOfRecords();
  this->StopeFileMap = new PointMap(numRecords);

  Data* values = new Data[file->nVars];
  file->OpenRecVarFile(this->GetStopeSummaryFileName());
  for (int i = 0; i < numRecords; i++)
  {
    file->GetRecVars(i, values);
    this->StopeFileMap->SetID(values[SID].v, i);
  }
  file->CloseRecVarFile();

  // cleanup
  delete[] values; // this is causing the problems
  delete file;

  return 1;
}

// --------------------------------------
void vtkDataMineWireFrameReader::Read(vtkPoints* points, vtkCellArray* cells)
{
  this->ReadPoints(points);
  this->ReadCells(cells);
}

// --------------------------------------
void vtkDataMineWireFrameReader::ReadPoints(vtkPoints* points)
{
  TDMFile* file = new TDMFile();
  file->LoadFileHeader(this->GetPointFileName());

  // need to create the point lookup index, for id,xp,yp,zp
  // since the binary file will have these fields, but the order of
  // them is not known
  int ID, X, Y, Z;
  char* varname = new char[256]; // make it really large so we don't run the bounds
  for (int i = 0; i < file->nVars; i++)
  {
    file->Vars[i].GetName(varname);
    if (strncmp(varname, "XP", 2) == 0)
    {
      X = i;
    }
    else if (strncmp(varname, "YP", 2) == 0)
    {
      Y = i;
    }
    else if (strncmp(varname, "ZP", 2) == 0)
    {
      Z = i;
    }
    else if (strncmp(varname, "PID", 3) == 0)
    {
      ID = i;
    }
    else
    {
      // hit a point property!
    }
  }
  delete[] varname;

  this->ParsePoints(points, file, ID, X, Y, Z);

  // cleanup
  delete file;
}

// --------------------------------------
void vtkDataMineWireFrameReader::ParsePoints(
  vtkPoints* points, TDMFile* file, const int& PID, const int& XID, const int& YID, const int& ZID)
{
  int numRecords = file->GetNumberOfRecords();
  this->PointMapping = new PointMap(numRecords);

  Data* values = new Data[file->nVars];
  file->OpenRecVarFile(this->GetPointFileName());
  for (int i = 0; i < numRecords; i++)
  {
    file->GetRecVars(i, values);
    this->PointMapping->SetID(values[PID].v, i);
    points->InsertPoint(i, values[XID].v, values[YID].v, values[ZID].v);
  }
  file->CloseRecVarFile();
  delete[] values; // this is causing the problems
}

// --------------------------------------
void vtkDataMineWireFrameReader::ReadCells(vtkCellArray* cells)
{
  TDMFile* file = new TDMFile();
  file->LoadFileHeader(this->GetTopoFileName());
  int numRecords = file->GetNumberOfRecords();
  // need to create lookup for point id's 1,2,3
  // since the binary file will have these fields, but the order of
  // them is not known
  int P1 = -1, P2 = -1, P3 = -1, Stope = -1;

  char* varname = new char[2048]; // make it really large so we don't run the bounds
  for (int i = 0; i < file->nVars; i++)
  {
    file->Vars[i].GetName(varname);
    if (strncmp(varname, "PID1", 4) == 0)
    {
      P1 = i;
    }
    else if (strncmp(varname, "PID2", 4) == 0)
    {
      P2 = i;
    }
    else if (strncmp(varname, "PID3", 4) == 0)
    {
      P3 = i;
    }
    else if (strncmp(varname, "STOPE", 5) == 0)
    {
      Stope = i;
    }

    this->AddProperty(varname, i, file->Vars[i].TypeIsNumerical(), numRecords);
  }

  // hackish way of adding stope properties to a cell
  // means we call a special ParseCells method when we have stopes
  if (this->UseStopeSummary)
  {
    TDMFile* stopeFile = new TDMFile();
    stopeFile->LoadFileHeader(this->GetStopeSummaryFileName());
    numRecords = stopeFile->GetNumberOfRecords();

    for (int j = 0; j < stopeFile->nVars; j++)
    {
      stopeFile->Vars[j].GetName(varname);
      this->AddProperty(varname, file->nVars + j, stopeFile->Vars[j].TypeIsNumerical(), numRecords);
    }

    this->ParseCellsWithStopes(cells, file, stopeFile, P1, P2, P3, Stope);

    // cleanup
    delete stopeFile;
  }
  else
  {
    // read the file triangles and properties
    this->ParseCells(cells, file, P1, P2, P3);
  }

  // cleanup
  delete[] varname;
  delete file;
}

// --------------------------------------
void vtkDataMineWireFrameReader::ParseCells(
  vtkCellArray* cells, TDMFile* file, const int& P1, const int& P2, const int& P3)
{
  int triangle[3]; // could not figure out a good name, so used an array
  Data* values = new Data[file->nVars];
  file->OpenRecVarFile(this->GetTopoFileName());
  int numRecords = file->GetNumberOfRecords();
  cells->Allocate(numRecords * 4);

  for (int i = 0; i < numRecords; i++)
  {

    file->GetRecVars(i, values);

    // need to detect the use case of the point table does not have the cell
    triangle[0] = this->PointMapping->GetID(values[P1].v);
    triangle[1] = this->PointMapping->GetID(values[P2].v);
    triangle[2] = this->PointMapping->GetID(values[P3].v);

    //-1 means a bad point so do not draw the triangle
    if (triangle[0] > -1 && triangle[1] > -1 && triangle[2] > -1)
    {
      // size is 3 because we only have triangles
      cells->InsertNextCell(3);
      cells->InsertCellPoint(triangle[0]);
      cells->InsertCellPoint(triangle[1]);
      cells->InsertCellPoint(triangle[2]);

      this->ParseProperties(values);
    }
  }
  file->CloseRecVarFile();
  delete[] values;
}

// --------------------------------------
void vtkDataMineWireFrameReader::ParseCellsWithStopes(vtkCellArray* cells, TDMFile* file,
  TDMFile* stopeFile, const int& P1, const int& P2, const int& P3, const int& stopeId)
{
  int numRecords = file->GetNumberOfRecords();
  Data* values = new Data[file->nVars + stopeFile->nVars];
  Data* stopeValues = &values[file->nVars];

  int triangle[3]; // could not figure out a good name, so used an array
  int previousStopeId = -1;
  int currentStopeId = 0;
  int cellsInStope = 0;
  int pos = 0;
  int oldPos = -1;
  int stopeIndex = 0;

  file->OpenRecVarFile(this->GetTopoFileName());
  stopeFile->OpenRecVarFile(this->GetStopeSummaryFileName());
  for (int i = 0; i < numRecords; i++)
  {
    file->GetRecVars(i, values);

    // ask the mapping for the correct index

    stopeIndex = static_cast<int>(values[stopeId].v);
    pos = this->StopeFileMap->GetID(stopeIndex);

    /*
    this is a neat little trick
    We don't delete values, so if we ask for the same stope summary index numerous times in a row
    we just reuse the Data* since it is already full of the right values
    */
    if (pos != oldPos)
    {
      stopeFile->GetRecVars(pos, stopeValues);
      pos = oldPos;
    }

    // need to detect the use case of the point table does not have the cell
    triangle[0] = this->PointMapping->GetID(values[P1].v);
    triangle[1] = this->PointMapping->GetID(values[P2].v);
    triangle[2] = this->PointMapping->GetID(values[P3].v);

    //-1 means a bad point so do not draw the triangle
    if (triangle[0] > -1 && triangle[1] > -1 && triangle[2] > -1)
    {
      // size is 3 because we only have triangles
      cells->InsertNextCell(3);
      cells->InsertCellPoint(triangle[0]);
      cells->InsertCellPoint(triangle[1]);
      cells->InsertCellPoint(triangle[2]);

      this->ParseProperties(values);
    }

    // code added to support segmentable properties
    // so track how many cells we are reading
    currentStopeId = static_cast<int>(values[stopeId].v);
    cellsInStope++;
    if (previousStopeId != currentStopeId)
    {
      if (previousStopeId > -1)
      {
        // update the property list with the new segemented data
        this->SegmentProperties(cellsInStope);
      }

      // reset the stope
      previousStopeId = currentStopeId;
      cellsInStope = 1;
    }
  }
  // do the last stope
  this->SegmentProperties(cellsInStope);

  file->CloseRecVarFile();
  stopeFile->CloseRecVarFile();

  delete[] values;
}
// --------------------------------------
bool vtkDataMineWireFrameReader::FindAndSetFilePath(
  std::string& dmExt, const bool& update, FileTypes type)
{
  std::string path(this->FileName);
  std::string baseName, baseExt;

  // default guess is the way datamine guess
  // which is namept.dm and nametr.dm ( that is why it is dot - 2 )
  const auto dot = path.rfind(".");
  baseName = path.substr(0, (dot - 2));
  baseExt = path.substr(dot, path.size());

  std::string dm(baseName + dmExt + baseExt);
  if (vtksys::SystemTools::FileExists(dm))
  {
    this->SetFileName(dm.c_str(), update, type);
    return 1;
  }
  return 0;
}

// --------------------------------------
void vtkDataMineWireFrameReader::SetFileName(
  const char* filename, const bool& update, FileTypes filetype)
{
  int modified = 0;
  if (update)
  {
    vtkSetStringMacroBody(FileName, filename);
  }
  if (update && modified)
  {
    // load the File
    TDMFile* dmFile = new TDMFile();
    dmFile->LoadFileHeader(this->FileName);

    // Get File Type
    filetype = dmFile->GetFileType();
    delete dmFile;
  }

  // check the type of the file, since we only support wireframes ( points / triangles )
  if (filetype == wframepoints)
  {
    this->SetPointFileName(filename);
  }
  else if (filetype == wframetriangle)
  {
    this->SetTopoFileName(filename);
  }
  else if (filetype == stopesummary)
  {
    this->SetStopeSummaryFileName(filename);
  }

  this->Modified();
}

// --------------------------------------
void vtkDataMineWireFrameReader::SetTopoFileName(const char* filename)
{
  int modified = 0;
  vtkSetStringMacroBody(TopoFileName, filename);
  if (modified)
  {
    // than we have to update the list with the new options
    this->UpdateDataSelection();
    this->Modified();
  }
}

// --------------------------------------
void vtkDataMineWireFrameReader::SetStopeSummaryFileName(const char* filename)
{
  int modified = 0;
  vtkSetStringMacroBody(StopeSummaryFileName, filename);
  if (modified)
  {
    // than we have to update the list with the new options
    this->UseStopeSummary = true;
    this->UpdateDataSelection();
    this->Modified();
  }
}

// --------------------------------------
int vtkDataMineWireFrameReader::PointFileBad()
{
  return !(this->CanRead(this->PointFileName, wframepoints));
}
// --------------------------------------
int vtkDataMineWireFrameReader::TopoFileBad()
{
  return !(this->CanRead(this->TopoFileName, wframetriangle));
}

// --------------------------------------
int vtkDataMineWireFrameReader::StopeFileBad()
{
  return !(this->CanRead(this->StopeSummaryFileName, stopesummary));
}

// --------------------------------------
void vtkDataMineWireFrameReader::UpdateDataSelection()
{
  if (this->TopoFileBad())
  {
    return;
  }
  // while this is not the most efficent way to add data it works
  // for all use cases ( new data, state files, changing topo files )
  // you can make this better by coding a specific algorithm for each use case
  // but that just makes the code nastier to understand
  // the upside of this way is that properties check state is mantained across topography files

  // copy the current selection set
  vtkDataArraySelection* oldSelections = vtkDataArraySelection::New();
  oldSelections->CopySelections(this->CellDataArraySelection);

  // because PropertyCount is not stored in the xml file, it will always be -1
  // the first time you load a file, or load a state with a datamine file in it.
  if (this->PropertyCount > -1)
  {
    this->CellDataArraySelection->RemoveAllArrays();
  }

  TDMFile* dmTopo = new TDMFile();

  if (dmTopo->LoadFileHeader(this->TopoFileName))
  {
    this->SetupDataSelection(dmTopo, oldSelections);
  }

  this->PropertyCount = dmTopo->nVars;
  delete dmTopo;

  // tack on all information in the stope summary file
  // a method should be made to reduce the code duplication
  if (this->UseStopeSummary)
  {
    TDMFile* dmStopes = new TDMFile();
    if (dmStopes->LoadFileHeader(this->StopeSummaryFileName))
    {
      this->SetupDataSelection(dmStopes, oldSelections);
    }
    this->PropertyCount += dmStopes->nVars;
    delete dmStopes;
  }

  // cleanup
  oldSelections->Delete();

  this->SetupOutputInformation(this->GetOutputPortInformation(0));
}

// --------------------------------------
void vtkDataMineWireFrameReader::SetupDataSelection(TDMFile* dmFile, vtkDataArraySelection* old)
{
  char* varname = new char[2048];
  for (int i = 0; i < dmFile->nVars; i++)
  {
    dmFile->Vars[i].GetName(varname);
    this->CellDataArraySelection->AddArray(varname);

    if (old->ArrayExists(varname))
    {
      this->SetCellArrayStatus(varname, old->ArrayIsEnabled(varname));
    }
    else
    {
      this->SetCellArrayStatus(varname, 0);
    }
  }

  delete[] varname;
}

// --------------------------------------
void vtkDataMineWireFrameReader::SetCellArrayStatus(const char* name, int status)
{
  switch (status)
  {
    case 2:
    case 1:
      this->CellDataArraySelection->EnableArray(name);
      break;
    case 0:
    default:
      this->CellDataArraySelection->DisableArray(name);
      break;
  }
}
