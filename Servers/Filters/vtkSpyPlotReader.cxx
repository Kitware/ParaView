/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkSpyPlotReader.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSpyPlotReader.h"

#include "vtkObjectFactory.h"
#include "vtkHierarchicalDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkCellData.h"
#include "vtkDataArraySelection.h"
#include "vtkCallbackCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkMultiProcessController.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkMultiGroupDataInformation.h"
#include "vtkUniformGrid.h"
#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkFloatArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkByteSwap.h"

#include "vtkExtractCTHPart.h" // for the BOUNDS key

#include <vtkstd/map>
#include <vtkstd/set>
#include <vtkstd/vector>
#include <vtkstd/string>
#include <sys/stat.h>
#include <vtksys/SystemTools.hxx>
#include <assert.h>

#define vtkMIN(x, y) \
  (\
   ((x)<(y))?(x):(y)\
   )

#define coutVector6(x) (x)[0] << " " << (x)[1] << " " << (x)[2] << " " << (x)[3] << " " << (x)[4] << " " << (x)[5]
#define coutVector3(x) (x)[0] << " " << (x)[1] << " " << (x)[2]

vtkCxxRevisionMacro(vtkSpyPlotReader, "1.41");
vtkStandardNewMacro(vtkSpyPlotReader);
vtkCxxSetObjectMacro(vtkSpyPlotReader,Controller,vtkMultiProcessController);

#define READ_SPCTH_VOLUME_FRACTION "Material volume fraction"

class vtkSpyPlotWriteString
{
public:
  vtkSpyPlotWriteString(const char* data, size_t length) : Data(data), Length(length) {}

  const char* Data;
  size_t Length;
};

inline ostream& operator<< (ostream& os, const vtkSpyPlotWriteString& c)
{
  os.write(c.Data, c.Length);
  os.flush();
  return os;
}

//-----------------------------------------------------------------------------
//=============================================================================
class vtkSpyPlotUniReader : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkSpyPlotUniReader, vtkObject);
  static vtkSpyPlotUniReader* New();

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  virtual void SetCellArraySelection(vtkDataArraySelection* da);

  int ReadInformation();
  int ReadData();
  void PrintInformation();
  void PrintMemoryUsage();

  int SetCurrentTime(double time);
  int SetCurrentTimeStep(int timeStep);

  vtkGetMacro(CurrentTime, double);
  vtkGetMacro(CurrentTimeStep, int);

  vtkGetVector2Macro(TimeStepRange, int);
  vtkGetVector2Macro(TimeRange, double);

  int GetTimeStepFromTime(double time);
  double GetTimeFromTimeStep(int timeStep);

  vtkGetMacro(NumberOfCellFields, int);

  double* GetTimeArray() { return this->DumpTime; }

  const char* GetCellFieldDescription(int field);

  int IsAMR() { return this->GetNumberOfDataBlocks() > 1; }
  int GetNumberOfDataBlocks();
  int GetDataBlockLevel(int block);
  int GetDataBlockDimensions(int block, int* dims, int* fixed);
  int GetDataBlockBounds(int block, double* bounds, int *fixed);
  int GetDataBlockVectors(int block, vtkDataArray** coordinates, int* fixed);
  int MarkVectorsAsFixed(int block);
  const char* GetCellFieldName(int field);
  vtkDataArray* GetCellFieldData(int block, int field, int* fixed);
  int MarkCellFieldDataFixed(int block, int field);

  struct CellMaterialField
  {
    char Id[30];
    char Comment[80];
    int Index;
  };

  struct Variable
  {
    char* Name;
    int Material;
    int Index;
    CellMaterialField* MaterialField;
    vtkDataArray** DataBlocks;
    int *GhostCellsFixed;
  };
  struct Block
  {
    int Nx;
    int Ny;
    int Nz;
    int Allocated;
    int Active;
    int Level;

    vtkFloatArray* XArray;
    vtkFloatArray* YArray;
    vtkFloatArray* ZArray;
    int VectorsFixedForGhostCells;
    int RemovedBadGhostCells[6];
  };
  struct DataDump
  {
    int NumVars;
    int* SavedVariables;
    vtkTypeInt64* SavedVariableOffsets;
    Variable *Variables;
    int NumberOfBlocks;
    int ActualNumberOfBlocks;
    Block* Blocks;
  };

  Block* GetDataBlock(int block);

  vtkSetMacro(DataTypeChanged, int);
  void SetDownConvertVolumeFraction(int vf);

protected:
  vtkSpyPlotUniReader();
  ~vtkSpyPlotUniReader();

private:
  int ReadInt(istream &ifs, int* val, int num);
  int ReadDouble(istream &ifs, double* val, int num);
  int ReadFileOffset(istream &ifs, vtkTypeInt64* val, int num);
  int ReadString(istream &ifs, char* str, size_t len);
  int ReadString(istream &ifs, unsigned char* str, size_t len);
  int RunLengthDeltaDecode(const unsigned char* in, int inSize, float* out, int outSize);
  int RunLengthDataDecode(const unsigned char* in, int inSize, float* out, int outSize);
  int RunLengthDataDecode(const unsigned char* in, int inSize, unsigned char* out, int outSize);


  int ReadDataArray(int block, int field, vtkFloatArray* da);
  int ReadDataArray(int block, int field, vtkUnsignedCharArray* da);


  // Header information
  char FileDescription[128];
  int FileVersion;
  int SizeOfFilePointer;
  int FileCompressionFlag;
  int FileProcessorId;
  int NumberOfProcessors;
  int IGM;
  int NumberOfDimensions;
  int NumberOfMaterials;
  int MaximumNumberOfMaterials;
  double GlobalMin[3];
  double GlobalMax[3];
  int NumberOfBlocks;
  int MaximumNumberOfLevels;

  // For storing possible cell/material fields meta data
  int NumberOfPossibleCellFields;
  CellMaterialField* CellFields;
  int NumberOfPossibleMaterialFields;
  CellMaterialField* MaterialFields;

  // Individual dump information
  int NumberOfDataDumps;
  int *DumpCycle;
  double *DumpTime;
  double *DumpDT; // SPCTH 102 (What is this anyway?)
  vtkTypeInt64 *DumpOffset;

  DataDump* DataDumps;


  // File name
  char* FileName;

  // Was information read
  int HaveInformation;

  // Current time and time range information
  int CurrentTimeStep;
  double CurrentTime;
  int TimeStepRange[2];
  double TimeRange[2];

  int DataTypeChanged;
  int DownConvertVolumeFraction;

  int NumberOfCellFields;

  vtkDataArraySelection* CellArraySelection;

  Variable* GetCellField(int field);
  int IsVolumeFraction(Variable* var);

  istream& Seek(istream* stream, vtkTypeInt64 offset, bool rel = false);
  vtkTypeInt64 Tell(istream* stream);
private:
  vtkSpyPlotUniReader(const vtkSpyPlotUniReader&); // Not implemented
  void operator=(const vtkSpyPlotUniReader&); // Not implemented
};
//=============================================================================
//-----------------------------------------------------------------------------

vtkCxxRevisionMacro(vtkSpyPlotUniReader, "1.41");
vtkStandardNewMacro(vtkSpyPlotUniReader);
vtkCxxSetObjectMacro(vtkSpyPlotUniReader, CellArraySelection, vtkDataArraySelection);

//-----------------------------------------------------------------------------
vtkSpyPlotUniReader::vtkSpyPlotUniReader()
{
  this->FileName = 0;
  this->FileVersion = 0;
  this->SizeOfFilePointer = 32;
  this->FileCompressionFlag = 0;
  this->FileProcessorId = 0;
  this->NumberOfProcessors = 0;
  this->IGM = 0;
  this->NumberOfDimensions = 0;
  this->NumberOfMaterials = 0;
  this->MaximumNumberOfMaterials = 0;
  this->NumberOfBlocks = 0;
  this->MaximumNumberOfLevels = 0;
  this->NumberOfPossibleCellFields = 0;
  this->NumberOfPossibleMaterialFields = 0;

  this->CellFields = 0;
  this->MaterialFields = 0;

  this->NumberOfDataDumps = 0;
  this->DumpCycle  = 0;
  this->DumpTime   = 0;
  this->DumpDT     = 0;
  this->DumpOffset = 0;

  this->DataDumps = 0;


  this->CellArraySelection = 0;

  this->TimeStepRange[0] = this->TimeStepRange[1] = 0;
  this->TimeRange[0] = this->TimeRange[1] = 0.0;
  this->CurrentTimeStep = 0;
  this->CurrentTime = 0.0;

  this->NumberOfCellFields = 0;
  this->HaveInformation = 0;
  this->DownConvertVolumeFraction = 1;
  this->DataTypeChanged = 0;
  if ( !this->HaveInformation ) { vtkDebugMacro( << __LINE__ << " " << this << " Read: " << this->HaveInformation ); }
}

//-----------------------------------------------------------------------------
vtkSpyPlotUniReader::~vtkSpyPlotUniReader()
{
  // Cleanup header
  delete [] this->CellFields;
  delete [] this->MaterialFields;
  delete [] this->DumpCycle;
  delete [] this->DumpTime;
  delete [] this->DumpDT;
  delete [] this->DumpOffset;

  int dump;
  for ( dump = 0; dump < this->NumberOfDataDumps; ++ dump )
    {
    vtkSpyPlotUniReader::DataDump* dp = this->DataDumps+dump;
    delete [] dp->SavedVariables;
    delete [] dp->SavedVariableOffsets;
    int var;
    for ( var = 0; var < dp->NumVars; ++ var)
      {
      vtkSpyPlotUniReader::Variable *cv = dp->Variables + var;
      delete [] cv->Name;
      if ( cv->DataBlocks )
        {
        int ca;
        for ( ca = 0; ca < dp->ActualNumberOfBlocks; ++ ca )
          {
          if ( cv->DataBlocks[ca] )
            {
            cv->DataBlocks[ca]->Delete();
            }
          }
        delete [] cv->DataBlocks;
        delete [] cv->GhostCellsFixed;
        }
      }
    delete [] dp->Variables;
    int block;
    for ( block = 0; block < this->DataDumps[dump].NumberOfBlocks; ++ block )
      {
      vtkSpyPlotUniReader::Block* bk = dp->Blocks+block;
      if ( bk->Allocated )
        {
        bk->XArray->Delete();
        bk->YArray->Delete();
        bk->ZArray->Delete();
        }
      }
    delete [] dp->Blocks;
    }
  delete [] this->DataDumps;
  this->SetFileName(0);
  this->SetCellArraySelection(0);
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::IsVolumeFraction(Variable* var)
{
  return strncmp(var->Name, READ_SPCTH_VOLUME_FRACTION, strlen(READ_SPCTH_VOLUME_FRACTION)) == 0;
}

//-----------------------------------------------------------------------------
void vtkSpyPlotUniReader::SetDownConvertVolumeFraction(int vf)
{
  if ( this->DownConvertVolumeFraction == vf )
    {
    return;
    }
  this->DownConvertVolumeFraction = vf;
  this->DataTypeChanged = 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotReadString(istream &ifs, char* str, size_t len)
{
  ifs.read(str, len);
  if ( len != static_cast<size_t>(ifs.gcount()) )
    {
    return 0;
    }
  return 1;
}

//-----------------------------------------------------------------------------
template<class TypeName>
int vtkSpyPlotReadSpecialType(istream &ifs, TypeName* val, int num)
{
  void* vval = val;
  int ret = ::vtkSpyPlotReadString(ifs, static_cast<char*>(vval), sizeof(TypeName)*num);
  if ( !ret )
    {
    return 0;
    }
  vtkByteSwap::SwapBERange(val, num);
  /*
    cout << " --- read [";
    int cc;
    for ( cc = 0; cc < num; cc ++ )
    {
    cout << " " << val[cc];
    }
    cout << " ]" << endl;
  */
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::ReadString(istream &ifs, char* str, size_t len)
{
  return ::vtkSpyPlotReadString(ifs, str, len);
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::ReadString(istream &ifs, unsigned char* str, size_t len)
{
  return ::vtkSpyPlotReadString(ifs, reinterpret_cast<char*>(str), len);
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::ReadInt(istream &ifs, int* val, int num)
{
  return ::vtkSpyPlotReadSpecialType(ifs, val, num);
}
//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::ReadDouble(istream &ifs, double* val, int num)
{
  return ::vtkSpyPlotReadSpecialType(ifs, val, num);
}
//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::ReadFileOffset(istream &ifs, vtkTypeInt64* val, int num)
{
  int cc;
  for ( cc = 0; cc < num; ++ cc )
    {
    double d;
    int res = this->ReadDouble(ifs, &d, 1);
    if ( !res )
      {
      return 0;
      }
    *val = static_cast<vtkTypeInt64>(d);
    val ++;
    }
  return 1;
}

//-----------------------------------------------------------------------------
istream& vtkSpyPlotUniReader::Seek(istream* stream, vtkTypeInt64 offset, bool rel /*=false*/)
{
  // TODO: Implement 64 bit seeking.
  if ( rel )
    {
    return stream->seekg(offset, ios::cur);
    }
  return stream->seekg(offset);
}

//-----------------------------------------------------------------------------
vtkTypeInt64 vtkSpyPlotUniReader::Tell(istream* stream)
{
  return stream->tellg();
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::ReadInformation()
{
  if ( !this->HaveInformation ) { vtkDebugMacro( << __LINE__ << " " << this << " Read: " << this->HaveInformation ); }
  if ( this->HaveInformation )
    {
    return 1;
    }
  // Initial checks
  if ( !this->CellArraySelection )
    {
    vtkErrorMacro( "Cell array selection not specified" );
    return 0;
    }
  if ( !this->FileName )
    {
    vtkErrorMacro( "FileName not specifed" );
    return 0;
    }
  ifstream ifs(this->FileName, ios::binary|ios::in);
  if ( !ifs )
    {
    vtkErrorMacro( "Cannot open file: " << this->FileName );
    return 0;
    }
  vtkDebugMacro( << this << " Reading file: " << this->FileName );
  //printf("Before anything: %ld\n", ifs.tellg());


  // Ok, file is open, so read the header
  char magic[8];
  if ( !this->ReadString(ifs, magic, 8) )
    {
    vtkErrorMacro( "Cannot read magic" );
    return 0;
    }
  if ( strncmp(magic, "spydata", 7) != 0 || magic[7] != 0)
    {
    vtkErrorMacro( "Bad magic: " << vtkSpyPlotWriteString(magic, 7) );
    return 0;
    }
  if ( !this->ReadString(ifs, this->FileDescription, 128) )
    {
    vtkErrorMacro( "Cannot read FileDescription" );
    return 0;
    }
  //printf("here: %ld\n", ifs.tellg());
  if ( !this->ReadInt(ifs, &(this->FileVersion), 1) )
    {
    vtkErrorMacro( "Cannot read file version" );
    return 0;
    }
  //cout << "File version: " << this->FileVersion << endl;
  if ( this->FileVersion >= 102 )
    {
    if ( !this->ReadInt(ifs, &(this->SizeOfFilePointer), 1) )
      {
      vtkErrorMacro( "Cannot read the seize of file pointer" );
      return 0;
      }
    switch ( this->SizeOfFilePointer )
      {
    case 32:
    case 64:
      break;
    default:
      vtkErrorMacro( "Unknown size of file pointer: " << this->SizeOfFilePointer
        << ". Only handle 32 and 64 bit sizes." );
      return 0;
      }
    //cout << "File pointer size: " << this->SizeOfFilePointer << endl;
    }
  if ( !this->ReadInt(ifs, &(this->FileCompressionFlag), 7) )
    {
    vtkErrorMacro( "Cannot read file version" );
    return 0;
    }
  //printf("here: %ld\n", ifs.tellg());
  if ( !this->ReadDouble(ifs, this->GlobalMin, 6) )
    {
    vtkErrorMacro( "Cannot read file version" );
    return 0;
    }
  //printf("here: %ld\n", ifs.tellg());
  if ( !this->ReadInt(ifs, &(this->NumberOfBlocks), 2) )
    {
    vtkErrorMacro( "Cannot read file version" );
    return 0;
    }
  // Done with header

  //printf("Before cell fields: %ld\n", ifs.tellg());
  // Read all possible cell fields
  if ( !this->ReadInt(ifs, &(this->NumberOfPossibleCellFields), 1) )
    {
    vtkErrorMacro( "Cannot read number of material fields" );
    return 0;
    }
  this->CellFields = new vtkSpyPlotUniReader::CellMaterialField[this->NumberOfPossibleCellFields];
  int fieldCnt;
  for ( fieldCnt = 0; fieldCnt < this->NumberOfPossibleCellFields; ++ fieldCnt )
    {
    vtkSpyPlotUniReader::CellMaterialField *field = this->CellFields + fieldCnt;
    field->Index = 0;
    if ( !this->ReadString(ifs, field->Id, 30) )
      {
      vtkErrorMacro( "Cannot read field " << fieldCnt << " id" );
      return 0;
      }
    if ( !this->ReadString(ifs, field->Comment, 80) )
      {
      vtkErrorMacro( "Cannot read field " << fieldCnt << " commenet" );
      return 0;
      }
    if ( this->FileVersion >= 101 )
      {
      if ( !this->ReadInt(ifs, &(field->Index), 1) )
        {
        vtkErrorMacro( "Cannot read field " << fieldCnt << " int" );
        return 0;
        }
      }
    }

  //printf("Before material fields: %ld\n", ifs.tellg());
  // Read all possible material fields
  if ( !this->ReadInt(ifs, &(this->NumberOfPossibleMaterialFields), 1) )
    {
    vtkErrorMacro( "Cannot read number of material fields" );
    return 0;
    }

  this->MaterialFields = new vtkSpyPlotUniReader::CellMaterialField[this->NumberOfPossibleMaterialFields];
  for ( fieldCnt = 0; fieldCnt < this->NumberOfPossibleMaterialFields; ++ fieldCnt )
    {
    vtkSpyPlotUniReader::CellMaterialField *field = this->MaterialFields + fieldCnt;
    field->Index = 0;
    if ( !this->ReadString(ifs, field->Id, 30) )
      {
      vtkErrorMacro( "Cannot read field " << fieldCnt << " id" );
      return 0;
      }
    if ( !this->ReadString(ifs, field->Comment, 80) )
      {
      vtkErrorMacro( "Cannot read field " << fieldCnt << " commenet" );
      return 0;
      }
    if ( this->FileVersion >= 101 )
      {
      if ( !this->ReadInt(ifs, &(field->Index), 1) )
        {
        vtkErrorMacro( "Cannot read field " << fieldCnt << " int" );
        return 0;
        }
      }
    }

  // Read group headers. Groups are also time steps
  const int MAX_DUMPS = 100;
  struct GroupHeader
  {
    vtkTypeInt64 Offset;
    int NumberOfDataDumps;
    int DumpCycle[MAX_DUMPS];
    double DumpTime[MAX_DUMPS];
    double DumpDT[MAX_DUMPS]; // SPCTH 102 What is this anyway?
    vtkTypeInt64 DumpOffset[MAX_DUMPS];
  };

  struct CummulativeGroupHeader
  {
    int NumberOfDataDumps;
    int *DumpCycle;
    double *DumpTime;
    double *DumpDT; // SPCTH 102 What is this anyway?
    vtkTypeInt64 *DumpOffset;
  };

  int dump;
  while ( 1 )
    {
    GroupHeader gh;
    if ( !this->ReadFileOffset(ifs, &(gh.Offset), 1) )
      {
      vtkErrorMacro( "Cannot get group header offset" );
      return 0;
      }
    vtkTypeInt64 cpos = this->Tell(&ifs);
    //vtkDebugMacro( "position: " << cpos );
    //vtkDebugMacro( "offset:   " << gh.Offset );
    if ( cpos > gh.Offset )
      {
      vtkErrorMacro("The offset is back in file: " << cpos << " > " << gh.Offset);
      return 0;
      }
    this->Seek(&ifs, gh.Offset);
    if ( !this->ReadInt(ifs, &(gh.NumberOfDataDumps), 1) )
      {
      vtkErrorMacro( "Problem reading the num dumps" );
      return 0;
      }
    if ( !this->ReadInt(ifs, gh.DumpCycle, MAX_DUMPS) )
      {
      vtkErrorMacro( "Problem reading the dump times" );
      return 0;
      }
    if ( !this->ReadDouble(ifs, gh.DumpTime, MAX_DUMPS) )
      {
      vtkErrorMacro( "Problem reading the dump times" );
      return 0;
      }
    if ( this->FileVersion >= 102 )
      {
      //cout << "This is SPCTH " << this->FileVersion << " so read DumpDT's" << endl;
      if ( !this->ReadDouble(ifs, gh.DumpDT, MAX_DUMPS) )
        {
        vtkErrorMacro( "Problem reading the dump DT's" );
        return 0;
        }
      }
    if ( !this->ReadFileOffset(ifs, gh.DumpOffset, MAX_DUMPS) )
      {
      vtkErrorMacro( "Problem reading the dump offsets" );
      return 0;
      }
    //vtkDebugMacro( "Number of dumps: " << gh.NumberOfDataDumps );
    //for ( dump = 0; dump < gh.NumberOfDataDumps; ++ dump )
    //  {
    //  vtkDebugMacro( " Dump " << dump << " cycle: " << gh.DumpCycle[dump] << " time: " << gh.DumpTime[dump] << " offset: " << gh.DumpOffset[dump] );
    //  }
    CummulativeGroupHeader nch;
    nch.NumberOfDataDumps = this->NumberOfDataDumps + gh.NumberOfDataDumps;
    nch.DumpCycle  = new int[nch.NumberOfDataDumps];
    nch.DumpTime   = new double[nch.NumberOfDataDumps];
    if ( this->FileVersion >= 102 )
      {
      nch.DumpDT = new double[nch.NumberOfDataDumps];
      }
    nch.DumpOffset = new vtkTypeInt64[nch.NumberOfDataDumps];
    if ( this->DumpCycle )
      {
      memcpy(nch.DumpCycle,  this->DumpCycle,  this->NumberOfDataDumps * sizeof(int));
      memcpy(nch.DumpTime,   this->DumpTime,   this->NumberOfDataDumps * sizeof(double));
      if ( this->FileVersion >= 102 )
        {
        memcpy(nch.DumpDT,   this->DumpDT,   this->NumberOfDataDumps * sizeof(double));
        }
      memcpy(nch.DumpOffset, this->DumpOffset, this->NumberOfDataDumps * sizeof(vtkTypeInt64));
      delete [] this->DumpCycle;
      delete [] this->DumpTime;
      if ( this->FileVersion >= 102 )
        {
        delete [] this->DumpDT;
        }
      delete [] this->DumpOffset;
      }
    memcpy(nch.DumpCycle  + this->NumberOfDataDumps, gh.DumpCycle,  gh.NumberOfDataDumps * sizeof(int));
    memcpy(nch.DumpTime   + this->NumberOfDataDumps, gh.DumpTime,   gh.NumberOfDataDumps * sizeof(double));
    if ( this->FileVersion >= 102 )
      {
      memcpy(nch.DumpDT   + this->NumberOfDataDumps, gh.DumpDT,     gh.NumberOfDataDumps * sizeof(double));
      }
    memcpy(nch.DumpOffset + this->NumberOfDataDumps, gh.DumpOffset, gh.NumberOfDataDumps * sizeof(vtkTypeInt64));

    this->NumberOfDataDumps   = nch.NumberOfDataDumps;
    this->DumpCycle  = nch.DumpCycle;
    this->DumpTime   = nch.DumpTime;
    this->DumpDT     = nch.DumpDT;
    this->DumpOffset = nch.DumpOffset;
    memset(&nch, 0, sizeof(nch));
    if ( gh.NumberOfDataDumps != MAX_DUMPS )
      {
      break;
      }
    }

  this->TimeStepRange[1] = this->NumberOfDataDumps-1;
  this->TimeRange[0] = this->DumpTime[0];
  this->TimeRange[1] = this->DumpTime[this->NumberOfDataDumps-1];

  this->DataDumps = new vtkSpyPlotUniReader::DataDump[this->NumberOfDataDumps];
  for ( dump = 0; dump < this->NumberOfDataDumps; ++dump )
    {
    vtkTypeInt64 cpos = this->Tell(&ifs);
    vtkTypeInt64 offset = this->DumpOffset[dump];
    if ( cpos > offset )
      {
      vtkDebugMacro(<< "The offset is back in file: " << cpos << " > " << offset);
      }
    this->Seek(&ifs, offset);
    vtkSpyPlotUniReader::DataDump *dh = &this->DataDumps[dump];
    memset(dh, 0, sizeof(dh));
    if ( !this->ReadInt(ifs, &(dh->NumVars), 1) )
      {
      vtkErrorMacro( "Cannot read number of variables" );
      return 0;
      }
    if ( dh->NumVars <= 0 )
      {
      vtkErrorMacro( "Got bad number of variables: " << dh->NumVars );
      return 0;
      }
    dh->SavedVariables = new int[ dh->NumVars ];
    dh->SavedVariableOffsets = new vtkTypeInt64[ dh->NumVars ];
    //printf("Reading saved variables: %ld\n", ifs.tellg());
    if ( !this->ReadInt(ifs, dh->SavedVariables, dh->NumVars) )
      {
      vtkErrorMacro( "Cannot read the saved variables" );
      return 0;
      }
    if ( !this->ReadFileOffset(ifs, dh->SavedVariableOffsets, dh->NumVars) )
      {
      vtkErrorMacro( "Cannot read the saved variable offsets" );
      return 0;
      }
    dh->Variables = new vtkSpyPlotUniReader::Variable[dh->NumVars];
    for ( fieldCnt = 0; fieldCnt < dh->NumVars; fieldCnt ++ )
      {
      vtkSpyPlotUniReader::Variable* variable = dh->Variables+fieldCnt;
      variable->Material = -1;
      variable->Index = -1;
      variable->DataBlocks = 0;
      int var = dh->SavedVariables[fieldCnt];
      if ( var >= 100 )
        {
        variable->Index = var % 100 - 1;
        var /= 100;
        var *= 100;
        }
      int cfc;
      if ( variable->Index >= 0 )
        {
        for ( cfc = 0; cfc < this->NumberOfPossibleMaterialFields; ++ cfc )
          {
          if ( this->MaterialFields[cfc].Index == var )
            {
            variable->Material = cfc;
            variable->MaterialField = this->MaterialFields + cfc;
            break;
            }
          }
        }
      else
        {
        for ( cfc = 0; cfc < this->NumberOfPossibleCellFields; ++ cfc )
          {
          if ( this->CellFields[cfc].Index == var )
            {
            variable->Material = cfc;
            variable->MaterialField = this->CellFields + cfc;
            break;
            }
          }
        }
      if ( variable->Material < 0 )
        {
        vtkErrorMacro( "Cannot found variable or material with ID: " << var );
        return 0;
        }
      if ( variable->Index >= 0 )
        {
        ostrstream ostr;
        ostr << this->MaterialFields[variable->Material].Comment << " - " << variable->Index << ends;
        variable->Name = new char[strlen(ostr.str()) + 1];
        strcpy(variable->Name, ostr.str());
        ostr.rdbuf()->freeze(0);
        }
      else
        {
        const char* cname = this->CellFields[variable->Material].Comment;
        variable->Name = new char[strlen(cname) + 1];
        strcpy(variable->Name, cname);
        }
      if ( !this->CellArraySelection->ArrayExists(variable->Name) )
        {
        //vtkDebugMacro( << __LINE__ << " Disable array: " << variable->Name );
        this->CellArraySelection->DisableArray(variable->Name);
        }
      }

    //printf("Before tracers: %ld\n", ifs.tellg());
    // Skip tracers
    int numberOfTracers;
    if ( !this->ReadInt(ifs, &numberOfTracers, 1) )
      {
      vtkErrorMacro( "Problem reading the num of tracers" );
      return 0;
      }
    if ( numberOfTracers > 0 )
      {
      int tracer;
      for ( tracer = 0; tracer < 7; ++ tracer ) // yes, 7 is the magic number
        {
        int someSize;
        if ( !this->ReadInt(ifs, &someSize, 1) )
          {
          vtkErrorMacro( "Problem reading the num of tracers" );
          return 0;
          }
        this->Seek(&ifs, someSize, true);
        }
      }

    // Skip Histogram
    int numberOfIndicators;
    if ( !this->ReadInt(ifs, &numberOfIndicators, 1) )
      {
      vtkErrorMacro( "Problem reading the num of tracers" );
      return 0;
      }
    if ( numberOfIndicators > 0 )
      {
      this->Seek(&ifs, sizeof(int), true);
      int ind;
      for ( ind = 0; ind < numberOfIndicators; ++ ind )
        {
        this->Seek(&ifs,
          sizeof(int) +
          sizeof(double) * 6,
          true);
        int numBins;
        if ( !this->ReadInt(ifs, &numBins, 1) )
          {
          vtkErrorMacro( "Problem reading the num of tracers" );
          return 0;
          }
        if ( numBins > 0 )
          {
          int someSize;
          if ( !this->ReadInt(ifs, &someSize, 1) )
            {
            vtkErrorMacro( "Problem reading the num of tracers" );
            return 0;
            }
          this->Seek(&ifs, someSize, true);
          }
        }
      }

    // Now read the data blocks information
    if ( !this->ReadInt(ifs, &dh->NumberOfBlocks, 1) )
      {
      vtkErrorMacro( "Problem reading the num of blocks" );
      return 0;
      }
    if ( this->NumberOfBlocks != dh->NumberOfBlocks )
      {
      vtkErrorMacro( "Different number of blocks..." );
      }
    dh->Blocks = new vtkSpyPlotUniReader::Block[dh->NumberOfBlocks];
    int block;
    int totalBlocks = 0;
    for ( block = 0; block < dh->NumberOfBlocks; ++ block )
      {
      //long l = ifs.tellg();
      vtkSpyPlotUniReader::Block *b = dh->Blocks + block;
      int bc;
      for ( bc = 0; bc < 6; ++ bc )
        {
        b->RemovedBadGhostCells[bc] = 0;
        }
      if ( !this->ReadInt(ifs, &(b->Nx), 6) )
        {
        vtkErrorMacro( "Problem reading the block information" );
        return 0;
        }

      //printf("Read data block header: n %d allocated %d pos %ld\n", block, b->Allocated, l);
      if ( b->Allocated )
        {
        b->XArray = vtkFloatArray::New();
        b->XArray->SetNumberOfTuples(b->Nx+1);
        b->YArray = vtkFloatArray::New();
        b->YArray->SetNumberOfTuples(b->Ny+1);
        b->ZArray = vtkFloatArray::New();
        b->ZArray->SetNumberOfTuples(b->Nz+1);
        totalBlocks ++;
        }
      else
        {
        b->XArray = 0;
        b->YArray = 0;
        b->ZArray = 0;
        }
      }

    dh->ActualNumberOfBlocks = totalBlocks;

    vtkstd::vector<unsigned char> arrayBuffer;
    for ( block = 0; block < dh->NumberOfBlocks; ++ block )
      {
      vtkSpyPlotUniReader::Block *b = dh->Blocks + block;
      if ( b->Allocated )
        {
        int numBytes;
        int component;
        //vtkDebugMacro( "Block: " << block );
        for ( component = 0; component < 3; ++ component )
          {
          if ( !this->ReadInt(ifs, &numBytes, 1) )
            {
            vtkErrorMacro( "Problem reading the number of bytes" );
            return 0;
            }
          //vtkDebugMacro( "  Number of bytes for " << component << ": " << numBytes );
          if ( static_cast<int>(arrayBuffer.size()) < numBytes )
            {
            arrayBuffer.resize(numBytes);
            }

          if ( !this->ReadString(ifs, &*arrayBuffer.begin(), numBytes) )
            {
            vtkErrorMacro( "Problem reading the bytes" );
            return 0;
            }
          vtkFloatArray* currentArray = 0;
          switch ( component )
            {
            case 0: currentArray = b->XArray; break;
            case 1: currentArray = b->YArray; break;
            case 2: currentArray = b->ZArray; break;
            }
          if ( !currentArray )
            {
            vtkErrorMacro( "Internal error" );
            return 0;
            }
          float* array = currentArray->GetPointer(0);
          int *size = (&b->Nx)+component;
          if ( !this->RunLengthDeltaDecode(&*arrayBuffer.begin(), numBytes, array, (*size) + 1) )
            {
            vtkErrorMacro( "Problem RLD decoding rectilinear grid array: " << component );
            return 0;
            }
          b->VectorsFixedForGhostCells = 0;
          vtkDebugMacro( " " << b << " vectors initialized" );
          }
        }
      }
    }

  this->NumberOfCellFields = this->CellArraySelection->GetNumberOfArrays();
  this->CurrentTime = this->TimeRange[0];
  if ( !this->HaveInformation ) { vtkDebugMacro( << __LINE__ << " " << this << " Read: " << this->HaveInformation ); }
  
  this->HaveInformation = 1;
  if ( !this->HaveInformation ) { vtkDebugMacro( << __LINE__ << " " << this << " Read: " << this->HaveInformation ); }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::ReadData()
{
  if ( !this->HaveInformation ) { vtkDebugMacro( << __LINE__ << " " << this << " Read: " << this->HaveInformation ); }
  if ( !this->HaveInformation )
    {
    if ( !this->ReadInformation() )
      {
      return 0;
      }
    }
  vtkstd::vector<unsigned char> arrayBuffer;
  ifstream ifs(this->FileName, ios::binary|ios::in);
  int dump;
  for ( dump = 0; dump < this->NumberOfDataDumps; ++ dump )
    {
    if ( dump != this->CurrentTimeStep )
      {
      vtkSpyPlotUniReader::DataDump* dp = this->DataDumps+dump;
      int var;
      for ( var = 0; var < dp->NumVars; ++ var)
        {
        vtkSpyPlotUniReader::Variable *cv = dp->Variables + var;
        if ( cv->DataBlocks )
          {
          int ca;
          for ( ca = 0; ca < dp->ActualNumberOfBlocks; ++ ca )
            {
            if ( cv->DataBlocks[ca] )
              {
              cv->DataBlocks[ca]->Delete();
              cv->DataBlocks[ca] = 0;
              }
            }
          vtkDebugMacro( "* Delete Data blocks for variable: " << cv->Name );
          delete [] cv->DataBlocks;
          cv->DataBlocks = 0;
          delete [] cv->GhostCellsFixed;
          cv->GhostCellsFixed = 0;
          }
        }
      }
    }

  dump = this->CurrentTimeStep;
  vtkSpyPlotUniReader::DataDump* dp = this->DataDumps+dump;
  //vtkDebugMacro( "Dump: " << dump << " / " << this->NumberOfDataDumps << " at time: " << this->DumpTime[dump] );
  int fieldCnt;
  for ( fieldCnt = 0; fieldCnt < dp->NumVars; ++ fieldCnt )
    {
    vtkSpyPlotUniReader::Variable* var = dp->Variables + fieldCnt;
    vtkDebugMacro( "Variable: " << var << " (" << var->Name << ") - " << fieldCnt << " (file: " << this->FileName << ") " );

    // Do we need to create new data blocks
    int blocksExists = 0;
    if ( var->DataBlocks )
      {
      vtkDebugMacro( " *** Looks like variable: " << var->Name << " is already loaded" );
      blocksExists = 1;
      }
    // Did we create data blocks that we do not need any more
    if ( !this->CellArraySelection->ArrayIsEnabled(var->Name) ||
         this->DataTypeChanged && this->IsVolumeFraction(var) )
      {
      if ( var->DataBlocks )
        {
        vtkDebugMacro( " ** Variable " << var->Name << " was unselected, so remove" );
        int dataBlock;
        for ( dataBlock = 0; dataBlock < dp->ActualNumberOfBlocks; ++ dataBlock )
          {
          var->DataBlocks[dataBlock]->Delete();
          var->DataBlocks[dataBlock] = 0;
          }
        delete [] var->DataBlocks;
        var->DataBlocks = 0;
        delete [] var->GhostCellsFixed;
        var->GhostCellsFixed = 0;
        vtkDebugMacro( "* Delete Data blocks for variable: " << var->Name );
        }
      vtkDebugMacro( " *** Ignore variable: " << var->Name );
      if ( !this->CellArraySelection->ArrayIsEnabled(var->Name) )
        {
        continue;
        }
      }

    if ( this->CellArraySelection->ArrayIsEnabled(var->Name) && !var->DataBlocks )
      {
      vtkDebugMacro( " ** Allocate new space for variable: " << var->Name << " - " << this->FileName );
      var->DataBlocks = new vtkDataArray*[dp->ActualNumberOfBlocks];
      memset(var->DataBlocks, 0, dp->ActualNumberOfBlocks * sizeof(vtkDataArray*));
      var->GhostCellsFixed = new int[dp->ActualNumberOfBlocks];
      memset(var->GhostCellsFixed, 0, dp->ActualNumberOfBlocks * sizeof(int));
      vtkDebugMacro( " Allocate DataBlocks: " << var->DataBlocks );
      blocksExists = 0;
      }

    if ( blocksExists )
      {
      vtkDebugMacro( << var << " Skip reading of variable: " << var->Name << " / " << this->FileName );
      continue;
      }

    //vtkDebugMacro( "  Field: " << fieldCnt << " / " << dp->NumVars << " [" << var->Name << "]" );
    //vtkDebugMacro( "    Jump to: " << dp->SavedVariableOffsets[fieldCnt] );
    this->Seek(&ifs, dp->SavedVariableOffsets[fieldCnt]);
    int numBytes;
    int block;
    int actualBlockId = 0;
    for ( block = 0; block < dp->NumberOfBlocks; ++ block )
      {
      vtkSpyPlotUniReader::Block* bk = dp->Blocks+block;
      if ( bk->Allocated )
        {
        vtkFloatArray* floatArray = 0;
        vtkUnsignedCharArray* unsignedCharArray = 0;
        vtkDataArray* dataArray = 0;
        if ( this->CellArraySelection->ArrayIsEnabled(var->Name) && !var->DataBlocks[actualBlockId] )
          {
          if ( this->DownConvertVolumeFraction && this->IsVolumeFraction(var) )
            {
            unsignedCharArray = vtkUnsignedCharArray::New();
            dataArray = unsignedCharArray;
            }
          else
            {
            floatArray = vtkFloatArray::New();
            dataArray = floatArray;
            }
          dataArray->SetNumberOfComponents(1);
          dataArray->SetNumberOfTuples(bk->Nx * bk->Ny * bk->Nz);
          dataArray->SetName(var->Name);
          //vtkDebugMacro( "*** Create data array: " << dataArray->GetNumberOfTuples() );
          }
        int zax;
        for ( zax = 0; zax < bk->Nz; ++ zax )
          { 
          int planeSize = bk->Nx * bk->Ny;
          if ( !this->ReadInt(ifs, &numBytes, 1) )
            {
            vtkErrorMacro( "Problem reading the number of bytes" );
            return 0;
            }
          if ( static_cast<int>(arrayBuffer.size()) < numBytes )
            {
            arrayBuffer.resize(numBytes);
            }
          if ( !this->ReadString(ifs, &*arrayBuffer.begin(), numBytes) )
            {
            vtkErrorMacro( "Problem reading the bytes" );
            return 0;
            }
          if ( floatArray )
            {
            float* ptr = floatArray->GetPointer(zax * planeSize);
            if ( !this->RunLengthDataDecode(&*arrayBuffer.begin(), numBytes, ptr, planeSize) )
              {
              vtkErrorMacro( "Problem RLD decoding float data array" );
              return 0;
              }
            }
          if ( unsignedCharArray )
            {
            unsigned char* ptr = unsignedCharArray->GetPointer(zax * planeSize);
            if ( !this->RunLengthDataDecode(&*arrayBuffer.begin(), numBytes, ptr, planeSize) )
              {
              vtkErrorMacro( "Problem RLD decoding unsigned char data array" );
              return 0;
              }
            }
          }
        if ( dataArray )
          {
          var->DataBlocks[actualBlockId] = dataArray;
          var->GhostCellsFixed[actualBlockId] = 0;
          vtkDebugMacro( " " << dataArray << " initialized: " << dataArray->GetName() );
          actualBlockId++;
          }
        }
      }
    }
  this->DataTypeChanged = 0;
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSpyPlotUniReader::PrintMemoryUsage()
{
  int cc;
  cout << "Global size: " << sizeof(this) << endl;

  long total = 0;
  for ( cc = 0; cc < this->NumberOfPossibleCellFields; ++ cc )
    {
    total += sizeof(this->CellFields[cc]);
    }
  cout << "cell fields: " << total << endl;
  total = 0;
  for ( cc = 0; cc < this->NumberOfPossibleMaterialFields; ++ cc )
    {
    total += sizeof(this->MaterialFields[cc]);
    }
  cout << "material fields: " << total << endl;
}

//-----------------------------------------------------------------------------
void vtkSpyPlotUniReader::PrintInformation()
{
  if ( !this->HaveInformation )
    {
    this->ReadInformation();
    }
  vtkDebugMacro( "FileDescription: \"");
  size_t cc;
  for ( cc = 0; cc < 128; ++ cc )
    {
    if ( !this->FileDescription[cc] )
      {
      break;
      }
    }
  //cout.write(this->FileDescription, cc);
  vtkDebugMacro( "\"" );

  vtkDebugMacro( "FileVersion:        " << this->FileVersion );
  vtkDebugMacro( "FileCompressionFlag:    " << this->FileCompressionFlag );
  vtkDebugMacro( "FileProcessorId:        " << this->FileProcessorId );
  vtkDebugMacro( "NumberOfProcessors: " << this->NumberOfProcessors );
  vtkDebugMacro( "IGM:                " << this->IGM );
  vtkDebugMacro( "NumberOfDimensions:            " << this->NumberOfDimensions );
  vtkDebugMacro( "NumberOfMaterials:             " << this->NumberOfMaterials );
  vtkDebugMacro( "MaximumNumberOfMaterials:             " << this->MaximumNumberOfMaterials );
  vtkDebugMacro( "GMin:               " << this->GlobalMin[0] << ", " << this->GlobalMin[1] << ", " << this->GlobalMin[2] );
  vtkDebugMacro( "GMax:               " << this->GlobalMax[0] << ", " << this->GlobalMax[1] << ", " << this->GlobalMax[2] );
  vtkDebugMacro( "NumberOfBlocks:          " << this->NumberOfBlocks );
  vtkDebugMacro( "MaximumNumberOfLevels:          " << this->MaximumNumberOfLevels );
  vtkDebugMacro( "NumberOfPossibleCellFields:      " << this->NumberOfPossibleCellFields );

  vtkDebugMacro( "Cell fields: " );
  int fieldCnt;
  for ( fieldCnt = 0; fieldCnt < this->NumberOfPossibleCellFields; ++ fieldCnt )
    {
    vtkDebugMacro( "Cell field " << fieldCnt );
    vtkDebugMacro( "  Id:      " << this->CellFields[fieldCnt].Id );
    vtkDebugMacro( "  Comment: " << this->CellFields[fieldCnt].Comment );
    vtkDebugMacro( "  Index:     " << this->CellFields[fieldCnt].Index );
    }

  vtkDebugMacro( "Material fields: " );
  for ( fieldCnt = 0; fieldCnt < this->NumberOfPossibleMaterialFields; ++ fieldCnt )
    {
    vtkDebugMacro( "Material field " << fieldCnt );
    vtkDebugMacro( "  Id:      " << this->MaterialFields[fieldCnt].Id );
    vtkDebugMacro( "  Comment: " << this->MaterialFields[fieldCnt].Comment );
    vtkDebugMacro( "  Index:     " << this->MaterialFields[fieldCnt].Index );
    }

  vtkDebugMacro( "Cummulative number of dumps: " << this->NumberOfDataDumps );
  int dump;
  for ( dump = 0; dump < this->NumberOfDataDumps; ++ dump )
    {
    vtkDebugMacro( " Dump " << dump << " cycle: " << this->DumpCycle[dump] << " time: " << this->DumpTime[dump] << " offset: " << this->DumpOffset[dump] );
    }

  vtkDebugMacro( "Headers: " );
  for ( dump = 0; dump < this->NumberOfDataDumps; ++dump )
    {
    vtkSpyPlotUniReader::DataDump* dp = this->DataDumps+dump;
    vtkDebugMacro( "  " << dump );
    vtkTypeInt64 offset = this->DumpOffset[dump];
    vtkDebugMacro( "    offset:   " << offset << " number of variables: " << dp->NumVars );
    int var;
    for ( var = 0; var < dp->NumVars; ++ var )
      {
      vtkDebugMacro( "      Variable: " << dp->SavedVariables[var] << " -> " << dp->SavedVariableOffsets[var] );
      }

    int block;
    vtkDebugMacro( "Blocks: " );
    for ( block = 0; block < dp->NumberOfBlocks; ++ block )
      {
      vtkSpyPlotUniReader::Block *b = dp->Blocks + block;
      vtkDebugMacro( "  " << block );
      vtkDebugMacro( "    Allocated: " << b->Allocated );
      vtkDebugMacro( "    Active: " << b->Active );
      vtkDebugMacro( "    Level: " << b->Level );
      vtkDebugMacro( "    Nx: " << b->Nx );
      vtkDebugMacro( "    Ny: " << b->Ny );
      vtkDebugMacro( "    Nz: " << b->Nz );
      if ( b->Allocated )
        {
        int num;
        vtkDebugMacro( "    XArray:");
        for ( num = 0; num <= b->Nx; num ++ )
          {
          vtkDebugMacro( " " << b->XArray->GetValue(num));
          }
        vtkDebugMacro( "    YArray:");
        for ( num = 0; num <= b->Ny; num ++ )
          {
          vtkDebugMacro( " " << b->YArray->GetValue(num));
          }
        vtkDebugMacro( "    ZArray:");
        for ( num = 0; num <= b->Nz; num ++ )
          {
          vtkDebugMacro( " " << b->ZArray->GetValue(num));
          }
        }
      }

    for ( fieldCnt = 0; fieldCnt < dp->NumVars; ++ fieldCnt )
      {
      vtkSpyPlotUniReader::Variable* currentVar = dp->Variables + fieldCnt;
      vtkDebugMacro( "   Variable: " << fieldCnt << " - \"" << currentVar->Name << "\" Material: " << currentVar->Material );
      if ( currentVar->DataBlocks )
        {
        int dataBlock;
        for ( dataBlock = 0; dataBlock < dp->ActualNumberOfBlocks; ++ dataBlock)
          {
          vtkDebugMacro( "      DataBlock: " << dataBlock );
          if ( currentVar->DataBlocks[dataBlock] )
            {
            currentVar->DataBlocks[dataBlock]->Print(cout);
            }
          vtkDebugMacro( "      Ghost cells fixed: " << currentVar->GhostCellsFixed[dataBlock] );
          }
        }
      else
        {
        vtkDebugMacro( "      Not read" );
        }
      }
    }

  this->CellArraySelection->Print(cout);

}

//-----------------------------------------------------------------------------
/* Routine run-length-encodes the data pointed to by *data, placing
   the result in *out. n is the number of doubles to encode. n_out
   is the number of bytes used for the compression (and stored at
   *out). delta is the smallest change of adjacent values that will
   be accepted (changes smaller than delta will be ignored). 

   Note: *out needs to be allocated by the calling application. 
   Its worst-case size is 5*n bytes. */



/* Routine run-length-decodes the data pointed to by *in and
   returns a collection of doubles in *data. Performs the
   inverse of rle above. Application should provide both
   n (the expected number of doubles) and n_in the number
   of bytes to decode from *in. Again, the application needs
   to provide allocated space for *data which will be
   n bytes long. */

int vtkSpyPlotUniReader::RunLengthDeltaDecode(const unsigned char* in, int inSize, float* out, int outSize)
{
  int outIndex = 0, inIndex = 0;

  const unsigned char* ptmp = in;

  /* Run-length decode */

  // Get first value
  float val;
  memcpy(&val, ptmp, sizeof(float));
  vtkByteSwap::SwapBE(&val);
  ptmp += 4;

  // Get delta
  float delta;
  memcpy(&delta, ptmp, sizeof(float));
  vtkByteSwap::SwapBE(&delta);
  ptmp += 4;

  // Now loop around until I get to the end of
  // the input array
  inIndex += 8;
  while ((outIndex<outSize) && (inIndex<inSize))
    {
    // Okay get the run length
    unsigned char runLength = *ptmp;
    ptmp ++;
    if (runLength < 128)
      {
      ptmp += 4;
      // Now populate the out data
      int k;
      for (k=0; k<runLength; ++k)
        {
        if ( outIndex >= outSize )
          {
          vtkErrorMacro( "Problem doing RLD decode. Too much data generated. Excpected: " << outSize );
          return 0;
          }
        out[outIndex] = val + outIndex*delta;
        outIndex++;
        }
      inIndex += 5;
      }
    else  // runLength >= 128
      {
      int k;
      for (k=0; k<runLength-128; ++k)
        {
        if ( outIndex >= outSize )
          {
          vtkErrorMacro( "Problem doing RLD decode. Too much data generated. Excpected: " << outSize );
          return 0;
          }
        float nval;
        memcpy(&nval, ptmp, sizeof(float));
        vtkByteSwap::SwapBE(&nval);
        out[outIndex] = nval + outIndex*delta;
        outIndex++;
        ptmp += 4;
        }
      inIndex += 4*(runLength-128)+1;
      }
    } // while 

  return 1;
} 

//-----------------------------------------------------------------------------
template<class t>
int vtkSpyPlotUniReaderRunLengthDataDecode(vtkSpyPlotUniReader* self, const unsigned char* in, int inSize, t* out, int outSize, t scale=1)
{
  int outIndex = 0, inIndex = 0;

  const unsigned char* ptmp = in;

  /* Run-length decode */
  while ((outIndex<outSize) && (inIndex<inSize))
    {
    // Okay get the run length
    unsigned char runLength = *ptmp;
    ptmp ++;
    if (runLength < 128)
      {
      float val;
      memcpy(&val, ptmp, sizeof(float));
      vtkByteSwap::SwapBE(&val);
      ptmp += 4;
      // Now populate the out data
      int k;
      for (k=0; k<runLength; ++k)
        {
        if ( outIndex >= outSize )
          {
          vtkErrorWithObjectMacro(self, "Problem doing RLD decode. Too much data generated. Excpected: " << outSize );
          return 0;
          }
        out[outIndex] = static_cast<t>(val*scale);
        outIndex++;
        }
      inIndex += 5;
      }
    else  // runLength >= 128
      {
      int k;
      for (k=0; k<runLength-128; ++k)
        {
        if ( outIndex >= outSize )
          {
          vtkErrorWithObjectMacro(self, "Problem doing RLD decode. Too much data generated. Excpected: " << outSize );
          return 0;
          }
        float val;
        memcpy(&val, ptmp, sizeof(float));
        vtkByteSwap::SwapBE(&val);
        out[outIndex]=static_cast<t>(val*scale);
        outIndex++;
        ptmp += 4;
        }
      inIndex += 4*(runLength-128)+1;
      }
    } // while

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::RunLengthDataDecode(const unsigned char* in, int inSize, float* out, int outSize)
{
  return ::vtkSpyPlotUniReaderRunLengthDataDecode(this, in, inSize, out, outSize);
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::RunLengthDataDecode(const unsigned char* in, int inSize, unsigned char* out, int outSize)
{
  return ::vtkSpyPlotUniReaderRunLengthDataDecode(this, in, inSize, out, outSize, static_cast<unsigned char>(255));
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::SetCurrentTime(double time)
{
  if ( !this->HaveInformation ) { vtkDebugMacro( << __LINE__ << " " << this << " Read: " << this->HaveInformation ); }
  this->ReadInformation();
  if ( time < this->TimeRange[0] || time > this->TimeRange[1] )
    {
    return 0;
    }
  this->CurrentTime = time;
  this->CurrentTimeStep = this->GetTimeStepFromTime(time);
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::SetCurrentTimeStep(int timeStep)
{
  if ( !this->HaveInformation ) { vtkDebugMacro( << __LINE__ << " " << this << " Read: " << this->HaveInformation ); }
  this->ReadInformation();
  if ( timeStep < this->TimeStepRange[0] || timeStep > this->TimeStepRange[1] )
    {
    return 0;
    }
  this->CurrentTimeStep = timeStep;
  this->CurrentTime = this->GetTimeFromTimeStep(timeStep);
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::GetTimeStepFromTime(double time)
{
  if ( !this->HaveInformation ) { vtkDebugMacro( << __LINE__ << " " << this << " Read: " << this->HaveInformation ); }
  this->ReadInformation();
  int dump;
  for ( dump = 0; dump < this->NumberOfDataDumps; ++ dump )
    {
    if ( time < this->DumpTime[dump] )
      {
      return dump-1;
      }
    }
  return this->TimeStepRange[1];
}

//-----------------------------------------------------------------------------
double vtkSpyPlotUniReader::GetTimeFromTimeStep(int timeStep)
{
  if ( !this->HaveInformation ) { vtkDebugMacro( << __LINE__ << " " << this << " Read: " << this->HaveInformation ); }
  this->ReadInformation();
  if ( timeStep < this->TimeStepRange[0] )
    {
    return this->TimeRange[0];
    }
  if ( timeStep > this->TimeStepRange[1] )
    {
    return this->TimeRange[1];
    }
  return this->DumpTime[timeStep];
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::GetNumberOfDataBlocks()
{
  if ( !this->HaveInformation ) { vtkDebugMacro( << __LINE__ << " " << this << " Read: " << this->HaveInformation ); }
  this->ReadInformation();
  return this->DataDumps[this->CurrentTimeStep].ActualNumberOfBlocks;
}

//-----------------------------------------------------------------------------
vtkSpyPlotUniReader::Block* vtkSpyPlotUniReader::GetDataBlock(int block)
{
  if ( !this->HaveInformation ) { vtkDebugMacro( << __LINE__ << " " << this << " Read: " << this->HaveInformation ); }
  this->ReadInformation();
  vtkSpyPlotUniReader::DataDump* dp = this->DataDumps+this->CurrentTimeStep;
  int cb = 0;
  int blockId;
  for ( blockId = 0; blockId < dp->NumberOfBlocks; ++ blockId )
    {
    if ( dp->Blocks[blockId].Allocated )
      {
      if ( cb == block )
        {
        return dp->Blocks+blockId;
        }
      cb ++;
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
vtkSpyPlotUniReader::Variable* vtkSpyPlotUniReader::GetCellField(int field)
{
  if ( !this->HaveInformation ) { vtkDebugMacro( << __LINE__ << " " << this << " Read: " << this->HaveInformation ); }
  this->ReadInformation();
  vtkSpyPlotUniReader::DataDump* dp = this->DataDumps+this->CurrentTimeStep;
  if ( field < 0 || field >= dp->NumVars )
    {
    return 0;
    }
  return  dp->Variables + field;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::GetDataBlockLevel(int block)
{
  vtkSpyPlotUniReader::Block* bk = this->GetDataBlock(block);
  if ( !bk )
    {
    return 0;
    }
  return bk->Level;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::GetDataBlockDimensions(int block, int* dims, int* fixed)
{
  vtkSpyPlotUniReader::Block* bk = this->GetDataBlock(block);
  if ( !bk )
    {
    return 0;
    }
  dims[0] = bk->XArray->GetNumberOfTuples()-1;
  dims[1] = bk->YArray->GetNumberOfTuples()-1;
  dims[2] = bk->ZArray->GetNumberOfTuples()-1;
  vtkDebugMacro( "Dimensions: " << coutVector3(dims) );

  dims[0] = bk->Nx;
  dims[1] = bk->Ny;
  dims[2] = bk->Nz;
  vtkDebugMacro( "Real Dimensions: " << coutVector3(dims) );

  *fixed = bk->VectorsFixedForGhostCells;
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::GetDataBlockBounds(int block, double* bounds, int* fixed)
{
  vtkSpyPlotUniReader::Block* bk = this->GetDataBlock(block);
  if ( !bk )
    {
    return 0;
    }
  bounds[0] = bk->XArray->GetTuple1(0);
  bounds[1] = bk->XArray->GetTuple1(bk->XArray->GetNumberOfTuples()-1);
  bounds[2] = bk->YArray->GetTuple1(0);
  bounds[3] = bk->YArray->GetTuple1(bk->YArray->GetNumberOfTuples()-1);
  bounds[4] = bk->ZArray->GetTuple1(0);
  bounds[5] = bk->ZArray->GetTuple1(bk->ZArray->GetNumberOfTuples()-1);
  *fixed = bk->VectorsFixedForGhostCells;
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::GetDataBlockVectors(int block, vtkDataArray** coordinates, int *fixed)
{
  vtkSpyPlotUniReader::Block* bk = this->GetDataBlock(block);
  if ( !bk )
    {
    return 0;
    }
  coordinates[0] = bk->XArray;
  coordinates[1] = bk->YArray;
  coordinates[2] = bk->ZArray;
  *fixed = bk->VectorsFixedForGhostCells;
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::MarkVectorsAsFixed(int block)
{
  vtkSpyPlotUniReader::Block* bk = this->GetDataBlock(block);
  if ( !bk )
    {
    return 0;
    }
  bk->VectorsFixedForGhostCells = 1;
  vtkDebugMacro( " " << bk << " Vectors are fixed " << this->FileName );
  return 1;
}

//-----------------------------------------------------------------------------
const char* vtkSpyPlotUniReader::GetCellFieldName(int field)
{
  vtkSpyPlotUniReader::Variable *var = this->GetCellField(field);
  if ( !var )
    {
    return 0;
    }
  return var->Name;
}

//-----------------------------------------------------------------------------
vtkDataArray* vtkSpyPlotUniReader::GetCellFieldData(int block, int field, int* fixed)
{
  vtkSpyPlotUniReader::DataDump* dp = this->DataDumps+this->CurrentTimeStep;
  if ( block < 0 || block > dp->ActualNumberOfBlocks )
    {
    return 0;
    }
  vtkSpyPlotUniReader::Variable *var = this->GetCellField(field);
  if ( !var )
    {
    return 0;
    }

  *fixed = var->GhostCellsFixed[block];

  vtkDebugMacro( "GetCellField(" << block << " " << field << " " << *fixed << ") = " << var->DataBlocks[block] );
  return var->DataBlocks[block];
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::MarkCellFieldDataFixed(int block, int field)
{
  vtkSpyPlotUniReader::DataDump* dp = this->DataDumps+this->CurrentTimeStep;
  if ( block < 0 || block > dp->ActualNumberOfBlocks )
    {
    return 0;
    }
  vtkSpyPlotUniReader::Variable *var = this->GetCellField(field);
  if ( !var )
    {
    return 0;
    }
  var->GhostCellsFixed[block] = 1;
  vtkDebugMacro( " " << var->DataBlocks[block] << " fixed: " << var->DataBlocks[block]->GetName() );
  return 1;
}

//-----------------------------------------------------------------------------
//=============================================================================
class vtkSpyPlotReaderMap
{
public:
  typedef vtkstd::map<vtkstd::string, vtkSpyPlotUniReader*> MapOfStringToSPCTH;
  typedef vtkstd::vector<vtkstd::string> VectorOfStrings;
  MapOfStringToSPCTH Files;
  vtkstd::string MasterFileName;

  void Initialize(const char *file)
    {
      if ( !file || file != this->MasterFileName )
        {
        this->Clean(0);
        }
    }
  void Clean(vtkSpyPlotUniReader* save)
    {
      MapOfStringToSPCTH::iterator it=this->Files.begin();
      MapOfStringToSPCTH::iterator end=this->Files.end();
      while(it!=end)
        {
        if ( it->second && it->second != save )
          {
          it->second->Delete();
          it->second = 0;
          }
        ++it;
        }
      this->Files.erase(this->Files.begin(),end);
    }
  vtkSpyPlotUniReader* GetReader(MapOfStringToSPCTH::iterator& it, vtkSpyPlotReader* parent)
    {
      if ( !it->second )
        {
        it->second = vtkSpyPlotUniReader::New();
        it->second->SetCellArraySelection(parent->GetCellDataArraySelection());
        it->second->SetFileName(it->first.c_str());
        //cout << parent->GetController()->GetLocalProcessId() << "Create reader: " << it->second << endl;
        }
      return it->second;
    }
};

//-----------------------------------------------------------------------------
class vtkSpyPlotBlockIterator
{
public:
  // Description:
  // Initialize the iterator with informations about processors,
  // files and timestep.
  virtual void Init(int numberOfProcessors,
                    int processorId,
                    vtkSpyPlotReader *parent,
                    vtkSpyPlotReaderMap *fileMap,
                    int currentTimeStep)
    {
      assert("pre: fileMap_exists" && fileMap!=0);
      
      this->NumberOfProcessors=numberOfProcessors;
      this->ProcessorId=processorId;
      this->FileMap=fileMap;
      this->Parent = parent;
      this->CurrentTimeStep=currentTimeStep;
      this->NumberOfFiles=this->FileMap->Files.size();
    }
  
  // Description:
  // Go to first block if any.
  virtual void Start()=0;
 
  // Description:
  // Returns the total number of blocks to be processed by this Processor.
  // Can be called only after Init().
  virtual int GetNumberOfBlocksToProcess()=0;
  
  // Description:
  // Is there no block at current position?
  int IsOff()
    {
      return this->Off;
    }
  
  // Description:
  // Go to the next block if any
  // \pre not_is_off: !IsOff()
  void Next()
    {
      assert("pre: not_is_off" && !IsOff() );
      
      ++this->Block;
      if(this->Block>this->BlockEnd)
        {
        ++this->FileIterator;
        ++this->FileIndex;
        this->FindFirstBlockOfCurrentOrNextFile();
        }
    }
  
  // Description:
  // Return the block at current position.
  // \pre not_is_off: !IsOff()
  int GetBlock()
    {
      assert("pre: not_is_off" && !IsOff() );
      return this->Block;
    }
  
  // Description:
  // Return the number of fields at current position.
  // \pre not_is_off: !IsOff()
  int GetNumberOfFields()
    {
      assert("pre: not_is_off" && !IsOff() );
      return this->NumberOfFields;
    }
  
  // Description:
  // Return the SPCTH API handle at current position.
  // \pre not_is_off: !IsOff()
  vtkSpyPlotUniReader *GetUniReader()
    {
      assert("pre: not_is_off" && !IsOff() );
      return this->UniReader;
    }
  
  virtual ~vtkSpyPlotBlockIterator() {}

protected:
  vtkSpyPlotBlockIterator()
    {
      this->NumberOfProcessors = 0;
      this->ProcessorId = 0;
      this->CurrentTimeStep = 0;

      this->NumberOfFiles = 0;

      this->Off = 0;
      this->Block = 0;
      this->NumberOfFields = 0;
      this->FileIndex = 0;

      this->BlockEnd = 0;
      this->FileMap = 0;
      this->UniReader = 0;
      this->Parent = 0;
    }

  virtual void FindFirstBlockOfCurrentOrNextFile()=0;
  
  int NumberOfProcessors;
  int ProcessorId;
  vtkSpyPlotReaderMap *FileMap;
  int CurrentTimeStep;
  
  int NumberOfFiles;
  
  int Off;
  int Block;
  int NumberOfFields;
  vtkSpyPlotUniReader *UniReader;
  
  vtkSpyPlotReaderMap::MapOfStringToSPCTH::iterator FileIterator;
  int FileIndex;
  
  int BlockEnd;
  vtkSpyPlotReader* Parent;
};

class vtkSpyPlotBlockDistributionBlockIterator
  : public vtkSpyPlotBlockIterator
{
public:
  vtkSpyPlotBlockDistributionBlockIterator() {}
  virtual ~vtkSpyPlotBlockDistributionBlockIterator() {}
  
  virtual void Start()
    {
      this->FileIterator=this->FileMap->Files.begin();
      this->FileIndex=0;
      this->FindFirstBlockOfCurrentOrNextFile();
    }
 
  
  virtual int GetNumberOfBlocksToProcess()
    {
      // When distributing blocks, each process is as such
      // going to read each file, so it's okay if this method 
      // creates reader for all files and reads information from them.
      int total_num_blocks = 0;
      vtkSpyPlotReaderMap::MapOfStringToSPCTH::iterator fileIterator;
      fileIterator = this->FileMap->Files.begin();
      int numFiles = this->FileMap->Files.size();
      int cur_file = 1;
      int progressInterval = numFiles/20 + 1;
      for ( ;fileIterator != this->FileMap->Files.end(); fileIterator++, cur_file++)
        {
        if ( !(cur_file%progressInterval) )
          {
          this->Parent->UpdateProgress(0.2 * static_cast<double>(cur_file)/numFiles);
          }
        vtkSpyPlotUniReader* reader = this->FileMap->GetReader(fileIterator, 
                                                               this->Parent);
        reader->ReadInformation();
        reader->SetCurrentTimeStep(this->CurrentTimeStep);
        int numBlocks = reader->GetNumberOfDataBlocks();
        int numBlocksPerProcess = ( numBlocks / this->NumberOfProcessors);
        int leftOverBlocks = numBlocks - 
          (numBlocksPerProcess*this->NumberOfProcessors);
        if (this->ProcessorId < leftOverBlocks)
          {
          total_num_blocks += numBlocksPerProcess + 1;
          }
        else
          {
          total_num_blocks += numBlocksPerProcess;
          }
        }
      return total_num_blocks;
    }

protected:
  virtual void FindFirstBlockOfCurrentOrNextFile()
    {
      this->Off=this->FileIndex>=this->NumberOfFiles;
      int found=0;
      while(!this->Off && !found)
        {
        const char *fname=this->FileIterator->first.c_str();
        this->UniReader=this->FileMap->GetReader(this->FileIterator, this->Parent);
        this->UniReader->SetFileName(fname);
        this->UniReader->ReadInformation();
        this->UniReader->SetCurrentTimeStep(this->CurrentTimeStep);
        
        this->NumberOfFields=this->UniReader->GetNumberOfCellFields();
        
        int numBlocks=this->UniReader->GetNumberOfDataBlocks();
        
        found=this->ProcessorId<numBlocks;
          
        if(found) // otherwise skip to the next file
          {
          int numBlocksPerProcess=numBlocks/this->NumberOfProcessors;
          int leftOverBlocks=numBlocks-
            (numBlocksPerProcess*this->NumberOfProcessors);
          
          int blockStart;
          
          if(this->ProcessorId<leftOverBlocks)
            {
            blockStart=(numBlocksPerProcess+1)*this->ProcessorId;
            this->BlockEnd=blockStart+(numBlocksPerProcess+1)-1;
            }
          else
            {
            blockStart=numBlocksPerProcess*this->ProcessorId+leftOverBlocks;
            this->BlockEnd=blockStart+numBlocksPerProcess-1;
            }
          this->Block=blockStart;
          found=this->Block<=this->BlockEnd;
          }
        if(!found)
          {
          ++this->FileIterator;
          ++this->FileIndex;
          this->Off=this->FileIndex>=this->NumberOfFiles;
          }
#if 0
        else
          {
          cout<<"proc="<<this->ProcessorId<<" file="<<this->FileIndex<<" blockStart="<<this->Block<<" blockEnd="<<this->BlockEnd<<" numBlock="<<numBlocks<<endl;
          }
#endif
        }
    }
};

class vtkSpyPlotFileDistributionBlockIterator
  : public vtkSpyPlotBlockIterator
{
public:
  vtkSpyPlotFileDistributionBlockIterator()
    {
      this->FileStart = 0;
      this->FileEnd = 0;
    }
  virtual ~vtkSpyPlotFileDistributionBlockIterator() {}
  virtual void Init(int numberOfProcessors,
                    int processorId,
                    vtkSpyPlotReader *parent,
                    vtkSpyPlotReaderMap *fileMap,
                    int currentTimeStep)
    {
      vtkSpyPlotBlockIterator::Init(numberOfProcessors,processorId,parent,fileMap,
                                    currentTimeStep);
      
      if(this->ProcessorId>=this->NumberOfFiles)
        {
        this->FileEnd=this->NumberOfFiles;
        this->FileStart=this->FileEnd+1;
        }
      else
        {
        int numFilesPerProcess=this->NumberOfFiles/this->NumberOfProcessors;
        int leftOverFiles=this->NumberOfFiles
          -(numFilesPerProcess*this->NumberOfProcessors);
        
        if (this->ProcessorId < leftOverFiles)
          {
          this->FileStart = (numFilesPerProcess+1) * this->ProcessorId;
          this->FileEnd = this->FileStart + (numFilesPerProcess+1) - 1;
          }
        else
          {
          this->FileStart=numFilesPerProcess*this->ProcessorId + leftOverFiles;
          this->FileEnd = this->FileStart + numFilesPerProcess - 1;
          }
        }
    }
  
  virtual void Start()
    {
      this->Off=this->ProcessorId>=this->NumberOfFiles; // processor not used
      if(!this->Off)
        {
        // skip the first files
        this->FileIndex=0;
        this->FileIterator=this->FileMap->Files.begin();
        while(this->FileIndex<this->FileStart)
          {
          ++this->FileIterator;
          ++this->FileIndex;
          }
        
        this->FindFirstBlockOfCurrentOrNextFile();
        }
    }
 
  virtual int GetNumberOfBlocksToProcess()
    {
      int total_num_blocks = 0;
      vtkSpyPlotReaderMap::MapOfStringToSPCTH::iterator fileIterator;
      fileIterator = this->FileMap->Files.begin();
      int file_index = 0;
      int numFiles = this->FileEnd - this->FileStart + 1;
      int progressInterval = numFiles/ 20 + 1;
      for ( ;fileIterator != this->FileMap->Files.end() && file_index <= this->FileEnd; 
            fileIterator++, file_index++)
        {
        if (file_index < this->FileStart)
          {
          continue;
          }
        if ( !(file_index % progressInterval) )
          {
          this->Parent->UpdateProgress(0.2 * (file_index+1.0)/numFiles);
          }
        vtkSpyPlotUniReader* reader = this->FileMap->GetReader(fileIterator, 
                                                               this->Parent);
        reader->ReadInformation();
        reader->SetCurrentTimeStep(this->CurrentTimeStep);
        total_num_blocks += reader->GetNumberOfDataBlocks();
        }
      return total_num_blocks;
    }

protected:
  virtual void FindFirstBlockOfCurrentOrNextFile()
    {
      this->Off=this->FileIndex>this->FileEnd;
      int found=0;
      while(!this->Off && !found)
        {
        const char *fname=this->FileIterator->first.c_str();
        this->UniReader=this->FileMap->GetReader(this->FileIterator, this->Parent);
//        vtkDebugMacro("Reading data from file: " << fname);
        
        this->UniReader->SetFileName(fname);
        this->UniReader->ReadInformation();
        this->UniReader->SetCurrentTimeStep(this->CurrentTimeStep);
        
        this->NumberOfFields=this->UniReader->GetNumberOfCellFields();
        
        int numberOfBlocks;
        numberOfBlocks=this->UniReader->GetNumberOfDataBlocks();
        
        this->BlockEnd=numberOfBlocks-1;
        this->Block=0;
        found=this->Block<=this->BlockEnd;
        if(!found)
          {
          ++this->FileIterator;
          ++this->FileIndex;
          this->Off=this->FileIndex>this->FileEnd;
          }
        }
    }
  int FileStart;
  int FileEnd;
};

//=============================================================================
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Return whether the path represents a full path (not relative). 
static bool FileIsFullPath(const char* in_name)
{
  vtkstd::string name = in_name;
#if defined(_WIN32) || defined(__CYGWIN__)
  // On Windows, the name must be at least two characters long.
  if(name.length() < 2)
    {
    return false;
    }
  if(name[1] == ':')
    {
    return true;
    }
  if(name[0] == '\\')
    {
    return true;
    }
#else
  // On UNIX, the name must be at least one character long.
  if(name.length() < 1)
    {
    return false;
    }
#endif
  // On UNIX, the name must begin in a '/'.
  // On Windows, if the name begins in a '/', then it is a full
  // network path.
  if(name[0] == '/')
    {
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
// replace replace with with as many times as it shows up in source.
// write the result into source.
static void ReplaceString(vtkstd::string& source,
                          const char* replace,
                          const char* with)
{
  const char *src = source.c_str();
  char *searchPos = const_cast<char *>(strstr(src,replace));

  // get out quick if string is not found
  if (!searchPos)
    {
    return;
    }

  // perform replacements until done
  size_t replaceSize = strlen(replace);
  char *orig = strdup(src);
  char *currentPos = orig;
  searchPos = searchPos - src + orig;

  // initialize the result
  source.erase(source.begin(),source.end());
  do
    {
    *searchPos = '\0';
    source += currentPos;
    currentPos = searchPos + replaceSize;
    // replace
    source += with;
    searchPos = strstr(currentPos,replace);
    }
  while (searchPos);

  // copy any trailing text
  source += currentPos;
  free(orig);
}

//-----------------------------------------------------------------------------
// convert windows slashes to unix slashes 
//
static void ConvertToUnixSlashes(vtkstd::string& path)
{
  vtkstd::string::size_type pos = 0;
  while((pos = path.find('\\', pos)) != vtkstd::string::npos)
    {
    // make sure we don't convert an escaped space to a unix slash
    if(pos < path.size()-1)
      {
      if(path[pos+1] != ' ')
        {
        path[pos] = '/';
        }
      }
    pos++;
    }
  // Remove all // from the path just like most unix shells
  int start_find;

#ifdef _WIN32
  // However, on windows if the first characters are both slashes,
  // then keep them that way, so that network paths can be handled.
  start_find = 1;
#else
  start_find = 0;
#endif

  while((pos = path.find("//", start_find)) != vtkstd::string::npos)
    {
    ::ReplaceString(path, "//", "/");
    }

  // remove any trailing slash
  if(path.size() > 1 && path[path.size()-1] == '/')
    {
    path = path.substr(0, path.size()-1);
    }

  // if there is a tilda ~ then replace it with HOME
  if(path.find("~") == 0)
    {
    if (getenv("HOME"))
      {
      path = vtkstd::string(getenv("HOME")) + path.substr(1);
      }
    }
}

//-----------------------------------------------------------------------------
// Return path of a full filename (no trailing slashes).
// Warning: returned path is converted to Unix slashes format.
//
static vtkstd::string GetFilenamePath(const vtkstd::string& filename)
{
  vtkstd::string fn = filename;
  ::ConvertToUnixSlashes(fn);

  vtkstd::string::size_type slash_pos = fn.rfind("/");
  if(slash_pos != vtkstd::string::npos)
    {
    return fn.substr(0, slash_pos);
    }
  else
    {
    return "";
    }
}

//-----------------------------------------------------------------------------
// Due to a buggy stream library on the HP and another on Mac OSX, we
// need this very carefully written version of getline.  Returns true
// if any data were read before the end-of-file was reached.
// 
static bool GetLineFromStream(istream& is,
                              vtkstd::string& line, bool *has_newline = 0)
{
  const int bufferSize = 1024;
  char buffer[bufferSize];
  line = "";
  bool haveData = false;
  if ( has_newline )
    {
    *has_newline = false;
    }

  // If no characters are read from the stream, the end of file has
  // been reached.
  while((is.getline(buffer, bufferSize), is.gcount() > 0))
    {
    haveData = true;
    line.append(buffer);

    // If newline character was read, the gcount includes the
    // character, but the buffer does not.  The end of line has been
    // reached.
    if(strlen(buffer) < static_cast<size_t>(is.gcount()))
      {
      if ( has_newline )
        {
        *has_newline = true;
        }
      break;
      }

    // The fail bit may be set.  Clear it.
    is.clear(is.rdstate() & ~ios::failbit);
    }
  return haveData;
}


//-----------------------------------------------------------------------------
vtkSpyPlotReader::vtkSpyPlotReader()
{
  this->SetNumberOfInputPorts(0);
  
  this->Map = new vtkSpyPlotReaderMap;
  this->FileName = 0;
  this->CurrentFileName = 0;
  this->CellDataArraySelection = vtkDataArraySelection::New();
  // Setup the selection callback to modify this object when an array
  // selection is changed.
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(
    &vtkSpyPlotReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->CellDataArraySelection->AddObserver(vtkCommand::ModifiedEvent,
                                            this->SelectionObserver);
  this->TimeStep = 0;
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;

  this->DownConvertVolumeFraction = 1;

  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  
  this->DistributeFiles=0; // by default, distribute blocks, not files.
  this->GenerateLevelArray=0; // by default, do not generate level array.
  this->GenerateBlockIdArray=0; // by default, do not generate block id array.
  this->GenerateActiveBlockArray = 0; // by default do not generate active array
}

//-----------------------------------------------------------------------------
vtkSpyPlotReader::~vtkSpyPlotReader()
{
  this->SetFileName(0);
  this->SetCurrentFileName(0);
  this->CellDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->SelectionObserver->Delete();
  this->CellDataArraySelection->Delete();
  this->Map->Clean(0);
  delete this->Map;
  this->Map = 0;
  this->SetController(0);
}

//-----------------------------------------------------------------------------
// Read the case file and the first binary file do get meta
// informations (number of files, number of fields, number of timestep).
int vtkSpyPlotReader::RequestInformation(vtkInformation *request, 
                                         vtkInformationVector **inputVector, 
                                         vtkInformationVector *outputVector)
{
  if(this->Controller==0)
    {
    vtkErrorMacro("Controller not specified. This reader requires controller to be set.");
    //return 0;
    }
  if(!this->Superclass::RequestInformation(request,inputVector,outputVector))
    {
    return 0;
    }
  
  vtkMultiGroupDataInformation *compInfo
    =vtkMultiGroupDataInformation::New();

  vtkInformation *info=outputVector->GetInformationObject(0);
  info->Set(vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION(),compInfo);

  info->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);
  compInfo->Delete();

  struct stat fs;
  if(stat(this->FileName,&fs)!=0)
    {
    vtkErrorMacro("Cannot find file " << this->FileName);
    return 0;
    }

  ifstream ifs(this->FileName);
  if(!ifs)
    {
    vtkErrorMacro("Error opening file " << this->FileName);
    return 0;
    }
  char buffer[8];
  if(!ifs.read(buffer,7))
    {
    vtkErrorMacro("Problem reading header of file: " << this->FileName);
    return 0;
    }
  buffer[7] = 0;
  ifs.close();
  if(strcmp(buffer,"spydata")==0)
    {
    vtkstd::string extension = vtksys::SystemTools::GetFilenameLastExtension(this->FileName);
    int isASeries = 1;
    size_t cc;
    if ( extension.size() > 0 )
      {
      for ( cc = ((extension[0]=='.')?1:0); cc < extension.size(); ++ cc )
        {
        if ( extension[cc] < '0' || extension[cc] > '9' )
          {
          isASeries = 0;
          break;
          }
        }
      }
    if ( isASeries )
      {
      return this->UpdateNoCaseFile(extension.c_str(), request, outputVector);
      }
    else
      {
      this->SetCurrentFileName(this->FileName);

      vtkSpyPlotReaderMap::MapOfStringToSPCTH::iterator mapIt = this->Map->Files.find(this->FileName);
      vtkSpyPlotUniReader* oldReader = 0;
      if ( mapIt != this->Map->Files.end() )
        {
        oldReader = this->Map->GetReader(mapIt, this);
        oldReader->Register(this);
        oldReader->Print(cout);
        }
      this->Map->Clean(oldReader);
      if ( oldReader )
        {
        this->Map->Files[this->FileName]=oldReader;
        oldReader->UnRegister(this);
        }
      else
        {
        this->Map->Files[this->FileName]= 0;
        vtkDebugMacro( << __LINE__ << " Create new uni reader: " << this->Map->Files[this->FileName] );
        }
      return this->UpdateMetaData(request, outputVector);
      }
    }
  else
    {
    if(strcmp(buffer,"spycase")==0)
      {
      return this->UpdateCaseFile(this->FileName, request, outputVector);
      }
    else
      {
      vtkErrorMacro("Not a SpyData file");
      return 0;
      }
    }
}

//-----------------------------------------------------------------------------
int vtkSpyPlotReader::UpdateNoCaseFile(const char *ext,
                                       vtkInformation* request, 
                                       vtkInformationVector* outputVector)
{
  // Check to see if this is the same filename as before
  // if so then I already have the meta info, so just return
  if(this->GetCurrentFileName()!=0 &&
     strcmp(this->FileName,this->GetCurrentFileName())==0)
    {
    return 1;
    }
  this->SetCurrentFileName(this->FileName);
  vtkstd::string fileNoExt = vtksys::SystemTools::GetFilenameWithoutLastExtension(this->FileName);
  vtkstd::string filePath = vtksys::SystemTools::GetFilenamePath(this->FileName);

  int currentNum = atoi(ext);

  int idx = currentNum - 100;
  int last = currentNum;
  char buffer[1024];
  int found = currentNum;
  int minimum = currentNum;
  int maximum = currentNum;
  while ( 1 )
    {
    sprintf(buffer, "%s/%s.%d",filePath.c_str(), fileNoExt.c_str(), idx);
    if ( !vtksys::SystemTools::FileExists(buffer) )
      {
      int next = idx;
      for ( idx = last; idx > next; idx -- )
        {
        sprintf(buffer, "%s/%s.%d", filePath.c_str(), fileNoExt.c_str(), idx);
        if ( !vtksys::SystemTools::FileExists(buffer) )
          {
          break;
          }
        else
          {
          found = idx;
          }
        }
      break;
      }
    last = idx;
    idx -= 100;
    }
  minimum = found;
  idx = currentNum + 100;
  last = currentNum;
  found = currentNum;
  while ( 1 )
    {
    sprintf(buffer, "%s/%s.%d", filePath.c_str(), fileNoExt.c_str(), idx);
    if ( !vtksys::SystemTools::FileExists(buffer) )
      {
      int next = idx;
      for ( idx = last; idx < next; idx ++ )
        {
        sprintf(buffer, "%s/%s.%d", filePath.c_str(), fileNoExt.c_str(), idx);
        if ( !vtksys::SystemTools::FileExists(buffer) )
          {
          break;
          }
        else
          {
          found = idx;
          }
        }
      break;
      }
    last = idx;
    idx += 100;
    }
  maximum = found;
  for ( idx = minimum; idx <= maximum; ++ idx )
    {
    sprintf(buffer, "%s/%s.%d", filePath.c_str(), fileNoExt.c_str(), idx);
    this->Map->Files[buffer]=0;
    vtkDebugMacro( << __LINE__ << " Create new uni reader: " << this->Map->Files[buffer] );
    }
  // Okay now open just the first file to get meta data
  vtkDebugMacro("Reading Meta Data in UpdateCaseFile(ExecuteInformation) from file: " << this->Map->Files.begin()->first.c_str());
  return this->UpdateMetaData(request, outputVector);
}

//-----------------------------------------------------------------------------
int vtkSpyPlotReader::UpdateCaseFile(const char *fname,
                                     vtkInformation* request, 
                                     vtkInformationVector* outputVector)
{
  // Check to see if this is the same filename as before
  // if so then I already have the meta info, so just return
  if(this->GetCurrentFileName()!=0 &&
     strcmp(fname,this->GetCurrentFileName())==0)
    {
    return 1;
    }

  // Set case file name and clean/initialize file map
  this->SetCurrentFileName(fname);
  this->Map->Clean(0);

  // Setup the filemap and spcth structures
  ifstream ifs(this->FileName);
  if (!ifs )
    {
    vtkErrorMacro("Error opening file " << fname);
    return 0;
    }
  
  vtkstd::string line;
  if(!::GetLineFromStream(ifs,line)) // eat spycase line
    {
    vtkErrorMacro("Syntax error in case file " << fname);
    return 0;
    }

  while(::GetLineFromStream(ifs, line))
    {
    if(line.length()!=0)  // Skip blank lines
      {
      vtkstd::string::size_type stp = line.find_first_not_of(" \n\t\r");
      vtkstd::string::size_type etp = line.find_last_not_of(" \n\t\r");
      vtkstd::string f(line, stp, etp-stp+1);
      if(f[0]!='#') // skip comment
        {
        if(!::FileIsFullPath(f.c_str()))
          {
          f=::GetFilenamePath(this->FileName)+"/"+f;
          }
        this->Map->Files[f.c_str()]=0;
        vtkDebugMacro( << __LINE__ << " Create new uni reader: " << this->Map->Files[f.c_str()] );
        }
      }
    }

  // Okay now open just the first file to get meta data
  vtkDebugMacro("Reading Meta Data in UpdateCaseFile(ExecuteInformation) from file: " << this->Map->Files.begin()->first.c_str());
  return this->UpdateMetaData(request, outputVector);
}

//-----------------------------------------------------------------------------
int vtkSpyPlotReader::UpdateMetaData(vtkInformation* request,
                                     vtkInformationVector* outputVector)
{
  vtkSpyPlotReaderMap::MapOfStringToSPCTH::iterator it;
  it=this->Map->Files.begin();
  
  const char *fname=0;
  vtkSpyPlotUniReader *uniReader=0;
  
  if(it==this->Map->Files.end())
    {
    vtkErrorMacro("The internal file map is empty!");
    return 0;
    }

  fname=it->first.c_str();
  uniReader=this->Map->GetReader(it, this);

  int i;
  int num_time_steps;
  
  // Open the file and get the number time steps
  uniReader->SetFileName(fname);
  uniReader->ReadInformation();
  int* timeStepRange = uniReader->GetTimeStepRange();
  num_time_steps=timeStepRange[1] + 1;
  this->TimeStepRange[1]=timeStepRange[1];
  
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), 
                 uniReader->GetTimeArray(),
                 num_time_steps);
    }

  this->CurrentTimeStep=this->TimeStep;
  if(this->CurrentTimeStep<0||this->CurrentTimeStep>=num_time_steps)
    {
    vtkErrorMacro("TimeStep set to " << this->CurrentTimeStep << " outside of the range 0 - " << (num_time_steps-1) << ". Use 0.");
    this->CurrentTimeStep=0;
    }

  // Set the reader to read the first time step.
  uniReader->SetCurrentTimeStep(this->CurrentTimeStep);

  // Print info
  vtkDebugMacro("File has " << num_time_steps << " timesteps");
  vtkDebugMacro("Timestep values:");
  for(i=0; i< num_time_steps; ++i)
    {
    vtkDebugMacro(<< i << ": " << uniReader->GetTimeFromTimeStep(i));
    }

  // AMR (hierarchy of uniform grids) or flat mesh (set of rectilinear grids)
  this->IsAMR=uniReader->IsAMR();

  // Fields
  int fieldsCount = uniReader->GetNumberOfCellFields();
  vtkDebugMacro("Number of fields: " << fieldsCount);
  
  
  vtkstd::set<vtkstd::string> fileFields;
  
  int field;
  for(field=0; field<fieldsCount; ++field)
    {
    const char*fieldName=this->CellDataArraySelection->GetArrayName(field);
    vtkDebugMacro("Field #" << field << ": " << fieldName);
    fileFields.insert(fieldName);
    }
  // Now remove the exising array that were not found in the file.
  field=0;
  // the trick is that GetNumberOfArrays() may change at each step.
  while(field<this->CellDataArraySelection->GetNumberOfArrays())
    {
    if(fileFields.find(this->CellDataArraySelection->GetArrayName(field))==fileFields.end())
      {
      this->CellDataArraySelection->RemoveArrayByIndex(field);
      }
    else
      {
      ++field;
      }
    }
  return 1;
}

// Magic number that encode the message ids for parallel communication
enum
{
  VTK_MSG_SPY_READER_HAS_BOUNDS=288302,
  VTK_MSG_SPY_READER_LOCAL_BOUNDS,
  VTK_MSG_SPY_READER_GLOBAL_BOUNDS,
  VTK_MSG_SPY_READER_LOCAL_NUMBER_OF_LEVELS,
  VTK_MSG_SPY_READER_GLOBAL_NUMBER_OF_LEVELS,
  VTK_MSG_SPY_READER_LOCAL_NUMBER_OF_DATASETS,
  VTK_MSG_SPY_READER_GLOBAL_NUMBER_OF_DATASETS,
  VTK_MSG_SPY_READER_GLOBAL_DATASETS_INDEX
};

template<class DataType>
int vtkSpyPlotRemoveBadGhostCells(DataType* dataType, vtkDataArray* dataArray, int realExtents[6], int realDims[3], int ptDims[3], int realPtDims[3])
{
  /*
    vtkDebugMacro( "//////////////////////////////////////////////////////////////////////////////////" );
    vtkDebugMacro( << dataArray << " vtkSpyPlotRemoveBadGhostCells(" << dataArray->GetName() << ")" );
    vtkDebugMacro( "DataArray: ");
    dataArray->Print(cout);
    vtkDebugMacro( "Real Extents: " << coutVector6(realExtents) );
    vtkDebugMacro( "Real Dims:    " << coutVector3(realDims) );
    vtkDebugMacro( "PT Dims:      " << coutVector3(ptDims) );
    vtkDebugMacro( "Real PT Dims: " << coutVector3(realPtDims) );
    vtkDebugMacro( "//////////////////////////////////////////////////////////////////////////////////" );
  */
  (void)* dataType;
  // skip some cell data.
  int xyz[3];
  int destXyz[3];
  xyz[2]=realExtents[4];
  destXyz[2]=0;
  DataType* dataPtr = static_cast<DataType*>(dataArray->GetVoidPointer(0));
  while(xyz[2]<realExtents[5])
    {
    destXyz[1]=0;
    xyz[1]=realExtents[2];
    while(xyz[1]<realExtents[3])
      {
      destXyz[0]=0;
      xyz[0]=realExtents[0];
      while(xyz[0]<realExtents[1])
        {
        dataPtr[vtkStructuredData::ComputeCellId(realPtDims,destXyz)] =
          dataPtr[vtkStructuredData::ComputeCellId(ptDims,xyz)];
        ++xyz[0];
        ++destXyz[0];
        }
      ++xyz[1];
      ++destXyz[1];
      }
    ++xyz[2];
    ++destXyz[2];
    }
  dataArray->SetNumberOfTuples(realDims[0]*realDims[1]*realDims[2]);
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotReader::RequestData(
  vtkInformation *request,
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkDebugMacro( "--------------------------- Request Data --------------------------------" );
  vtkSpyPlotUniReader* uniReader = 0;
  const char *fname=0;

  vtkstd::vector<vtkRectilinearGrid*> grids;

  vtkInformation *info=outputVector->GetInformationObject(0);
  vtkDataObject *doOutput=info->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET());
  vtkHierarchicalDataSet *hb=vtkHierarchicalDataSet::SafeDownCast(doOutput);  
  if(!hb)
    {
    vtkErrorMacro("The output is not a HierarchicalDataSet");
    return 0;
    }
  if (!info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) ||
      !info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
    {
    vtkErrorMacro("Expected information not found. "
                  "Cannot provide update extent.");
    return 0;
    }


  vtkMultiGroupDataInformation *compInfo=
    vtkMultiGroupDataInformation::SafeDownCast(
      info->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION()));

  hb->Initialize(); // remove all previous blocks
  hb->SetMultiGroupDataInformation(compInfo);
  
  //int numFiles = this->Map->Files.size();

  // By setting SetMaximumNumberOfPieces(-1) 
  // then GetUpdateNumberOfPieces() should always return the number
  // of processors in the parallel job and GetUpdatePiece() should
  // return the specific process number
  int processNumber = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numProcessors =info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  
  // Update the timestep.
  this->UpdateMetaData(request, outputVector);
  
  int block;
  int field;
  vtkSpyPlotBlockIterator *blockIterator;
  if(this->DistributeFiles)
    {
    vtkDebugMacro("Distribute files");
    blockIterator=new vtkSpyPlotFileDistributionBlockIterator;
    }
  else
    {
    vtkDebugMacro("Distribute blocks");
    blockIterator=new vtkSpyPlotBlockDistributionBlockIterator;
    }
  
  blockIterator->Init(numProcessors,processNumber,this,this->Map,this->CurrentTimeStep);

#if 1 // skip loading for valgrind test
  
//  double dsBounds[6]; // only for removing wrong ghost cells
  int firstBlock=1;
  
  blockIterator->Start();
  int total_num_of_blocks = blockIterator->GetNumberOfBlocksToProcess();
  int current_block_number = 1;
  int progressInterval = total_num_of_blocks / 10 + 1;
  while(!blockIterator->IsOff())
    {
    if (!(current_block_number % progressInterval))
      {
      this->UpdateProgress(
        static_cast<double>(0.2 + current_block_number)/total_num_of_blocks * 0.4);
      }
    block=blockIterator->GetBlock();
    uniReader=blockIterator->GetUniReader();
    int dims[3];
    int fixed = 0;
    uniReader->GetDataBlockDimensions(block, dims, &fixed);

    //cout  << "Dims: " << dims[0] << " " << dims[1] << " " << dims[2] << endl;
    double realBounds[6];
    
    // Compute real bounds for the current block
    if(this->IsAMR)
      {
      double bounds[6];
      fixed = 0;
      uniReader->GetDataBlockBounds(block, bounds, &fixed);

      //cout  << "Bounds " << block << ": " << bounds[0] << " " << bounds[1] << " " << bounds[2] << " " << bounds[3] << " " << bounds[4] << " " << bounds[5] << endl;
      double spacing;
      int cc=0;
      while(cc<3)
        {
        // as the file includes ghost cells,
        // the bounding box also includes ghost cells
        // there is then two extras endpoints in each axis
        if(dims[cc]>1)
          {
          spacing=(bounds[2*cc+1]-bounds[2*cc])/dims[cc];
          realBounds[2*cc]=bounds[2*cc]+spacing;
          realBounds[2*cc+1]=bounds[2*cc+1]-spacing;
          }
        else
          {
          realBounds[2*cc]=0;
          realBounds[2*cc+1]=0;
          }
        ++cc;
        }
      }
    else
      {
      vtkDataArray *coordinates[3];
      fixed =0;
      uniReader->GetDataBlockVectors(block,coordinates, &fixed);
      int cc;
      //for ( cc = 0; cc < 3; ++ cc )
      //  {
      //  cout  << "CO[" << cc << "] =";
      //  vtkIdType kk;
      //  for ( kk = 0; kk < coordinates[cc]->GetNumberOfTuples(); ++ kk )
      //    {
      //    vtkDebugMacro( " " << coordinates[cc]->GetTuple1(kk);
      //    }
      //  vtkDebugMacro( endl);
      //  }
      for ( cc = 0; cc < 3; ++ cc )
        {
        if(dims[cc]>1)
          {
          if ( fixed )
            {
            realBounds[2*cc]=coordinates[cc]->GetTuple1(0);
            realBounds[2*cc+1]=coordinates[cc]->GetTuple1(dims[cc]-2);
            }
          else
            {
            realBounds[2*cc]=coordinates[cc]->GetTuple1(1);
            realBounds[2*cc+1]=coordinates[cc]->GetTuple1(dims[cc]-1);
            }

          //cout  << "Real bounds[" << (2*cc) << "] = " << realBounds[2*cc] << endl;
          //cout  << "Real bounds[" << (2*cc+1) << "] = " << realBounds[2*cc+1] << endl;
          }
        else
          {
          realBounds[2*cc]=0;
          realBounds[2*cc+1]=0;
          }
        }
      }
    // Update the whole bounds
    // we will be use only for removing wrong ghost cells
    if(firstBlock)
      {
      int cc=0;
      while(cc<6)
        {
        this->Bounds[cc]=realBounds[cc];
        ++cc;
        }
      firstBlock=0;
      }
    else
      {
      int cc=0;
      while(cc<3)
        {
        if(realBounds[2*cc]<this->Bounds[2*cc])
          {
          this->Bounds[2*cc]=realBounds[2*cc];
          }
        if(realBounds[2*cc+1]>this->Bounds[2*cc+1])
          {
          this->Bounds[2*cc+1]=realBounds[2*cc+1];
          }
        ++cc;
        }
      }
    blockIterator->Next();
    current_block_number++;
    } // while
  
  int parent;
  int left=GetLeftChildProcessor(processNumber);
  int right=left+1;
  if(processNumber>0) // not root (nothing to do if root)
    {
    parent=this->GetParentProcessor(processNumber);
    }
  else
    {
    parent=0; // just to remove warnings, never used
    }
  
  double otherBounds[6];
  int leftHasBounds=0; // init is not useful, just for compiler warnings
  int rightHasBounds=0; // init is not useful, just for compiler warnings
  
  if ( this->Controller )
    {
    if(left<numProcessors)
      {
      // TODO WARNING if the child is empty the bounds are not initialized!
      // Grab the bounds from left child
      this->Controller->Receive(&leftHasBounds, 1, left,
        VTK_MSG_SPY_READER_HAS_BOUNDS);

      if(leftHasBounds)
        {
        this->Controller->Receive(otherBounds, 6, left,
          VTK_MSG_SPY_READER_LOCAL_BOUNDS);

        if(firstBlock) // impossible the current processor is not a leaf
          {
          int cc=0;
          while(cc<6)
            {
            this->Bounds[cc]=otherBounds[cc];
            ++cc;
            }
          firstBlock=0;
          }
        else
          {
          int cc=0;
          while(cc<3)
            {
            if(otherBounds[2*cc]<this->Bounds[2*cc])
              {
              this->Bounds[2*cc]=otherBounds[2*cc];
              }
            if(otherBounds[2*cc+1]>this->Bounds[2*cc+1])
              {
              this->Bounds[2*cc+1]=otherBounds[2*cc+1];
              }
            ++cc;
            }
          }
        }

      if(right<numProcessors)
        {
        // Grab the bounds from right child
        this->Controller->Receive(&rightHasBounds, 1, right,
          VTK_MSG_SPY_READER_HAS_BOUNDS);
        if(rightHasBounds)
          {
          this->Controller->Receive(otherBounds, 6, right,
            VTK_MSG_SPY_READER_LOCAL_BOUNDS);
          if(firstBlock)// impossible the current processor is not a leaf
            {
            int cc=0;
            while(cc<6)
              {
              this->Bounds[cc]=otherBounds[cc];
              ++cc;
              }
            firstBlock=0;
            }
          else
            {
            int cc=0;
            while(cc<3)
              {
              if(otherBounds[2*cc]<this->Bounds[2*cc])
                {
                this->Bounds[2*cc]=otherBounds[2*cc];
                }
              if(otherBounds[2*cc+1]>this->Bounds[2*cc+1])
                {
                this->Bounds[2*cc+1]=otherBounds[2*cc+1];
                }
              ++cc;
              }
            }
          }
        }
      }

    // Send local to parent, Receive global from the parent.
    if(processNumber>0) // not root (nothing to do if root)
      {
      int hasBounds=!firstBlock;
      this->Controller->Send(&hasBounds, 1, parent,
        VTK_MSG_SPY_READER_HAS_BOUNDS);
      if(hasBounds)
        {
        this->Controller->Send(this->Bounds, 6, parent,
          VTK_MSG_SPY_READER_LOCAL_BOUNDS);

        this->Controller->Receive(this->Bounds, 6, parent,
          VTK_MSG_SPY_READER_GLOBAL_BOUNDS);
        }
      }

    if(firstBlock) // empty, no bounds, nothing to do
      {
      info->Set(vtkExtractCTHPart::BOUNDS(),this->Bounds,6);
      delete blockIterator;
      return 1;
      }

    // Send it to children.
    if(left<numProcessors)
      {
      if(leftHasBounds)
        {
        this->Controller->Send(this->Bounds, 6, left,
          VTK_MSG_SPY_READER_GLOBAL_BOUNDS);
        }
      if(right<numProcessors)
        {
        if(rightHasBounds)
          {
          this->Controller->Send(this->Bounds, 6, right,
            VTK_MSG_SPY_READER_GLOBAL_BOUNDS);
          }
        }
      }
    }
  
  // At this point, the global bounds is set in each processor.
  info->Set(vtkExtractCTHPart::BOUNDS(),this->Bounds,6);
  
  //vtkDebugMacro(processNumber<<" bounds="<<this->Bounds[0]<<"; "<<this->Bounds[1]<<"; "<<this->Bounds[2]<<"; "<<this->Bounds[3]<<"; "<<this->Bounds[4]<<"; "<<this->Bounds[5]);
  
  // Read all files
  
  //vtkDebugMacro("there is (are) "<<numFiles<<" file(s)");
  // Read only the part of the file for this processNumber.
//    for ( block = startBlock; block <= endBlock; ++ block )
  
  blockIterator->Start();
  current_block_number = 1;
  while(!blockIterator->IsOff())
    {
    if ( !(current_block_number % progressInterval) )
      {
      this->UpdateProgress(0.6 + 
                           0.4 * static_cast<double>(current_block_number)/total_num_of_blocks);
      }
    block=blockIterator->GetBlock();
    int numFields=blockIterator->GetNumberOfFields();
    uniReader=blockIterator->GetUniReader();
      
    int cc;
    int dims[3];
    int fixed =0;
    uniReader->GetDataBlockDimensions(block, dims, &fixed);

    //cout  << "Dims: " << dims[0] << " " << dims[1] << " " << dims[2] << endl;
    
    int extents[6];
    cc=0;
    while(cc<3)
      {
      extents[2*cc]=0;
      if(dims[cc]==1)
        {
        extents[2*cc+1]=0;
        }
      else
        {
        extents[2*cc+1]=dims[cc];
        }
      ++cc;
      }
    vtkCellData* cd;
    double bounds[6];
      
    int hasBadGhostCells;
    int badGhostCell[6] = { 0, 0, 0, 0, 0, 0 };
    int realExtents[6];
    int realDims[3];
      
    int level;
    if(this->IsAMR)
      {
      fixed = 0;
      level = uniReader->GetDataBlockLevel(block);
      uniReader->GetDataBlockBounds(block, bounds, &fixed);

      //cout  << "Bounds " << block << ": " << bounds[0] << " " << bounds[1] << " " << bounds[2] << " " << bounds[3] << " " << bounds[4] << " " << bounds[5] << endl;
      
      // add at the end of the level list.
      vtkUniformGrid* ug = vtkUniformGrid::New();
//      vtkImageData* ug = vtkImageData::New();
      hb->SetDataSet(level, hb->GetNumberOfDataSets(level), ug);
      ug->Delete();
      
      double origin[3];
      double spacing[3];
      cc=0;
      while(cc<3)
        {
        // as the file includes ghost cells,
        // the bounding box also includes ghost cells
        // there is then two extras endpoints in each axis
        spacing[cc]=(bounds[2*cc+1]-bounds[2*cc])/dims[cc];
        // skip the first cell, which is a ghost cell
        // otherwise it should be just bounds[2*cc]
        origin[cc]=bounds[2*cc]; //+spacing[cc];
        ++cc;
        }
      ug->SetSpacing(spacing);
      
      hasBadGhostCells=0;
      cc=0;
      while(cc<3)
        {
        if(dims[cc]>1)
          {
          if(bounds[2*cc]<this->Bounds[2*cc])
            {
            realExtents[2*cc]=1;
            if ( !fixed )
              {
              --extents[2*cc+1];
              }
            origin[cc]+=spacing[cc];
            hasBadGhostCells=1;
            badGhostCell[2*cc] = 1;
            }
          else
            {
            realExtents[2*cc]=0;
            }
          if(bounds[2*cc+1]>this->Bounds[2*cc+1])
            {
            realExtents[2*cc+1]=dims[cc]-1;
            if ( !fixed )
              {
              --extents[2*cc+1];
              }
            hasBadGhostCells=1;
            badGhostCell[2*cc+1] = 1;
            }
          else
            {
            realExtents[2*cc+1]=dims[cc];
            }
          realDims[cc]=realExtents[2*cc+1]-realExtents[2*cc];
//            if(realDims[cc]==0)
//              {
//              realDims[cc]=1;
//              }
          }
        else
          {
          realExtents[2*cc]=0;
          realExtents[2*cc+1]=1;
          realDims[cc]=1;
          }
        ++cc;
        }
      vtkDebugMacro( "RealDims: " << realDims[0] << " " << realDims[1] << " " << realDims[2] );
      
      ug->SetExtent(extents);
      ug->SetOrigin(origin);

      cd = ug->GetCellData();

      }
    else
      {
      int vectorsWereFixed = 0;
      level=0;
      vtkRectilinearGrid *rg=vtkRectilinearGrid::New();
      grids.push_back(rg);
      vtkDataArray *coordinates[3];
      vtkSpyPlotUniReader::Block* blk = uniReader->GetDataBlock(block);
      
      fixed =0;
      uniReader->GetDataBlockVectors(block, coordinates, &fixed);
      vtkDebugMacro( "Vectors for block: " << block << " " << uniReader->GetFileName() );
      vtkDebugMacro( "  X: " << coordinates[0]->GetNumberOfTuples() );
      vtkDebugMacro( "  Y: " << coordinates[1]->GetNumberOfTuples() );
      vtkDebugMacro( "  Z: " << coordinates[2]->GetNumberOfTuples() );
      
      // compute real bounds (removing the ghostcells)
      // we will be use only for removing wrong ghost cells
      cc=0;
      while(cc<3)
        {
        if(dims[cc]>1)
          {
          bounds[2*cc]=coordinates[cc]->GetTuple1(0); // coordinates[cc]->GetValue(0);
          if ( coordinates[cc]->GetNumberOfTuples() > dims[cc] )
            {
            bounds[2*cc+1]=coordinates[cc]->GetTuple1(dims[cc]); // coordinates[cc]->GetValue(dims[cc]);
            }

          //cout  << "Bounds[" << (2*cc) << "] = " << bounds[2*cc] << endl;
          //cout  << "Bounds[" << (2*cc+1) << "] = " << bounds[2*cc+1] << endl;
          }
        else
          {
          bounds[2*cc]=0;
          bounds[2*cc+1]=0;
          }
        ++cc;
        }
      
      hasBadGhostCells=0;
      cc=0;
      vtkDebugMacro( << __LINE__ << " BadGhostCells:" << coutVector6(blk->RemovedBadGhostCells) );
      vtkDebugMacro( << __LINE__ << " Dims: " << coutVector3(dims) );
      vtkDebugMacro( << __LINE__ << " Bool: " << blk->VectorsFixedForGhostCells );
      while(cc<3)
        {
        if(dims[cc]>1)
          {
          if(blk->RemovedBadGhostCells[2*cc] || (bounds[2*cc]<this->Bounds[2*cc]) && !blk->VectorsFixedForGhostCells)
            {
            realExtents[2*cc]=1;
            --extents[2*cc+1];
            hasBadGhostCells=1;
            badGhostCell[2*cc] = 1;
            }
          else
            {
            realExtents[2*cc]=0;
            }
          if(blk->RemovedBadGhostCells[2*cc+1] || (bounds[2*cc+1]>this->Bounds[2*cc+1]) && !blk->VectorsFixedForGhostCells)
            {
            realExtents[2*cc+1]=dims[cc]-1;
            --extents[2*cc+1];
            hasBadGhostCells=1;
            badGhostCell[2*cc+1] = 1;
            }
          else
            {
            realExtents[2*cc+1]=dims[cc];
            }
          realDims[cc]=realExtents[2*cc+1]-realExtents[2*cc];
//            if(realDims[cc]==0)
//              {
//              realDims[cc]=1;
//              }
          }
        else
          {
          realExtents[2*cc]=0;
          realExtents[2*cc+1]=1;
          realDims[cc]=1;
          }
        ++cc;
        }
      for ( cc = 0; cc < 6; cc ++ )
        {
        blk->RemovedBadGhostCells[cc] = badGhostCell[cc];
        }
      vtkDebugMacro( << __LINE__ << " Real dims:    " << coutVector3(realDims) );
      vtkDebugMacro( << __LINE__ << " Real Extents: " << coutVector6(realExtents) );
      vtkDebugMacro( << __LINE__ << " Extents:      " << coutVector6(extents) );
      vtkDebugMacro( << __LINE__ << " Bounds:       " << coutVector6(bounds) );
      vtkDebugMacro( << __LINE__ << " Global Bounds:" << coutVector6(this->Bounds) );
      vtkDebugMacro( << __LINE__ << " BadGhostCells:" << coutVector6(blk->RemovedBadGhostCells) );
      vtkDebugMacro( << " Rectilinear grid pointer: " << rg );
      
      rg->SetExtent(extents);
      
      cc=0;
      vtkDebugMacro( "Vectors for block: " << block << " " << uniReader->GetFileName() );
      vtkDebugMacro( "  X: " << coordinates[0] << ": " << coordinates[0]->GetNumberOfTuples() );
      vtkDebugMacro( "  Y: " << coordinates[1] << ": " << coordinates[1]->GetNumberOfTuples() );
      vtkDebugMacro( "  Z: " << coordinates[2] << ": " << coordinates[2]->GetNumberOfTuples() );
      while(cc<3)
        {
        if(dims[cc]==1)
          {
          coordinates[cc]=0;
          }
        else
          {
          //vtkDebugMacro( "Has bad ghost cells: " << hasBadGhostCells );
          //vtkDebugMacro( "BGC: " << badGhostCell[0] << " " << badGhostCell[1] << " " << badGhostCell[2] << " " << badGhostCell[3] << " " << badGhostCell[4] << " " << badGhostCell[5] );
          //uniReader->GetDataBlockVectors(block, coordinates);
          //cout  << "--- Size: " << coordinates[cc]->GetNumberOfTuples() << " should be: " << realDims[cc]+1 << endl;
          //cout  << "CO[" << cc << "] =";
          //vtkIdType kk;
          //for ( kk = 0; kk < coordinates[cc]->GetNumberOfTuples(); ++ kk )
          //  {
          //  vtkDebugMacro( " " << coordinates[cc]->GetTuple1(kk);
          //  }
          //vtkDebugMacro( endl);
          if ( !fixed )
            {
            vtkDebugMacro( " Fix bad ghost cells for rg vectors" );
            if ( badGhostCell[cc*2] )
              {
              coordinates[cc]->RemoveFirstTuple();
              vectorsWereFixed = 1;
              }
            if ( badGhostCell[cc*2+1] )
              {
              coordinates[cc]->RemoveLastTuple();
              vectorsWereFixed = 1;
              }
            }
          //cout  << "--- Size: " << coordinates[cc]->GetNumberOfTuples() << " should be: " << realDims[cc]+1 << endl;
          //cout  << "CO[" << cc << "] =";
          //for ( kk = 0; kk < coordinates[cc]->GetNumberOfTuples(); ++ kk )
          //  {
          //  vtkDebugMacro( " " << coordinates[cc]->GetTuple1(kk);
          //  }
          //vtkDebugMacro( endl);
          vtkDebugMacro( "NT: " << coordinates[cc]->GetNumberOfTuples() );
          }
        ++cc;
        }
      
      if(dims[0]>1)
        {
        rg->SetXCoordinates(coordinates[0]);
        }
      if(dims[1]>1)
        {
        rg->SetYCoordinates(coordinates[1]);
        }
      if(dims[2]>1)
        {
        rg->SetZCoordinates(coordinates[2]);
        }
      vtkDebugMacro( "*******************" );
      vtkDebugMacro( "Coordinates: " );
      /*
        int cor;
        for ( cor = 0; cor < 3; cor ++ )
        {
        coordinates[cor]->Print(cout);
        }
        vtkDebugMacro( "*******************" );
      */
      hb->SetDataSet(0, hb->GetNumberOfDataSets(0), rg);
      rg->Delete();
      
      cd = rg->GetCellData();
      if ( vectorsWereFixed )
        {
        uniReader->MarkVectorsAsFixed(block);
        }
      }
    vtkDebugMacro("Executing block: " << block);
    uniReader->ReadData();
    // uniReader->PrintMemoryUsage();

    if(!hasBadGhostCells)
      {
      for ( field = 0; field < numFields; field ++ )
        {
        fname = uniReader->GetCellFieldName(field);
        if (this->CellDataArraySelection->ArrayIsEnabled(fname))
          {
          vtkDataArray *array=cd->GetArray(fname);
          if(array!=0)
            {
            cd->RemoveArray(fname); // if this is not the first step,
            // make sure we have a clean array
            }
          
          fixed = 0;
          array = uniReader->GetCellFieldData(block, field, &fixed);
          //vtkDebugMacro( << __LINE__ << " Read data block: " << block << " " << field << "  [" << array->GetName() << "]" );
          cd->AddArray(array);
          }
        }

      // Add a level array, for debugging
      if(this->GenerateLevelArray)
        {
        vtkDataArray *array=cd->GetArray("levels");
        if(array!=0)
          {
          cd->RemoveArray("levels"); // if this is not the first step,
          // make sure we have a clean array
          }
        
        array = vtkIntArray::New();
        cd->AddArray(array);
        array->Delete();
        
        array->SetName("levels");
        array->SetNumberOfComponents(1);
        int c=dims[0]*dims[1]*dims[2];
        array->SetNumberOfTuples(c);
        int *ptr=static_cast<int *>(array->GetVoidPointer(0));
        int i=0;
        while(i<c)
          {
          ptr[i]=level;
          ++i;
          }
        
        }
      // Mark the bounding cells as ghost cells of level 1.
      vtkUnsignedCharArray *ghostArray=vtkUnsignedCharArray::New();
      ghostArray->SetNumberOfTuples(dims[0]*dims[1]*dims[2]);
      ghostArray->SetName("vtkGhostLevels"); //("vtkGhostLevels");
      cd->AddArray(ghostArray);
      ghostArray->Delete();
      unsigned char *ptr =static_cast<unsigned char*>(ghostArray->GetVoidPointer(0));
      
      int k=0;
      while(k<dims[2])
        {
        int kIsGhost;
        if(dims[2]==1)
          {
          kIsGhost=0;
          }
        else
          {
          kIsGhost=k==0 || (k==dims[2]-1);
          }
        
        int j=0;
        while(j<dims[1])
          {
          int jIsGhost=kIsGhost;
          if(!jIsGhost)
            {
            if(dims[1]>1)
              {
              jIsGhost=j==0 || (j==dims[1]-1);
              }
            }
          
          int i=0;
          while(i<dims[0])
            {
            int isGhost=jIsGhost;
            if(!isGhost)
              {
              if(dims[0]>1)
                {
                isGhost=i==0 || (i==dims[0]-1);
                }
              }
            if(isGhost)
              {
              (*ptr)=1;
              }
            else
              {
              (*ptr)=0;
              }
            ++ptr;
            ++i;
            }
          ++j;
          }
        ++k;
        }
      }
    else // some bad ghost cells
      {
      int realPtDims[3];
      int ptDims[3];
      
      cc=0;
      while(cc<3)
        {
        realPtDims[cc]=realDims[cc]+1;
        ptDims[cc]=dims[cc]+1;
        ++cc;
        }
      
      for ( field = 0; field < numFields; field ++ )
        {
        fname = uniReader->GetCellFieldName(field);
        if (this->CellDataArraySelection->ArrayIsEnabled(fname))
          {
          vtkDataArray *array=cd->GetArray(fname);
          if(array!=0)
            {
            cd->RemoveArray(fname); // if this is not the first step,
            // make sure we have a clean array
            }
          
          fixed = 0;
          array = uniReader->GetCellFieldData(block, field, &fixed);
          //vtkDebugMacro( << __LINE__ << " Read data block: " << block << " " << field << "  [" << array->GetName() << "]" );
          cd->AddArray(array);
          //array->Print(cout);
          
          if ( !fixed )
            {
            vtkDebugMacro( " Fix bad ghost cells for the array: " << block << " / " << field << " (" << uniReader->GetFileName() << ")" );
            switch(array->GetDataType())
              {
              vtkTemplateMacro(
                ::vtkSpyPlotRemoveBadGhostCells(static_cast<VTK_TT*>(0), array,
                                                realExtents, realDims, ptDims, realPtDims));
              }
            uniReader->MarkCellFieldDataFixed(block, field);
            }
          else
            {
            vtkDebugMacro( " Bad ghost cells already fixed for the array" );
            }
          }
        }
      
      // Add a level array, for debugging
      if(this->GenerateLevelArray)
        {
        vtkDataArray *array=cd->GetArray("levels");
        if(array!=0)
          {
          cd->RemoveArray("levels"); // if this is not the first step,
          // make sure we have a clean array
          }
        
        array = vtkIntArray::New();
        cd->AddArray(array);
        array->Delete();
        
        array->SetName("levels");
        array->SetNumberOfComponents(1);
        int c=realDims[0]*realDims[1]*realDims[2];
        array->SetNumberOfTuples(c);
        int *ptr=static_cast<int *>(array->GetVoidPointer(0));
        int i=0;
        while(i<c)
          {
          ptr[i]=level;
          ++i;
          }
        }
      
      // Mark the remains ghost cell as real ghost cells of level 1.
      vtkUnsignedCharArray *ghostArray=vtkUnsignedCharArray::New();
      ghostArray->SetNumberOfTuples(realDims[0]*realDims[1]*realDims[2]);
      ghostArray->SetName("vtkGhostLevels"); //("vtkGhostLevels");
      cd->AddArray(ghostArray);
      ghostArray->Delete();
      unsigned char *ptr =static_cast<unsigned char*>(ghostArray->GetVoidPointer(0));
      
      int k=0;
      while(k<realDims[2])
        {
        int kIsGhost;
        if(realDims[2]==1)
          {
          kIsGhost=0;
          }
        else
          {
          kIsGhost=(k==0 && realExtents[4]==0) || (k==realDims[2]-1 && realExtents[5]==dims[2]);
          }
        
        int j=0;
        while(j<realDims[1])
          {
          int jIsGhost=kIsGhost;
          if(!jIsGhost)
            {
            if(realDims[1]>1)
              {
              jIsGhost=(j==0 && realExtents[2]==0) || (j==realDims[1]-1 && realExtents[3]==dims[1]);
              }
            }
          
          int i=0;
          while(i<realDims[0])
            {
            int isGhost=jIsGhost;
            if(!isGhost)
              {
              if(realDims[0]>1)
                {
                isGhost=(i==0 && realExtents[0]==0 ) || (i==realDims[0]-1 && realExtents[1]==dims[0]);
                }
              }
            if(isGhost)
              {
              (*ptr)=1;
              }
            else
              {
              (*ptr)=0;
              }
            ++ptr;
            ++i;
            }
          ++j;
          }
        ++k;
        }
      }
    // Add active block array, for debugging
    if (this->GenerateActiveBlockArray)
      {
      vtkSpyPlotUniReader::Block* bk = uniReader->GetDataBlock(block);
      vtkUnsignedCharArray* activeArray = vtkUnsignedCharArray::New();
      activeArray->SetName("ActiveBlock");
      vtkIdType numCells=realDims[0]*realDims[1]*realDims[2];
      activeArray->SetNumberOfTuples(numCells);
      for (vtkIdType myIdx=0; myIdx<numCells; myIdx++)
        {
        activeArray->SetValue(myIdx, bk->Active);
        }
      cd->AddArray(activeArray);
      activeArray->Delete();
      }

    //this->MergeVectors(cd);
    blockIterator->Next();
    current_block_number++;
    } // while
  delete blockIterator;
  
#endif //  if 0 skip loading for valgrind test
  // All files seem to have 1 ghost level.
//  this->AddGhostLevelArray(1);
 
  // At this point, each processor has its own blocks
  // They have to exchange the blocks they have get a unique id for
  // each block over the all dataset.
  

  // Update the number of levels.
  unsigned int numberOfLevels=hb->GetNumberOfLevels();
  
  if(this->IsAMR && this->Controller)
    {
    unsigned long ulintMsgValue;
    // Update it from the children
    if(left<numProcessors)
      {
      if(leftHasBounds)
        {
        // Grab the number of levels from left child
        this->Controller->Receive(&ulintMsgValue, 1, left,
                                  VTK_MSG_SPY_READER_LOCAL_NUMBER_OF_LEVELS);
        if(numberOfLevels<ulintMsgValue)
          {
          numberOfLevels=ulintMsgValue;
          }
        }
      if(right<numProcessors)
        {
        if(rightHasBounds)
          {
          // Grab the number of levels from right child
          this->Controller->Receive(&ulintMsgValue, 1, right,
                                    VTK_MSG_SPY_READER_LOCAL_NUMBER_OF_LEVELS);
          if(numberOfLevels<ulintMsgValue)
            {
            numberOfLevels=ulintMsgValue;
            }
          }
        }
      }
    
    ulintMsgValue=numberOfLevels;
    
    // Send local to parent, Receive global from the parent.
    if(processNumber>0) // not root (nothing to do if root)
      {
//      parent=this->GetParentProcessor(processNumber);
      this->Controller->Send(&ulintMsgValue, 1, parent,
                             VTK_MSG_SPY_READER_LOCAL_NUMBER_OF_LEVELS);
      this->Controller->Receive(&ulintMsgValue, 1, parent,
                                VTK_MSG_SPY_READER_GLOBAL_NUMBER_OF_LEVELS);
      numberOfLevels=ulintMsgValue;
      }
    
    // Send it to children.
    if(left<numProcessors)
      {
      if(leftHasBounds)
        {
        this->Controller->Send(&ulintMsgValue, 1, left,
                               VTK_MSG_SPY_READER_GLOBAL_NUMBER_OF_LEVELS);
        }
      if(right<numProcessors)
        {
        if(rightHasBounds)
          {
          this->Controller->Send(&ulintMsgValue, 1, right,
                                 VTK_MSG_SPY_READER_GLOBAL_NUMBER_OF_LEVELS);
          }
        }
      }
    
    if(numberOfLevels>hb->GetNumberOfLevels())
      {
      hb->SetNumberOfLevels(numberOfLevels);
      }
    } //  if(this->IsAMR)
  // At this point, the global number of levels is set in each processor.
  
  // Update each level
  unsigned int level=0;
  if ( this->Controller )
    {
    int intMsgValue;
    while(level<numberOfLevels)
      {
      int numberOfDataSets=hb->GetNumberOfDataSets(level);
      int totalNumberOfDataSets=numberOfDataSets;
      int leftNumberOfDataSets=0;
      int rightNumberOfDataSets=0;
      // Get number of dataset of each child
      if(left<numProcessors)
        {
        if(leftHasBounds)
          {
          // Grab info the number of datasets from left child
          this->Controller->Receive(&intMsgValue, 1, left,
            VTK_MSG_SPY_READER_LOCAL_NUMBER_OF_DATASETS);
          leftNumberOfDataSets=intMsgValue;
          }
        if(right<numProcessors)
          {
          if(rightHasBounds)
            {
            // Grab info the number of datasets from right child
            this->Controller->Receive(&intMsgValue, 1, right,
              VTK_MSG_SPY_READER_LOCAL_NUMBER_OF_DATASETS);
            rightNumberOfDataSets=intMsgValue;
            }
          }
        }

      int globalIndex;
      if(processNumber==0) // root
        {
        totalNumberOfDataSets=numberOfDataSets+leftNumberOfDataSets
          +rightNumberOfDataSets;
        globalIndex=0;
        }
      else
        {
        // Send local to parent, Receive global from the parent.
        intMsgValue=numberOfDataSets+leftNumberOfDataSets+rightNumberOfDataSets;
        this->Controller->Send(&intMsgValue, 1, parent,
          VTK_MSG_SPY_READER_LOCAL_NUMBER_OF_DATASETS);
        this->Controller->Receive(&intMsgValue, 1, parent,
          VTK_MSG_SPY_READER_GLOBAL_NUMBER_OF_DATASETS);
        totalNumberOfDataSets=intMsgValue;
        this->Controller->Receive(&intMsgValue, 1, parent,
          VTK_MSG_SPY_READER_GLOBAL_DATASETS_INDEX);
        globalIndex=intMsgValue;
        }

      // Send it to children.
      if(left<numProcessors)
        {
        if(leftHasBounds)
          {
          intMsgValue=totalNumberOfDataSets;
          this->Controller->Send(&intMsgValue, 1, left,
            VTK_MSG_SPY_READER_GLOBAL_NUMBER_OF_DATASETS);
          intMsgValue=globalIndex+numberOfDataSets;
          this->Controller->Send(&intMsgValue, 1, left,
            VTK_MSG_SPY_READER_GLOBAL_DATASETS_INDEX);
          }
        if(right<numProcessors)
          {
          if(rightHasBounds)
            {
            intMsgValue=totalNumberOfDataSets;
            this->Controller->Send(&intMsgValue, 1, right,
              VTK_MSG_SPY_READER_GLOBAL_NUMBER_OF_DATASETS);
            intMsgValue=globalIndex+numberOfDataSets+leftNumberOfDataSets;
            this->Controller->Send(&intMsgValue, 1, right,
              VTK_MSG_SPY_READER_GLOBAL_DATASETS_INDEX);
            }
          }
        }

      // Update the level.
      if(totalNumberOfDataSets>numberOfDataSets)
        {
        hb->SetNumberOfDataSets(level,totalNumberOfDataSets);
        int i;
        if(globalIndex>0)
          {
          // move the datasets to the right indices
          // we have to start at the end because the
          // original blocks location and final location
          // may overlap.
          i=numberOfDataSets-1;
          int j=globalIndex+numberOfDataSets-1;
          while(i>=0)
            {
            hb->SetDataSet(level,j,hb->GetDataSet(level,i));
            --i;
            --j;
            }
          // add null pointers at the beginning
          i=0;
          while(i<globalIndex)
            {
            hb->SetDataSet(level,i,0);
            ++i;
            }
          }
        // add null pointers at the end
        i=globalIndex+numberOfDataSets;
        while(i<totalNumberOfDataSets)
          {
          hb->SetDataSet(level,i,0);
          ++i;
          }
        }
      ++level;
      }
    }
  
  // Set the unique block id cell data
  if(this->GenerateBlockIdArray)
    {
    int blockId=0;
    level=0;
    numberOfLevels=hb->GetNumberOfLevels();
    while(level<numberOfLevels)
      {
      int totalNumberOfDataSets=hb->GetNumberOfDataSets(level);
      int i=0;
      while(i<totalNumberOfDataSets)
        {
        vtkDataObject *dataObject=hb->GetDataSet(level,i);
        if(dataObject!=0)
          {
          vtkDataSet *ds=vtkDataSet::SafeDownCast(dataObject);
          assert("check: ds_exists" && ds!=0);
          vtkCellData *cd=ds->GetCellData();
          // Add the block id cell data array
          
          vtkDataArray *array=cd->GetArray("blockId");
          if(array!=0)
            {
            cd->RemoveArray("blockId"); // if this is not the first step,
            // make sure we have a clean array
            }
          
          array = vtkIntArray::New();
          cd->AddArray(array);
          array->Delete();
        
          array->SetName("blockId");
          array->SetNumberOfComponents(1);
          int c=ds->GetNumberOfCells();
          array->SetNumberOfTuples(c);
          int *ptr=static_cast<int *>(array->GetVoidPointer(0));
          int k=0;
          while(k<c)
            {
            ptr[k]=blockId;
            ++k;
            }
          }
        ++i;
        ++blockId;
        }
      ++level;
      }
    }
  
#if 0
  //  Display the block list for each level
  
  level=0;
  numberOfLevels=hb->GetNumberOfLevels();
  while(level<numberOfLevels)
    {
    cout<<processNumber<<" level="<<level<<"/"<<numberOfLevels<<endl;
    int totalNumberOfDataSets=hb->GetNumberOfDataSets(level);
    int i=0;
    while(i<totalNumberOfDataSets)
      {
      cout<<processNumber<<" dataset="<<i<<"/"<<totalNumberOfDataSets;
      if(hb->GetDataSet(level,i)==0)
        {
        cout<<" Void"<<endl;
        }
      else
        {
        cout<<" Exists"<<endl;
        }
      ++i;
      }
    ++level;
    }
#endif
  
  /*
    vtkstd::vector<vtkRectilinearGrid*>::iterator it;
    for ( it = grids.begin(); it != grids.end(); ++ it )
    {
    (*it)->Print(cout);
    int cc;
    for ( cc = 0; cc < (*it)->GetCellData()->GetNumberOfArrays(); ++ cc )
    {
    (*it)->GetCellData()->GetArray(cc)->Print(cout);
    }
    }
  */
  
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSpyPlotReader::AddGhostLevelArray(int numLevels)
{
  (void)numLevels;
  /*
    int numCells = output->GetNumberOfCells();
    int numBlocks = output->GetNumberOfBlocks();
    vtkUnsignedCharArray* array = vtkUnsignedCharArray::New();
    int blockId;
    int dims[3];
    int i, j, k;
    unsigned char* ptr;
    int iLevel, jLevel, kLevel, tmp;

    output->SetNumberOfGhostLevels(numLevels);
    array->SetNumberOfTuples(numCells);
    ptr = (unsigned char*)(array->GetVoidPointer(0));


    for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    output->GetBlockCellDimensions(blockId, dims);    
    for (k = 0; k < dims[2]; ++k)
    {
    kLevel = numLevels - k;
    tmp = k - dims[2] + 1 + numLevels;
    if (tmp > kLevel) { kLevel = tmp;}
    if (dims[2] == 1) {kLevel = 0;}
    for (j = 0; j < dims[1]; ++j)
    {
    jLevel = kLevel;
    tmp = numLevels - j;
    if (tmp > jLevel) { jLevel = tmp;}
    tmp = j - dims[1] + 1 + numLevels;
    if (tmp > jLevel) { jLevel = tmp;}
    if (dims[1] == 1) {jLevel = 0;}
    for (i = 0; i < dims[0]; ++i)
    {
    iLevel = jLevel;
    tmp = numLevels - i;
    if (tmp > iLevel) { iLevel = tmp;}
    tmp = i - dims[0] + 1 + numLevels;
    if (tmp > iLevel) { iLevel = tmp;}
    if (dims[0] == 1) {iLevel = 0;}
    if (iLevel <= 0)
    {
    *ptr = 0;
    }
    else
    {
    *ptr = iLevel;
    }
    ++ptr;
    }
    }
    }
    }

    //array->SetName("Test");
    array->SetName("vtkGhostLevels");
    output->GetCellData()->AddArray(array);
    array->Delete();
  */
}

//----------------------------------------------------------------------------
void vtkSpyPlotReader::SelectionModifiedCallback(vtkObject*, unsigned long,
                                                 void* clientdata, void*)
{
  static_cast<vtkSpyPlotReader*>(clientdata)->Modified();
}
//-----------------------------------------------------------------------------
int vtkSpyPlotReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkSpyPlotReader::GetCellArrayName(int index)
{
  return this->CellDataArraySelection->GetArrayName(index);
}

//-----------------------------------------------------------------------------
int vtkSpyPlotReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}

//-----------------------------------------------------------------------------
void vtkSpyPlotReader::SetCellArrayStatus(const char* name, int status)
{
  vtkDebugMacro("Set cell array \"" << name << "\" status to: " << status);
  if(status)
    {
    this->CellDataArraySelection->EnableArray(name);
    }
  else
    {
    this->CellDataArraySelection->DisableArray(name);
    }
}

//-----------------------------------------------------------------------------
void vtkSpyPlotReader::MergeVectors(vtkDataSetAttributes* da)
{
  int numArrays = da->GetNumberOfArrays();
  int idx;
  vtkDataArray *a1, *a2, *a3;
  int flag = 1;

  // Loop merging arrays.
  // Since we are modifying the list of arrays that we are traversing,
  // merge one set of arrays at a time.
  while (flag)
    {
    flag = 0;  
    for (idx = 0; idx < numArrays-1 && !flag; ++idx)
      {
      a1 = da->GetArray(idx);
      a2 = da->GetArray(idx+1);
      if (idx+2 < numArrays)
        {
        a3 = da->GetArray(idx+2);
        if (this->MergeVectors(da, a1, a2, a3))
          {
          flag = 1;
          continue;
          }    
        if (this->MergeVectors(da, a3, a2, a1))
          {
          flag = 1;
          continue;
          }
        }
      if (this->MergeVectors(da, a1, a2))
        {
        flag = 1;
        continue;
        }
      if (this->MergeVectors(da, a2, a1))
        {
        flag = 1;
        continue;
        }
      }
    }
}

//-----------------------------------------------------------------------------
// The templated execute function handles all the data types.
template <class T>
void vtkMergeVectorComponents(vtkIdType length,
                              T *p1, T *p2, T *p3, T *po)
{
  vtkIdType idx;
  if (p3)
    {
    for (idx = 0; idx < length; ++idx)
      {
      *po++ = *p1++;
      *po++ = *p2++;
      *po++ = *p3++;
      }
    }
  else
    {
    for (idx = 0; idx < length; ++idx)
      {
      *po++ = *p1++;
      *po++ = *p2++;
      *po++ = (T)0;
      }
    }
}

//-----------------------------------------------------------------------------
int vtkSpyPlotReader::MergeVectors(vtkDataSetAttributes* da, 
                                   vtkDataArray * a1, vtkDataArray * a2, vtkDataArray * a3)
{
  int prefixFlag = 0;

  if (a1 == 0 || a2 == 0 || a3 == 0)
    {
    return 0;
    }
  if(a1->GetNumberOfTuples() != a2->GetNumberOfTuples() ||
     a1->GetNumberOfTuples() != a3->GetNumberOfTuples())
    { // Sanity check.  Should never happen.
    return 0;
    }
  if(a1->GetDataType()!=a2->GetDataType()||a1->GetDataType()!=a3->GetDataType())
    {
    return 0;
    }
  if(a1->GetNumberOfComponents()!=1 || a2->GetNumberOfComponents()!=1 ||
     a3->GetNumberOfComponents()!=1)
    {
    return 0;
    }
  const char *n1, *n2, *n3;
  int e1, e2, e3;
  n1 = a1->GetName();
  n2 = a2->GetName();
  n3 = a3->GetName();
  if (n1 == 0 || n2 == 0 || n3 == 0)
    {
    return 0;
    }  
  e1 = strlen(n1)-1;
  e2 = strlen(n2)-1;
  e3 = strlen(n3)-1;
  if(e1!=e2 || e1 != e3)
    {
    return 0;
    }
  if (strncmp(n1+1,n2+1,e1)==0 && strncmp(n1+1,n3+1,e1)==0)
    { // Trailing characters are the same. Check for prefix XYZ.
    if ((n1[0]!='X' || n2[0]!='Y' || n3[0]!='Z') &&
        (n1[0]!='x' || n2[0]!='x' || n3[0]!='x'))
      { // Since last characters are the same, there is no way
      // the names can have postfix XYZ.
      return 0;
      }
    prefixFlag = 1;
    }
  else
    { // Check for postfix case.
    if (strncmp(n1,n2,e1)!=0 || strncmp(n1,n3,e1)!=0)
      { // Not pre or postfix.
      return 0;
      }
    if ((n1[e1]!='X' || n2[e2]!='Y' || n3[e3]!='Z') &&
        (n1[e1]!='x' || n2[e2]!='x' || n3[e3]!='x'))
      { // Tails are the same, but postfix not XYZ.
      return 0;
      }
    }
  // Merge the arrays.
  vtkDataArray* newArray = a1->NewInstance();
  newArray->SetNumberOfComponents(3);
  vtkIdType length = a1->GetNumberOfTuples();
  newArray->SetNumberOfTuples(length);
  void *p1 = a1->GetVoidPointer(0);
  void *p2 = a2->GetVoidPointer(0);
  void *p3 = a3->GetVoidPointer(0);
  void *pn = newArray->GetVoidPointer(0);
  switch (a1->GetDataType())
    {
    vtkTemplateMacro(
      vtkMergeVectorComponents( length, 
                                (VTK_TT*)p1,
                                (VTK_TT*)p2,
                                (VTK_TT*)p3,
                                (VTK_TT*)pn));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return 0;
    }
  if (prefixFlag)
    {
    newArray->SetName(n1+1);
    }
  else
    {
    char* name = new char[e1+2];
    strncpy(name,n1,e1);
    name[e1] = '\0';
    newArray->SetName(name);
    delete [] name;
    }
  da->RemoveArray(n1);    
  da->RemoveArray(n2);    
  da->RemoveArray(n3);
  da->AddArray(newArray);
  newArray->Delete();    
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotReader::MergeVectors(vtkDataSetAttributes* da, 
                                   vtkDataArray * a1, vtkDataArray * a2)
{
  int prefixFlag = 0;

  if (a1 == 0 || a2 == 0)
    {
    return 0;
    }
  if (a1->GetNumberOfTuples() != a2->GetNumberOfTuples())
    { // Sanity check.  Should never happen.
    return 0;
    }
  if (a1->GetDataType() != a2->GetDataType())
    {
    return 0;
    }
  if (a1->GetNumberOfComponents() != 1 || a2->GetNumberOfComponents() != 1)
    {
    return 0;
    }
  const char *n1, *n2;
  int e1, e2;
  n1 = a1->GetName();
  n2 = a2->GetName();
  if (n1 == 0 || n2 == 0)
    {
    return 0;
    }  
  e1 = strlen(n1)-1;
  e2 = strlen(n2)-1;
  if (e1 != e2 )
    {
    return 0;
    }
  if ( strncmp(n1+1,n2+1,e1) == 0)
    { // Trailing characters are the same. Check for prefix XYZ.
    if ((n1[0]!='X' || n2[0]!='Y') && (n1[0]!='x' || n2[0]!='x'))
      { // Since last characters are the same, there is no way
      // the names can have postfix XYZ.
      return 0;
      }
    prefixFlag = 1;
    }
  else
    { // Check for postfix case.
    if (strncmp(n1,n2,e1) != 0)
      { // Not pre or postfix.
      return 0;
      }
    if ((n1[e1]!='X' || n2[e2]!='Y') && (n1[e1]!='x' || n2[e2]!='x'))
      { // Tails are the same, but postfix not XYZ.
      return 0;
      }
    }
  // Merge the arrays.
  vtkDataArray* newArray = a1->NewInstance();
  // Creae the third componnt and set to 0.
  newArray->SetNumberOfComponents(3);
  vtkIdType length = a1->GetNumberOfTuples();
  newArray->SetNumberOfTuples(length);
  void *p1 = a1->GetVoidPointer(0);
  void *p2 = a2->GetVoidPointer(0);
  void *pn = newArray->GetVoidPointer(0);
  switch (a1->GetDataType())
    {
    vtkTemplateMacro(
      vtkMergeVectorComponents( length, 
                                (VTK_TT*)p1,
                                (VTK_TT*)p2,
                                (VTK_TT*)0,
                                (VTK_TT*)pn));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return 0;
    }
  if (prefixFlag)
    {
    newArray->SetName(n1+1);
    }
  else
    {
    char* name = new char[e1+2];
    strncpy(name,n1,e1);
    name[e1] = '\0';
    newArray->SetName(name);
    delete [] name;
    }
  da->RemoveArray(n1);    
  da->RemoveArray(n2);    
  da->AddArray(newArray);
  newArray->Delete();    
  return 1;
}

// The processors are views as a heap tree. The root is the processor of
// id 0.
//-----------------------------------------------------------------------------
int vtkSpyPlotReader::GetParentProcessor(int proc)
{
  int result;
  if(proc%2==1)
    {
    result=proc>>1; // /2
    }
  else
    {
    result=(proc-1)>>1; // /2
    }
  return result;
}

int vtkSpyPlotReader::GetLeftChildProcessor(int proc)
{
  return (proc<<1)+1; // *2+1
}

//-----------------------------------------------------------------------------
int vtkSpyPlotReader::CanReadFile(const char* fname)
{
  ifstream ifs(fname, ios::binary|ios::in);
  if ( !ifs )
    {
    return 0;
    }
  char magic[8];
  if ( !::vtkSpyPlotReadString(ifs, magic, 8) )
    {
    vtkErrorMacro( "Cannot read magic" );
    return 0;
    }
  if ( strncmp(magic, "spydata", 7) != 0 &&
       strncmp(magic, "spycase", 7) != 0 )
    {
    return 0;
    }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSpyPlotReader::SetDownConvertVolumeFraction(int vf)
{
  if ( vf == this->DownConvertVolumeFraction )
    {
    return;
    }
  vtkSpyPlotReaderMap::MapOfStringToSPCTH::iterator mapIt;
  for ( mapIt = this->Map->Files.begin();
        mapIt != this->Map->Files.end();
        ++ mapIt )
    {
    this->Map->GetReader(mapIt, this)->SetDownConvertVolumeFraction(vf);
    }
  this->DownConvertVolumeFraction = vf;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSpyPlotReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << "FileName: " << (this->FileName?this->FileName:"(none)") << endl;
  os << "DistributeFiles: ";
  if(this->DistributeFiles)
    {
    os << "true"<<endl;
    }
  else
    {
    os << "false"<<endl;
    }
  
  os << "DownConvertVolumeFraction: ";
  if(this->DownConvertVolumeFraction)
    {
    os << "true"<<endl;
    }
  else
    {
    os << "false"<<endl;
    }

  os << "GenerateLevelArray: ";
  if(this->GenerateLevelArray)
    {
    os << "true"<<endl;
    }
  else
    {
    os << "false"<<endl;
    }
  
  os << "GenerateBlockIdArray: ";
  if(this->GenerateBlockIdArray)
    {
    os << "true"<<endl;
    }
  else
    {
    os << "false"<<endl;
    }

  os << "GenerateActiveBlockArray: ";
  if(this->GenerateActiveBlockArray)
    {
    os << "true"<<endl;
    }
  else
    {
    os << "false"<<endl;
    }
  
  os << "TimeStep: " << this->TimeStep << endl;
  os << "TimeStepRange: " << this->TimeStepRange[0] << " " << this->TimeStepRange[1] << endl;
  if ( this->CellDataArraySelection )
    {
    os << "CellDataArraySelection:" << endl;
    this->CellDataArraySelection->PrintSelf(os, indent.GetNextIndent());
    }
  if ( this->Controller)
    {
    os << "Controller:" << endl;
    this->Controller->PrintSelf(os, indent.GetNextIndent());
    }
}
