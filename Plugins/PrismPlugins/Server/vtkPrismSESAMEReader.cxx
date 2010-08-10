/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
#include "vtkPrismSESAMEReader.h"

#include <vtkFloatArray.h>
#include <vtkIntArray.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkRectilinearGrid.h>
#include <vtkstd/vector>
#include <vtkstd/string>
#include <vtkStringArray.h>
#include <vtkSmartPointer.h>

vtkStandardNewMacro(vtkPrismSESAMEReader);

static const int SESAME_NUM_CHARS = 512;
static const char* TableLineFormat = "%2i%6i%6i";

class vtkPrismSESAMEReader::MyInternal
{
public:
  vtkstd::string FileName;
  FILE* File;
  vtkstd::vector<int> TableIds;
  vtkstd::vector<long> TableLocations;
  vtkIdType TableId;
  vtkstd::vector<vtkstd::string> TableArrays;
  vtkstd::vector<int> TableArrayStatus;
  vtkIntArray* TableIdsArray;

  vtkstd::string TableXAxisName;
  vtkstd::string TableYAxisName;

  void ClearTables()
    {
    this->TableIds.clear();
    this->TableId = -1;
    this->TableIdsArray->Initialize();
    this->ClearArrays();
    }
  void ClearArrays()
    {
      this->TableArrays.clear();
      this->TableArrayStatus.clear();
      this->TableXAxisName.clear();
      this->TableYAxisName.clear();
    }
  MyInternal()
    {
    this->File = NULL;
    this->TableId = -1;
    this->TableIdsArray = vtkIntArray::New();
    }
  ~MyInternal()
    {
    this->TableIdsArray->Delete();
    }
};

// structures to hold information about SESAME files
static const int MaxTableArrays = 10;
struct vtkPrismSESAMETableDef
{
  int TableId;
  const char* XAxisName;
  const char* YAxisName;
  const char* Arrays[MaxTableArrays];
};

static const vtkPrismSESAMETableDef TableDefs[] =
{
    {301,
       "Density",
       "Temperature",
      {"301: Total EOS (Pressure)",
       "301: Total EOS (Energy)",
       "301: Total EOS (Free Energy)",
      0}  // keep 0 last
    },

    {303,
       "Density",
       "Temperature",
      {"303: Total EOS (Pressure)",
       "303: Total EOS (Energy)",
       "303: Total EOS (Free Energy)",
      0}  // keep 0 last
    },

    {304,
       "Density",
       "Temperature",
      {"304: Electron EOS (Pressure)",
       "304: Electron EOS (Energy)",
       "304: Electron EOS (Free Energy)",
       0}  // keep 0 last
    },

    {305,
       "Density",
       "Temperature",
      {"305: Total EOS (Pressure)",
       "305: Total EOS (Energy)",
       "305: Total EOS (Free Energy)",
      0}  // keep 0 last
    },

    {306,
       "Density",
       "Temperature",
      {"306: Total EOS (Pressure)",
       "306: Total EOS (Energy)",
       "306: Total EOS (Free Energy)",
      0}  // keep 0 last
    },

    {502,
       "Density",
       "Temperature",
      {"502: Rosseland Mean Opacity",
       0}  // keep 0 last
    },

    {503,
       "Density",
       "Temperature",
      {"503: Electron Conductive Opacity1",
       0}  // keep 0 last
    },

    {504,
       "Density",
       "Temperature",
      {"504: Mean Ion Charge1",
       0}  // keep 0 last
    },

    {505,
       "Density",
       "Temperature",
      {"505: Planck Mean Opacity",
       0}  // keep 0 last
    },

    {601,
       "Density",
       "Temperature",
      {"601: Mean Ion Charge2",
       0}  // keep 0 last
    },

    {602,
       "Density",
       "Temperature",
      {"602: Electrical Conductivity",
       0}  // keep 0 last
    },

    {603,
       "Density",
       "Temperature",
      {"603: Thermal Conductivity",
       0}  // keep 0 last
    },

    {604,
       "Density",
       "Temperature",
      {"604: Thermoelectric Coefficient",
       0}  // keep 0 last
    },

    {605,
       "Density",
       "Temperature",
    {"605: Electron Conductive Opacity2",
    0}  // keep 0 last
    }

};

static int TableIndex(int tableId)
{
  // check that we got a valid table id
  for(unsigned int i=0; i<sizeof(TableDefs)/sizeof(vtkPrismSESAMETableDef); i++)
    {
    if(tableId == TableDefs[i].TableId)
      {
      return i;
      }
    }
  return -1;
}


vtkPrismSESAMEReader::vtkPrismSESAMEReader() : vtkRectilinearGridSource()
{
  this->Internal = new MyInternal();
}

vtkPrismSESAMEReader::~vtkPrismSESAMEReader()
{
  this->CloseFile();
  delete this->Internal;
}

int vtkPrismSESAMEReader::IsValidFile()
{
  if(this->Internal->FileName.empty())
    {
    return 0;
    }

  // open the file
  FILE* f = fopen(this->GetFileName(), "rb");
  if(!f)
    {
    return 0;
    }

  // check that it is valid
  int a,b,c;
  int ret = fscanf(f, TableLineFormat, &a,&b,&c);
  fclose(f);
  if(ret != 3)
    {
    return 0;
    }
  return 1;
}

void vtkPrismSESAMEReader::SetFileName(const char* file)
{
  if(this->Internal->FileName == file)
    {
    return;
    }

  this->Internal->FileName = file;

  // clean out possible data from last file
  this->Internal->ClearTables();
  this->CloseFile();
  this->Modified();
}

const char* vtkPrismSESAMEReader::GetFileName()
{
  return this->Internal->FileName.c_str();
}

int vtkPrismSESAMEReader::OpenFile()
{
  //already open
  if(this->Internal->File)
    {
    return 1;
    }

  if(this->Internal->FileName.empty())
    {
    return 0;
    }

  // open the file
  this->Internal->File = fopen(this->GetFileName(), "rb");
  if(!this->Internal->File)
    {
    vtkErrorMacro(<<"Unable to open file " << this->GetFileName());
    return 0;
    }

  // check that it is valid
  int a,b,c;
  int ret = fscanf(this->Internal->File, TableLineFormat, &a,&b,&c);
  rewind(this->Internal->File);
  if(ret != 3)
    {
    vtkErrorMacro(<<this->GetFileName() << " is not a valid SESAME file");
    fclose(this->Internal->File);
    this->Internal->File = NULL;
    return 0;
    }
  return 1;
}

void vtkPrismSESAMEReader::CloseFile()
{
  if(this->Internal->File)
    {
    fclose(this->Internal->File);
    this->Internal->File = NULL;
    }
}

int vtkPrismSESAMEReader::GetNumberOfTableIds()
{
  this->ExecuteInformation();
  return static_cast<int>(this->Internal->TableIds.size());
}

int* vtkPrismSESAMEReader::GetTableIds()
{
  this->ExecuteInformation();
  return &this->Internal->TableIds[0];
}

vtkIntArray* vtkPrismSESAMEReader::GetTableIdsAsArray()
{
  this->Internal->TableIdsArray->Initialize();
  this->Internal->TableIdsArray->SetNumberOfComponents(1);
  this->ExecuteInformation();
  int numTableIds = static_cast<int>(this->Internal->TableIds.size());
  for (int i=0; i < numTableIds; ++i)
    {
    this->Internal->TableIdsArray->InsertNextValue(
      this->Internal->TableIds[i]);
    }
  return this->Internal->TableIdsArray;
}

void vtkPrismSESAMEReader::SetTable(int tableId)
{
  if(this->Internal->TableId != tableId)
    {
    if(TableIndex(tableId) != -1)
      {
      this->Internal->TableId = tableId;

      // clean out info about the previous table
      this->Internal->ClearArrays();
      this->Modified();
      }
    }
}

int vtkPrismSESAMEReader::GetTable()
{
  this->ExecuteInformation();
  return this->Internal->TableId;
}

int vtkPrismSESAMEReader::GetNumberOfTableArrayNames()
{
  this->ExecuteInformation();
  return static_cast<int>(this->Internal->TableArrays.size());
}

const char* vtkPrismSESAMEReader::GetTableArrayName(int index)
{
  this->ExecuteInformation();
  int s = static_cast<int>(this->Internal->TableArrays.size());
  if(s > index)
    {
    return this->Internal->TableArrays[index].c_str();
    }
  return NULL;
}
const char* vtkPrismSESAMEReader::GetTableXAxisName()
{
  this->ExecuteInformation();
    return this->Internal->TableXAxisName.c_str();
}
const char* vtkPrismSESAMEReader::GetTableYAxisName()
{
  this->ExecuteInformation();
    return this->Internal->TableYAxisName.c_str();
}

void vtkPrismSESAMEReader::SetTableArrayStatus(const char* name, int flag)
{
  int i, numArrays;
  numArrays = static_cast<int>(this->Internal->TableArrays.size());
  for(i=0; i<numArrays; i++)
    {
    if(this->Internal->TableArrays[i] == name)
      {
      this->Internal->TableArrayStatus[i] = flag;
      this->Modified();
      }
    }
}

int vtkPrismSESAMEReader::GetTableArrayStatus(const char* name)
{
  this->ExecuteInformation();
  int i, numArrays;
  numArrays = static_cast<int>(this->Internal->TableArrays.size());
  for(i=0; i<numArrays; i++)
    {
    if(this->Internal->TableArrays[i], name)
      {
      return this->Internal->TableArrayStatus[i];
      }
    }
  return 0;
}


void vtkPrismSESAMEReader::ExecuteInformation()
{
  // open the file
  if(!this->OpenFile())
    {
    return;
    }

  if(this->Internal->TableIds.empty())
    {
    this->Internal->TableLocations.clear();

    // get the table ids

    char buffer[SESAME_NUM_CHARS];
    int dummy;
    int internalId;
    int tableId;

    // read lines from the file the whole file
    while( fgets(buffer, SESAME_NUM_CHARS, this->Internal->File) != NULL )
      {
      // see if the line matches the  " 0 9999 602" format
      if(sscanf(buffer, TableLineFormat, &dummy, &internalId, &tableId) == 3)
        {
        if(TableIndex(tableId) != -1)
          {
          this->Internal->TableIds.push_back(tableId);
          long loc = ftell(this->Internal->File);
          this->Internal->TableLocations.push_back(loc);
          }
        }
      }
    }

  //if(this->Internal->TableId == -1 &&
  //   !this->Internal->TableIds.empty())
  //  {
  //  this->Internal->TableId = this->Internal->TableIds[0];
  //  }

  if(this->Internal->TableId != -1)
    {
    JumpToTable(this->Internal->TableId);
    float v[5] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
    if ( ReadTableValueLine( &(v[0]), &(v[1]),
                             &(v[2]), &(v[3]), &(v[4]) ) != 0)
      {
      // first two values are dimensions of
      // grid
      this->GetOutput()->SetWholeExtent(0, (int)(v[0]) - 1,
                                        0, (int)(v[1]) - 1, 0, 0 );
      }
    }

  if(this->Internal->TableId != -1 &&
     this->Internal->TableArrays.empty())
    {
    // get the names of the arrays in the table
    int tableIndex = TableIndex(this->Internal->TableId);

    this->Internal->TableXAxisName=TableDefs[tableIndex].XAxisName;
    this->Internal->TableYAxisName=TableDefs[tableIndex].YAxisName;
    for(int j=0; TableDefs[tableIndex].Arrays[j] != 0; j++)
      {
      this->Internal->TableArrays.push_back(
                TableDefs[tableIndex].Arrays[j]);
      this->Internal->TableArrayStatus.push_back(1);  // all arrays are on
                                                      // by default
      }
    }

}

int vtkPrismSESAMEReader::JumpToTable( int toTable )
{
  int numIds = static_cast<int>(this->Internal->TableIds.size());
  for(int i=0; i<numIds; i++)
    {
    if(this->Internal->TableIds[i] == toTable)
      {
      fseek(this->Internal->File, this->Internal->TableLocations[i], SEEK_SET);
      return 1;
      }
    }

  return 0;
}

void vtkPrismSESAMEReader::Execute()
{
  // read the file
  JumpToTable(this->Internal->TableId);
  this->ReadTable();
}

void vtkPrismSESAMEReader::ReadTable()
{
  vtkFloatArray *xCoords = vtkFloatArray::New();
  vtkFloatArray *yCoords = vtkFloatArray::New();
  vtkFloatArray *zCoords = vtkFloatArray::New();

  vtkRectilinearGrid *output = this->GetOutput();

  float v[5] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
  int datadims[2] = { 0, 0 };
  int numRead = 0;
  int result=0;

  result=ReadTableValueLine( &(v[0]), &(v[1]), &(v[2]), &(v[3]), &(v[4]) );
  // get the table header
  if (result!= 0)
    {
    // dimensions of grid
    datadims[0] = (int)(v[0]);
    datadims[1] = (int)(v[1]);
    output->SetDimensions( datadims[0], datadims[1], 1 );

    // allocate space
    xCoords->Allocate( datadims[0] );
    yCoords->Allocate( datadims[1] );
    zCoords->Allocate( 1 );
    zCoords->InsertNextTuple1( 0.0 );

    // the first three values are x samples Update: this only works if X has at least 3 values.
    //xCoords->InsertNextTuple1( v[2] );
    //xCoords->InsertNextTuple1( v[3] );
    //xCoords->InsertNextTuple1( v[4] );
    //numRead = 3;
    }




  vtkSmartPointer<vtkStringArray> xName=vtkSmartPointer<vtkStringArray>::New();
  xName->SetName("XAxisName");
  xName->InsertNextValue(this->Internal->TableXAxisName);
  vtkSmartPointer<vtkStringArray> yName=vtkSmartPointer<vtkStringArray>::New();
  yName->SetName("YAxisName");
  yName->InsertNextValue(this->Internal->TableYAxisName);

  unsigned int i;
  vtkstd::vector<vtkFloatArray*> scalars;
  for(i=0; i<this->Internal->TableArrayStatus.size(); i++)
    {
    vtkFloatArray* newArray = this->Internal->TableArrayStatus[i] ?
                      vtkFloatArray::New() : NULL;
    scalars.push_back(newArray);
    if(newArray)
      {
      newArray->Allocate(datadims[0] * datadims[1]);
      newArray->SetName(this->Internal->TableArrays[i].c_str());
      }
    }

  unsigned int scalarIndex = 0;
  int scalarCount = 0;
  int readFromTable = 0;

  if (result!= 0)
  {
    for (int k=2;k<5;k++)
    {
      if ( numRead < datadims[0] )
      {
        xCoords->InsertNextTuple1(  v[k] );
      }
      else if ( numRead < (datadims[0] + datadims[1]) )
      {
        yCoords->InsertNextTuple1(  v[k] );
      }
      else
      {
        scalarCount++;
        if(scalarCount > datadims[0] * datadims[1])
        {
          scalarCount = 1;
          scalarIndex++;
        }
        if(this->Internal->TableArrayStatus.size() > scalarIndex &&
          this->Internal->TableArrayStatus[scalarIndex])
        {
          scalars[scalarIndex]->InsertNextTuple1(v[k]);
        }
      }
      numRead++;
    }
  }


  while ( (readFromTable = ReadTableValueLine( &(v[0]), &(v[1]), &(v[2]), &(v[3]),
      &(v[4])  )) != 0)
    {
    for (int k=0;k<readFromTable;k++)
      {
      if ( numRead < datadims[0] )
        {
        xCoords->InsertNextTuple1(  v[k] );
        }
      else if ( numRead < (datadims[0] + datadims[1]) )
        {
        yCoords->InsertNextTuple1(  v[k] );
        }
      else
        {
        scalarCount++;
        if(scalarCount > datadims[0] * datadims[1])
          {
          scalarCount = 1;
          scalarIndex++;
          }
        if(this->Internal->TableArrayStatus.size() > scalarIndex &&
           this->Internal->TableArrayStatus[scalarIndex])
          {
          scalars[scalarIndex]->InsertNextTuple1(v[k]);
          }
        }
      numRead++;
      }
    }

  for(i=scalarIndex+1;
      i<this->Internal->TableArrayStatus.size();
      i++)
    {
    // fill in the empty scalars with zeros
    int max = datadims[0] * datadims[1];
    for(int j=0; j<max; j++)
      {
      scalars[i]->InsertNextTuple1(0.0);
      }
    }

  output->SetXCoordinates( xCoords );
  output->SetYCoordinates( yCoords );
  output->SetZCoordinates( zCoords );
  output->GetFieldData()->AddArray(xName);
  output->GetFieldData()->AddArray(yName);

  output->GetPointData()->Reset();

  for(i=0; i<scalars.size(); i++)
    {
    if(scalars[i])
      {
      if(scalars[i]->GetNumberOfTuples())
        {
        output->GetPointData()->AddArray(scalars[i]);
        }
      scalars[i]->Delete();
      }
    }

  xCoords->Delete();
  yCoords->Delete();
  zCoords->Delete();

  output->Squeeze();
}

int vtkPrismSESAMEReader::ReadTableValueLine ( float *v1, float *v2,
  float *v3, float *v4, float *v5)
{
  // by definition, a line of this file is 80 characters long
  // when we start reading the data values, the end of the line is a tag
  // (see note below), which we have to ignore in order to read the data
  // properly
  //
  char buffer[SESAME_NUM_CHARS + 1];
  buffer[SESAME_NUM_CHARS] = '\0';
  int numRead = 0;
  if ( fgets(buffer, SESAME_NUM_CHARS, this->Internal->File) != NULL )
    {
    int dummy;
    int internalId;
    int tableId;

    // see if the line matches the  " 0 9999 602" format
    if(sscanf(buffer, TableLineFormat, &dummy, &internalId, &tableId) == 3)
      {
      // this is the start of a new table
      numRead = 0;
      }
    else
      {
      // ignore the last 5 characters of the line (see notes above)
      buffer[75] = '\0';
      numRead = sscanf( buffer, "%e%e%e%e%e", v1, v2, v3, v4, v5);
      }
    }

  return numRead;
}

void vtkPrismSESAMEReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FileName: " << this->GetFileName() << "\n";
  os << indent << "Table: " << this->GetTable() << "\n";
}
