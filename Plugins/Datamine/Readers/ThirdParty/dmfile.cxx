/*
   Implementation of the TDMFile and TDMVar classes. Look at dmfile.hxx
   for full info.

   Revisions:
   99-04-11: Written by Jeremy Maccelari, visualn@iafrica.com
   99-04-26: cleaned up dumping to stdout from LoadFile, JM
   99-05-03: added byte_swapping, JM
   99-05-06: fixed byte swapping for variables, JM
   00-06-15: altered byte swapping routine so it reads test data correctly, AWD
   00-06-15: added IAC_DEBUG preprocessor definitions, AWD
   01-01-09: fixed major leak in ~TDMVariable, JM
   07-09-19: mod to handle both 32 bit and 64 bit formats, Bob Anderson, MIRARCO
   08-09-18: Fixed row reading to handle char*, Robert Maynard, MIRARCO
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dm.h"
#include "dmfile.h"

//#define IAC_DEBUG 1

// ---------------------------------------- Format parameters
// -------------------------------------------------
bool FMT64;     // false = 32 bit format, true = 64 bit format
int BufferSize; // SIZE_OF_BUFFER for 32 bit, SIZE_OF_BUFF64 for 64 bit format
int WordSize;   // SIZE_OF_WORD for 32 bit,   SIZE_OF_WORD64 for 64 bit format.

// ---------------------------------------- Utility functions
// -------------------------------------------------
void VISswap_4_byte_ptr(char* ptr)
{
  char tmp;

  for (int si = 0; si < 2; si++)
  {
    tmp = ptr[si];
    ptr[si] = ptr[7 - si];
    ptr[7 - si] = tmp;
  }
}

void VISswap_8_byte_ptr(char* ptr)
{
  char tmp;

  for (int si = 0; si < 4; si++)
  {
    tmp = ptr[si];
    ptr[si] = ptr[7 - si];
    ptr[7 - si] = tmp;
  }
}

// ---------------------------------------- TDMVariable class
// -------------------------------------------------
TDMVariable::TDMVariable()
{
  // public data
  fData = NULL;
  cData = NULL;
  // private data
  strcpy(DefaultAlphanumerical, "");
  DefaultNumerical = 0.0;
  LogicalRecPos = 0;
  strcpy(Name, "");
  nData = 0;
  strcpy(Type, "");
  strcpy(Unit, "");
  WordNumber = 0;
}

TDMVariable::~TDMVariable()
{
  // DEBUG_PRINT("deleting TDMVariable");
  // DEBUG_PRINT("freeing cData");
  if (cData)
  {
    for (int d = 0; d < nData; d++)
      delete[] cData[d];
    delete[] cData;
  }
  // DEBUG_PRINT("freeing fData");
  if (fData)
    delete[] fData;
}

float TDMVariable::GetDefaultNumerical()
{
  return (DefaultNumerical);
}

char* TDMVariable::GetDefaultAlphanumerical(char* def)
{
  strcpy(def, DefaultAlphanumerical);
  return (def);
}

int TDMVariable::GetLogicalRecPos()
{
  return (LogicalRecPos);
}

char* TDMVariable::GetName(char* name)
{
  strcpy(name, Name);
  return (name);
}

int TDMVariable::GetNData()
{
  return (nData);
}

char* TDMVariable::GetType(char* type)
{
  strcpy(type, Type);
  return (type);
}

char* TDMVariable::GetUnit(char* unit)
{
  strcpy(unit, Unit);
  return (unit);
}

int TDMVariable::GetWordNumber()
{
  return (WordNumber);
}

void TDMVariable::SetDefaultAlphanumericalFromBuf(char* buf, int varNo)
{
  int i, j;
  char def[SIZE_OF_WORD + 1];

  for (i = ((34 + (7 * varNo)) * WordSize), j = 0; i < ((35 + (7 * varNo)) * WordSize); i++)
  {
    def[j++] = buf[i];
    if (FMT64 && ((i + 1) % 4) == 0)
      i += 4; // skip second half of 64 bit word.
  }
  def[j] = 0x00;
  strcpy(DefaultAlphanumerical, def);
#ifdef IAC_DEBUG
  fprintf(stdout, "Default alphanumerical = %s\n", GetDefaultAlphanumerical(def));
#endif
}

void TDMVariable::SetDefaultNumericalFromBuf(char* buf, int varNo)
{
  float f;
  double d;

  if (FMT64)
    memcpy(&d, (buf + ((34 + (7 * varNo)) * WordSize)), SIZE_OF_DOUBLE);
  else
    memcpy(&f, (buf + ((34 + (7 * varNo)) * WordSize)), SIZE_OF_FLOAT);
  if (ByteSwapped)
  {
    if (FMT64)
      VISswap_8_byte_ptr((char*)&d);
    else
      VISswap_4_byte_ptr((char*)&f);
  }
  if (FMT64)
    DefaultNumerical = (float)d;
  else
    DefaultNumerical = f;
#ifdef IAC_DEBUG
  fprintf(stdout, "Default numerical = %.3f\n", GetDefaultNumerical());
#endif
}

void TDMVariable::SetLogicalRecPosFromBuf(char* buf, int varNo)
{
  float f;
  double d;

  if (FMT64)
    memcpy(&d, (buf + ((31 + (7 * varNo)) * WordSize)), SIZE_OF_DOUBLE);
  else
    memcpy(&f, (buf + ((31 + (7 * varNo)) * WordSize)), SIZE_OF_FLOAT);
  if (ByteSwapped)
  {
    if (FMT64)
      VISswap_8_byte_ptr((char*)&d);
    else
      VISswap_4_byte_ptr((char*)&f);
  }
  if (FMT64)
    LogicalRecPos = (int)d;
  else
    LogicalRecPos = (int)f;
#ifdef IAC_DEBUG
  fprintf(stdout, "Field position in logical record (0 = implicit) = %d\n", GetLogicalRecPos());
#endif
}

void TDMVariable::SetNameFromBuf(char* buf, int varNo)
{
  int i, j;
  char name[(2 * SIZE_OF_WORD) + 1];

  for (i = ((28 + (7 * varNo)) * WordSize), j = 0; i < ((30 + (7 * varNo)) * WordSize); i++)
  {
    name[j++] = buf[i];
    if (FMT64 && ((i + 1) % 4) == 0)
      i += 4; // skip second half of 64 bit word.
  }
  name[j] = 0x00;
  strcpy(Name, name);
#ifdef IAC_DEBUG
  fprintf(stdout, "Field name = *%s*\n", GetName(name));
#endif
}

void TDMVariable::SetNData(int ndata)
{
  int d;
  DEBUG_PRINT("freeing cData");
  if (cData)
  {
    for (d = 0; d < nData; d++)
      delete[] cData[d];
    delete[] cData;
  }
  // DEBUG_PRINT("freeing fData");
  if (fData)
    delete[] fData;
  nData = ndata;

  // do one word strings, since I don't have an example to check them
  DEBUG_PRINT("newing cData");
  cData = new char*[nData];
  for (d = 0; d < nData; d++)
    cData[d] = new char[SIZE_OF_WORD + 1];

  // DEBUG_PRINT("newing fData");
  fData = new float[nData];

#ifdef IAC_DEBUG
  fprintf(stdout, "NData = %d\n", GetNData());
#endif
}

void TDMVariable::SetTypeFromBuf(char* buf, int varNo)
{
  int i, j;
  char type[SIZE_OF_WORD + 1];

  for (i = ((30 + (7 * varNo)) * WordSize), j = 0; i < ((31 + (7 * varNo)) * WordSize); i++)
  {
    type[j++] = buf[i];
    if (FMT64 && ((i + 1) % 4) == 0)
      i += 4; // skip second half of 64 bit word.
  }
  type[j] = 0x00;
  strcpy(Type, type);

#ifdef IAC_DEBUG
  fprintf(stdout, "Field type = *%s*\n", GetType(type));
#endif
}

void TDMVariable::SetUnitFromBuf(char* buf, int varNo)
{
  int i, j;
  char unit[SIZE_OF_WORD + 1];

  for (i = ((33 + (7 * varNo)) * WordSize), j = 0; i < ((34 + (7 * varNo)) * WordSize); i++)
  {
    unit[j++] = buf[i];
    if (FMT64 && ((i + 1) % 4) == 0)
      i += 4; // skip second half of 64 bit word.
  }
  unit[j] = 0x00;
  strcpy(Unit, unit);

#ifdef IAC_DEBUG
  fprintf(stdout, "Unit = *%s*\n", GetUnit(unit));
#endif
}

void TDMVariable::SetWordNumberFromBuf(char* buf, int varNo)
{
  float f;
  double d;

  if (FMT64)
    memcpy(&d, (buf + ((32 + (7 * varNo)) * WordSize)), SIZE_OF_DOUBLE);
  else
    memcpy(&f, (buf + ((32 + (7 * varNo)) * WordSize)), SIZE_OF_FLOAT);
  if (ByteSwapped)
  {
    if (FMT64)
      VISswap_8_byte_ptr((char*)&d);
    else
      VISswap_4_byte_ptr((char*)&f);
  }
  if (FMT64)
    WordNumber = (int)d;
  else
    WordNumber = (int)f;
}

bool TDMVariable::TypeIsNumerical()
{
  if (strcmp(Type, "N   ") == 0)
    return (true);
  else
    return (false);
}

// ---------------------------------------- TDMFile class
// -----------------------------------------------------
TDMFile::TDMFile()
{
  this->is64 = false;
  ByteSwapped = false;
  strcpy(Description, "");
  strcpy(DirName, "");
  strcpy(FileName, "");
  FileType = invalid;
  LastModDate = 0;
  LogicalDataRecLen = 0;
  NLastPageRecs = 0;
  NPhysicalPages = 0;
  nVars = 0;
  OtherPerms = 2;
  strcpy(Owner, "");
  OwnerPerms = 0;
  Vars = NULL;
  recVars = NULL;
}

TDMFile::~TDMFile()
{
  // DEBUG_PRINT("deleting TDMFile");
  if (Vars)
  {
    delete[] Vars;
    Vars = NULL;
  }
  if (recVars)
  {
    delete recVars;
  }
}

bool TDMFile::GetByteSwapped()
{
  return (ByteSwapped);
}

char* TDMFile::GetDescription(char* desc)
{
  strcpy(desc, Description);
  return (desc);
}

char* TDMFile::GetDirName(char* name)
{
  strcpy(name, DirName);
  return (name);
}

char* TDMFile::GetFileName(char* name)
{
  strcpy(name, FileName);
  return (name);
}

FileTypes TDMFile::GetFileType()
{
  return (FileType);
}

char* TDMFile::GetOwner(char* owner)
{
  strcpy(owner, Owner);
  return (owner);
}

int TDMFile::GetLastModDate()
{
  return (LastModDate);
}

int TDMFile::GetLogicalDataRecLen()
{
  return (LogicalDataRecLen);
}

int TDMFile::GetNLastPageRecs()
{
  return (NLastPageRecs);
}

int TDMFile::GetNPhysicalPages()
{
  return (NPhysicalPages);
}

int TDMFile::GetOtherPerms()
{
  return (OtherPerms);
}

int TDMFile::GetNumberOfRecords()
{
  // record length
  int recordLength = GetLogicalDataRecLen();

  // number of pages minus the last + header page
  int numOfPages = GetNPhysicalPages() - 2;

  // number of records per page
  // There is only 508 bytes per page ( table is 512 with 4 reserved )
  const int PAGE_SIZE = 508;
  int numRecsPerPage = PAGE_SIZE / recordLength;

  // number of record on last page
  int numRecsLastPage = GetNLastPageRecs();

  // total amount of records
  return numOfPages * numRecsPerPage + numRecsLastPage;
}

int TDMFile::GetOwnerPerms()
{
  return (OwnerPerms);
}

bool TDMFile::LoadFileHeader(const char* fname)
{
  FILE* in;
  int nd, nv, v;
  char buf[SIZE_OF_BUFF64];

  if ((in = fopen(fname, "rb")) == NULL)
  {
    return false;
  }
  int rdsz;
  FMT64 = false; // default to 32 bit format
  this->is64 = false;
  BufferSize = SIZE_OF_BUFFER; // default to 32 bit size.
  WordSize = SIZE_OF_WORD;     // default to 32 bit size.

  // get header
  if ((rdsz = (int)fread(buf, sizeof(char), BufferSize, in)) != BufferSize)
  {
    if (in)
      fclose(in);
    return false;
  }

  // Establish if it is 32 or 64 bit format.
  // NOTE: Documentation and observation show that if it is double precision,
  // the 25th 64-bit word (of 1 to n) will be a double precision value = 456789.0
  // If not 456789.0 it should be 987654.0, but old files may not have set it.
  // the 25th 32-bit word (of 1 to n) should be 987654.0, but old files may not
  // have set it.  Word 25 was unused prior to introduction of double precision.
  double* dpp = (double*)&buf[24 * 8]; // pointer to 25th 64-bit word.
  if (*dpp == 456789.0)
  { // Positive test for FMT64
    FMT64 = true;
    this->is64 = true;
    // read the second half of the FMT64 page.
    fread(&buf[BufferSize], sizeof(char), BufferSize, in);
    BufferSize = SIZE_OF_BUFF64; // set 64 bit buffer size.
    WordSize = SIZE_OF_WORD64;
  }
  fclose(in); // re-open in LoadFIle()
  // fill it
  SetByteSwapped(buf);
  SetFileNameFromBuf(buf);
  SetDescriptionFromBuf(buf);
  SetOwnerFromBuf(buf);
  SetOwnerPermsFromBuf(buf);
  SetOtherPermsFromBuf(buf);
  SetLastModDateFromBuf(buf);
  // don't print LogicalDataRecLen yet, since it is updated when we check for implicit variables
  SetLogicalDataRecLenFromBuf(buf);
  nv = nVars = (int)GetLogicalDataRecLen();
  Vars = new TDMVariable[nVars];
  SetNPhysicalPagesFromBuf(buf);
  SetNLastPageRecsFromBuf(buf);
  for (v = 0; v < nv; v++)
  {
    Vars[v].ByteSwapped = GetByteSwapped();
    Vars[v].SetNameFromBuf(buf, v);
    Vars[v].SetTypeFromBuf(buf, v);
    Vars[v].SetLogicalRecPosFromBuf(buf, v);
    // if the variable is implicit, we decrement the LogicalRecordLength, since it is not
    // stored in the data records
    if (Vars[v].GetLogicalRecPos() == 0)
      DecrementLogicalDataRecLen();
    Vars[v].SetWordNumberFromBuf(buf, v);
    Vars[v].SetUnitFromBuf(buf, v);
    if (Vars[v].TypeIsNumerical())
      Vars[v].SetDefaultNumericalFromBuf(buf, v);
    else
      Vars[v].SetDefaultAlphanumericalFromBuf(buf, v);
  }
  for (v = 0; v < nv; v++)
  {
    if (GetLogicalDataRecLen() > 0)
      nd = (GetNPhysicalPages() - 2) * (508 / GetLogicalDataRecLen()) + GetNLastPageRecs();
    else
      nd = 0;
  }
  SetFileType();
  m_nd = nd;
  m_nv = nv;
  return true;
}

void TDMFile::LoadFile(const char* fname)
{
  FILE* in;
  int d, nd, np, nr, nv, p, r, v;
  int i, j;
  char tmpstr[129];
  char buf[SIZE_OF_BUFF64];
  float df;
  double dd;

  if ((in = fopen(fname, "rb")) == NULL)
  {
    return;
  }
  int rdsz;

  // get header
  if ((rdsz = (int)fread(buf, sizeof(char), BufferSize, in)) != BufferSize)
  {
    if (in)
      fclose(in);
    return;
  }
  nd = m_nd;
  nv = m_nv;
  for (v = 0; v < nv; v++)
  {
    if (GetLogicalDataRecLen() > 0)
      nd = (GetNPhysicalPages() - 2) * (508 / GetLogicalDataRecLen()) + GetNLastPageRecs();
    else
      nd = 0;
    Vars[v].SetNData(nd);
  }

  // Now actually read in the data from the file
  // number of data pages = number of pages less the first one we have already read in
  np = GetNPhysicalPages() - 1;
  if (np < 0)
    np = 0;
  d = 0;
  for (p = 0; p < np; p++)
  {
    // get this page and process it
    if ((rdsz = (int)fread(buf, sizeof(char), BufferSize, in)) != BufferSize && p != (np - 1))
    {
      fclose(in);
      nVars = 0;
      if (Vars)
        delete[] Vars;
      Vars = NULL;
      return;
    }
    if (GetLogicalDataRecLen() > 0)
      nr = 508 / GetLogicalDataRecLen();
    else
      nr = 0;
    if (p == (np - 1))
      nr = GetNLastPageRecs();
    // loop over records
    for (r = 0; r < nr; r++)
    {
      // loop over variables for this record
      for (v = 0; v < nv; v++)
      {
        if (!active_vars[v])
          continue;
        if (Vars[v].TypeIsNumerical())
        {
          // check if variable is implicit or not
          if (Vars[v].GetLogicalRecPos() != 0)
          {
            if (FMT64)
              memcpy(&dd, buf +
                  (((r * GetLogicalDataRecLen()) + (Vars[v].GetLogicalRecPos() - 1)) * WordSize),
                SIZE_OF_DOUBLE);
            else
              memcpy(&df, buf +
                  (((r * GetLogicalDataRecLen()) + (Vars[v].GetLogicalRecPos() - 1)) * WordSize),
                SIZE_OF_FLOAT);
            if (ByteSwapped)
            {
              if (FMT64)
                VISswap_8_byte_ptr((char*)&dd);
              else
                VISswap_4_byte_ptr((char*)&df);
            }
            if (FMT64)
              Vars[v].fData[d] = (float)dd;
            else
              Vars[v].fData[d] = df;
          }
          else
          {
            Vars[v].fData[d] = Vars[v].GetDefaultNumerical();
          }
        }
        else
        {
          // don't know if this works, since I don't have test data
          if (Vars[v].GetLogicalRecPos() != 0)
          {
            for (i = 0, j = 0; i < 4; i++)
            {
              tmpstr[j++] =
                buf[((r * GetLogicalDataRecLen()) + (Vars[v].GetLogicalRecPos() - 1)) * WordSize +
                  i];
            }
            tmpstr[j] = 0x00;
            strcpy(Vars[v].cData[d], tmpstr);
          }
          else
          {
            strcpy(Vars[v].cData[d], Vars[v].GetDefaultAlphanumerical(tmpstr));
          }
        }
      }
      d++;
    }
  }

  fclose(in);
}

void TDMFile::SetByteSwapped(char* buf)
{
  int date;

  // get date and check it is sensible, if not assume byte swapping
  if (FMT64)
    memcpy((char*)&date, (buf + (24 * WordSize) + 4), 1 * sizeof(int));
  else
    memcpy((char*)&date, (buf + (24 * WordSize)), 1 * sizeof(int));

  // Test endianess of data
  // note the following date check is not Y10k compliant
  if ((date < 720101) || (date > 99991231))
  {
    ByteSwapped = false;
  }
  else
  {
    ByteSwapped = true;
  }

#ifdef IAC_DEBUG
  if (ByteSwapped)
  {
    fprintf(stdout, "File is byte swapped. Date : %d\n", date);
    VISswap_4_byte_ptr((char*)&date);
    fprintf(stdout, "Date should be : %d\n", date);
  }
  else
  {
    fprintf(stdout, "File is not byte swapped. Date : %d\n", date);
  }
#endif
}

void TDMFile::SetDescriptionFromBuf(char* buf)
{
  int i, j;
  char desc[(16 * SIZE_OF_WORD) + 1];

  // get description
  for (i = (4 * WordSize), j = 0; i < (20 * WordSize); i++)
  {
    desc[j++] = buf[i];
    if (FMT64 && ((i + 1) % 4) == 0)
      i += 4; // skip second half of 64 bit word.
  }
  desc[j] = 0x00;
  strcpy(Description, desc);
#ifdef IAC_DEBUG
  fprintf(stdout, "Description = %s\n", GetDescription(desc));
#endif
}

void TDMFile::SetFileNameFromBuf(char* buf)
{
  int i, j;
  char dname[(2 * SIZE_OF_WORD) + 1], name[(2 * SIZE_OF_WORD) + 1];

  // get filename
  for (i = (0 * WordSize), j = 0; i < (2 * WordSize); i++)
  {
    name[j++] = buf[i];
    if (FMT64 && ((i + 1) % 4) == 0)
      i += 4; // skip second half of 64 bit word.
  }
  name[j] = 0x00;
  strcpy(FileName, name);

  // get directory name
  for (i = (2 * WordSize), j = 0; i < (4 * WordSize); i++)
  {
    dname[j++] = buf[i];
    if (FMT64 && ((i + 1) % 4) == 0)
      i += 4; // skip second half of 64 bit word.
  }
  dname[j] = 0x00;
  strcpy(DirName, dname);

#ifdef IAC_DEBUG
  fprintf(stdout, "Filename = %s\n", GetFileName(name));
  fprintf(stdout, "Directory name = %s\n", GetDirName(dname));
#endif
}

void TDMFile::SetFileType()
{
  int found;
  int v;
  char name[(2 * SIZE_OF_WORD) + 1];

  if (nVars > 0)
  {

    // Use goto to avoid too many nested if ... else's. Not quite kosher but if
    // you don't like it, please nest this lot for me...
    // We look for a distinctive variable name for each file type
    FileType = invalid;

    // RMAYNARD START
    // HAD TO REORDER SINCE WIREFRAME HAS PRIORITY, SECONDLY SINCE SOME WIREFRAMES
    // HAVE A PROPERTY NAMED IJK

    // wframetriangle, TRIANGLE, PID1, PID2, PID3
    for (v = 0; v < nVars; v++)
    {
      if (strcmp(Vars[v].GetName(name), "TRIANGLE") == 0)
      {
        DEBUG_PRINT("wframetriangle");
        FileType = wframetriangle;
        goto FoundLabel;
      }
    }
    // Found:;

    // wframepoints, XP, YP, ZP, PID
    for (v = 0; v < nVars; v++)
    {
      if (strcmp(Vars[v].GetName(name), "PID     ") == 0)
      {
        DEBUG_PRINT("wframepoints");
        FileType = wframepoints;
        goto FoundLabel;
      }
    }

    // perimeter, XP, YP, ZP, PTN, PVALUE
    for (v = 0; v < nVars; v++)
    {
      if (strcmp(Vars[v].GetName(name), "PTN     ") == 0)
      {
        DEBUG_PRINT("perimeter");
        FileType = perimeter;
        goto FoundLabel;
      }
    }
    // plot, X, Y, S1, S2, CODE, COLOUR, XMIN, XMAX, XSCALE, YSCALE, XORIG, YORIG, CHARSIZE,
    // ASPRATIO
    // changed to ASPRATIO because of some weird wireframe models
    for (v = 0; v < nVars; v++)
    {
      if (strcmp(Vars[v].GetName(name), "ASPRATIO") == 0)
      {
        DEBUG_PRINT("plot");
        FileType = plot;
        goto FoundLabel;
      }
    }
    // drillhole, BHID, FROM, TO, LENGTH, X, Y, Z, A0, B0
    // changed the logic because some wireframes have the BHID property
    found = 0;
    for (v = 0; v < nVars; v++)
    {
      if (strcmp(Vars[v].GetName(name), "BHID    ") == 0)
      { // weed out collar, survey, assay, litho files
        found++;
      }
      else if (strcmp(Vars[v].GetName(name), "LENGTH  ") == 0)
      {
        found++;
      }
      if (found == 2)
      {
        DEBUG_PRINT("drillhole");
        FileType = drillhole;
        goto FoundLabel;
      }
    }

    // blockmodel, IJK, XC, YC, ZC, XINC, YINC, ZINC, XMORIG, YMORIG, NX, NY, NZ
    found = 0;
    for (v = 0; v < nVars; v++)
    {
      // found needs to be at 2 or higher so we can presume the file is blockmodel
      // rather than something else with properties that have conflicting names

      if (strcmp(Vars[v].GetName(name), "IJK     ") == 0)
      {
        found++;
      }

      if (strcmp(Vars[v].GetName(name), "XINC    ") == 0)
      {
        found++;
      }

      if (strcmp(Vars[v].GetName(name), "XC      ") == 0)
      {
        found++;
      }

      if (found >= 2)
      {
        DEBUG_PRINT("blockmodel");
        FileType = blockmodel;
        goto FoundLabel;
      }
    }
    // RMAYNARD END

    // sectiondefinition, XCENTRE, YCENTRE, ZCENTRE, SDIP, SAZI, HSIZE, VSIZE
    for (v = 0; v < nVars; v++)
    {
      if (strcmp(Vars[v].GetName(name), "XCENTRE ") == 0)
      {
        DEBUG_PRINT("sectiondefinition");
        FileType = sectiondefinition;
        goto FoundLabel;
      }
    }
    // catalogue, FILENAM
    for (v = 0; v < nVars; v++)
    {
      if (strcmp(Vars[v].GetName(name), "FILENAM ") == 0)
      {
        DEBUG_PRINT("catalogue");
        FileType = catalogue;
        goto FoundLabel;
      }
    }
    // results, MODEL, BLOCKID, DENSITY, VOLUME, TONNES
    // do this test before 'scheduling' since both have BLOCKID
    for (v = 0; v < nVars; v++)
    {
      if (strcmp(Vars[v].GetName(name), "MODEL   ") == 0)
      {
        DEBUG_PRINT("results");
        FileType = results;
        goto FoundLabel;
      }
    }
    // scheduling, BLOCKID, VOLUME, TONNES, DENSITY, SLOT, PERCENT, DRAW, START, END, LENGTH, AREA
    for (v = 0; v < nVars; v++)
    {
      if (strcmp(Vars[v].GetName(name), "BLOCKID ") == 0)
      {
        DEBUG_PRINT("scheduling");
        FileType = scheduling;
        goto FoundLabel;
      }
    }

    // stope summary, STOPE, STOPTYPE, GROUP, XSTOPE, YSTOPE, ZSTOPE, ...
    found = 0;
    for (v = 0; v < nVars; v++)
    {
      // found needs to be at 2 or higher so we can presume the file is blockmodel
      // rather than something else with properties that have conflicting names

      if (strcmp(Vars[v].GetName(name), "STOPE   ") == 0)
      {
        found++;
      }

      if (strcmp(Vars[v].GetName(name), "STOPTYPE") == 0)
      {
        found++;
      }

      if (strcmp(Vars[v].GetName(name), "XSTOPE  ") == 0)
      {
        found++;
      }

      if (found >= 2)
      {
        DEBUG_PRINT("STOPESUMMARY");
        FileType = stopesummary;
        goto FoundLabel;
      }
    }
    // rosette, ROSNUM, ROSXPOS, ROSYPOS, ROSZMIN, ROSZMAX, ROSAZIM, ROSFANG, ROSBWID
    for (v = 0; v < nVars; v++)
    {
      if (strcmp(Vars[v].GetName(name), "ROSNUM  ") == 0)
      {
        DEBUG_PRINT("rosette");
        FileType = rosette;
        goto FoundLabel;
      }
    }
    // drivestats, PVALUE, VOLUME, TONNES, LENGTH, ZMIN, ZMAX, GRADIENT, XAREA
    for (v = 0; v < nVars; v++)
    {
      if (strcmp(Vars[v].GetName(name), "XAREA   ") == 0)
      {
        DEBUG_PRINT("drivestats");
        FileType = drivestats;
        goto FoundLabel;
      }
    }
    // point, XPT, YPT, ZPT
    for (v = 0; v < nVars; v++)
    {
      if (strcmp(Vars[v].GetName(name), "XPT     ") == 0)
      {
        DEBUG_PRINT("point");
        FileType = point;
        goto FoundLabel;
      }
    }
    // dependency, PNUM1, PNUM2
    for (v = 0; v < nVars; v++)
    {
      if (strcmp(Vars[v].GetName(name), "PNUM1   ") == 0)
      {
        DEBUG_PRINT("dependency");
        FileType = dependency;
        goto FoundLabel;
      }
    }
    // plotterpen, COLOUR, PEN
    for (v = 0; v < nVars; v++)
    {
      if (strcmp(Vars[v].GetName(name), "PEN     ") == 0)
      {
        DEBUG_PRINT("plotterpen");
        FileType = plotterpen;
        goto FoundLabel;
      }
    }
    // plotfilter, FIELD, TEST, IN, OUT, PEN
    for (v = 0; v < nVars; v++)
    {
      if (strcmp(Vars[v].GetName(name), "FIELD   ") == 0)
      {
        DEBUG_PRINT("plotfilter");
        FileType = plotfilter;
        goto FoundLabel;
      }
    }
    // validation ATTNAME, ATTTYPE, VALUE, MIN, MAX, DEFAULT
    for (v = 0; v < nVars; v++)
    {
      if (strcmp(Vars[v].GetName(name), "ATTNAME ") == 0)
      {
        DEBUG_PRINT("validation");
        FileType = validation;
        goto FoundLabel;
      }
    }
  // All ifs goto here
  FoundLabel:;
  }
  else
  {
    DEBUG_PRINT("nVars <= 0 : filetype invalid");
    FileType = invalid;
  }
}

void TDMFile::SetLastModDateFromBuf(char* buf)
{
  float f;
  double d;

  if (FMT64)
    memcpy(&d, (buf + (24 * WordSize)), SIZE_OF_DOUBLE);
  else
    memcpy(&f, (buf + (24 * WordSize)), SIZE_OF_FLOAT);
  if (ByteSwapped)
  {
    if (FMT64)
      VISswap_8_byte_ptr((char*)&d);
    else
      VISswap_4_byte_ptr((char*)&f);
  }
  if (FMT64)
    LastModDate = (int)d;
  else
    LastModDate = (int)f;
#ifdef IAC_DEBUG
  fprintf(stdout, "Last file modification date = %d\n", GetLastModDate());
#endif
}

void TDMFile::SetLogicalDataRecLenFromBuf(char* buf)
{
  float f;
  double d;

  if (FMT64)
    memcpy(&d, (buf + (25 * WordSize)), SIZE_OF_DOUBLE);
  else
    memcpy(&f, (buf + (25 * WordSize)), SIZE_OF_FLOAT);
  if (ByteSwapped)
  {
    if (FMT64)
      VISswap_8_byte_ptr((char*)&d);
    else
      VISswap_4_byte_ptr((char*)&f);
  }
  if (FMT64)
    LogicalDataRecLen = (int)d;
  else
    LogicalDataRecLen = (int)f;
#ifdef IAC_DEBUG
  fprintf(stdout, "Logical data record length = %d\n", GetLogicalDataRecLen());
#endif
}

void TDMFile::DecrementLogicalDataRecLen()
{
  LogicalDataRecLen--;
}

void TDMFile::SetNLastPageRecsFromBuf(char* buf)
{
  float f;
  double d;

  if (FMT64)
    memcpy(&d, (buf + (27 * WordSize)), SIZE_OF_DOUBLE);
  else
    memcpy(&f, (buf + (27 * WordSize)), SIZE_OF_FLOAT);
  if (ByteSwapped)
  {
    if (FMT64)
      VISswap_8_byte_ptr((char*)&d);
    else
      VISswap_4_byte_ptr((char*)&f);
  }
  if (FMT64)
    NLastPageRecs = (int)d;
  else
    NLastPageRecs = (int)f;
#ifdef IAC_DEBUG
  fprintf(stdout, "Number of logical records in last page of file = %d\n", GetNLastPageRecs());
#endif
}

void TDMFile::SetNPhysicalPagesFromBuf(char* buf)
{
  float f;
  double d;

  if (FMT64)
    memcpy(&d, (buf + (26 * WordSize)), SIZE_OF_DOUBLE);
  else
    memcpy(&f, (buf + (26 * WordSize)), SIZE_OF_FLOAT);
  if (ByteSwapped)
  {
    if (FMT64)
      VISswap_8_byte_ptr((char*)&d);
    else
      VISswap_4_byte_ptr((char*)&f);
  }
  if (FMT64)
    NPhysicalPages = (int)d;
  else
    NPhysicalPages = (int)f;
#ifdef IAC_DEBUG
  fprintf(stdout, "Number of physical pages in file = %d\n", GetNPhysicalPages());
#endif
}

void TDMFile::SetOwnerFromBuf(char* buf)
{
  int i, j;
  char owner[(2 * SIZE_OF_WORD) + 1];

  for (i = (20 * WordSize), j = 0; i < (22 * WordSize); i++)
  {
    owner[j++] = buf[i];
    if (FMT64 && ((i + 1) % 4) == 0)
      i += 4; // skip second half of 64 bit word.
  }
  owner[j] = 0x00;
  strcpy(Owner, owner);
#ifdef IAC_DEBUG
  fprintf(stdout, "Owner = %s\n", GetOwner(owner));
#endif
}

void TDMFile::SetOtherPermsFromBuf(char* buf)
{
  float f;
  double d;

  if (FMT64)
    memcpy(&d, (buf + (23 * WordSize)), SIZE_OF_DOUBLE);
  else
    memcpy(&f, (buf + (23 * WordSize)), SIZE_OF_FLOAT);
  if (ByteSwapped)
  {
    if (FMT64)
      VISswap_8_byte_ptr((char*)&d);
    else
      VISswap_4_byte_ptr((char*)&f);
  }
  if (FMT64)
    OtherPerms = (int)d;
  else
    OtherPerms = (int)f;
#ifdef IAC_DEBUG
  fprintf(stdout, "Other perms = %d\n", GetOtherPerms());
#endif
}

void TDMFile::SetOwnerPermsFromBuf(char* buf)
{
  float f;
  double d;

  if (FMT64)
    memcpy(&d, (buf + (22 * WordSize)), SIZE_OF_DOUBLE);
  else
    memcpy(&f, (buf + (22 * WordSize)), SIZE_OF_FLOAT);
  if (ByteSwapped)
  {
    if (FMT64)
      VISswap_8_byte_ptr((char*)&d);
    else
      VISswap_4_byte_ptr((char*)&f);
  }
  if (FMT64)
    OwnerPerms = (int)d;
  else
    OwnerPerms = (int)f;
#ifdef IAC_DEBUG
  fprintf(stdout, "Owner perms = %d\n", GetOwnerPerms());
#endif
}
void TDMFile::SetActiveVars(int indx, bool tf)
{
  if (indx < 80)
    active_vars[indx] = tf;
}
/*********************************************************************
 *  Get variable data for a singlee record.
 *  On first call (crec==0), open file, skip header and init parameters
 **********************************************************************/
int TDMFile::GetRecVars(int crec, double* recvars, char* filename)
{
  static FILE* in;
  static char buf[SIZE_OF_BUFF64];
  static int np, ldrl, nrpp, nvars;
  int rdsz, pgrecid;
  double dd;
  float df;
  if (crec == 0)
  {
    if (in != NULL)
      fclose(in);
    if ((in = fopen(filename, "rb")) == NULL)
    {
      return 0; // should never happen if we already read the header.
    }
    rdsz = (int)fread(buf, sizeof(char), BufferSize, in); // skip the header
    np = this->GetNPhysicalPages() - 1;                   // number of pages minus the last
    ldrl = this->GetLogicalDataRecLen();                  // record length
    nrpp = 508 / ldrl;                                    // number of records per page
    if (nVars > 56)
      nvars = 56;
    else
      nvars = nVars;
  }
  pgrecid = crec % nrpp;
  if (pgrecid == 0)
  { // read next page
    if ((rdsz = (int)fread(buf, sizeof(char), BufferSize, in)) != BufferSize)
      return 0;
  }
  for (int v = 0; v < nvars; v++)
  {
    if (Vars[v].TypeIsNumerical())
    {
      if (Vars[v].GetLogicalRecPos() != 0)
      {
        if (FMT64)
          memcpy(&dd, buf +
              (((pgrecid * GetLogicalDataRecLen()) + (Vars[v].GetLogicalRecPos() - 1)) * WordSize),
            SIZE_OF_DOUBLE);
        else
          memcpy(&df, buf +
              (((pgrecid * GetLogicalDataRecLen()) + (Vars[v].GetLogicalRecPos() - 1)) * WordSize),
            SIZE_OF_FLOAT);
        if (ByteSwapped)
        {
          if (FMT64)
            VISswap_8_byte_ptr((char*)&dd);
          else
            VISswap_4_byte_ptr((char*)&df);
        }
        if (FMT64)
          recvars[v] = dd;
        else
          recvars[v] = double(df);
      }
      else
      {
        recvars[v] = Vars[v].GetDefaultNumerical();
      }
    }
    else
    {
      recvars[v] = 0;
    }
  }
  return 1;
}

/*********************************************************************
 *  Everything after this point was added by
 *  Robert Maynard September 18 2008
 *  The purpose was to make dmfile.cxx actually properly handle
 *  string properties
 **********************************************************************/

bool TDMFile::Get64()
{
  return this->is64;
}
/*********************************************************************
 *  Get variable data for a single record.
 *  MODE determines what to do, removed the static variables, so we can have
 *  multiple files open in the same process
 **********************************************************************/
int TDMFile::GetRecVars(int crec, Data* values)
{
  int rdsz = 0;
  int pgrecid = crec % recVars->nrpp;
  int currentPage = crec / recVars->nrpp;

  if (currentPage == recVars->lastPage + 1)
  {
    // Toto, we're home. Home! And this is my room, and you're all here
    //...
    // there's no place like Sequential Read Mode!
    fread(recVars->buf, sizeof(char), BufferSize, recVars->in);
  }
  else if (currentPage != recVars->lastPage)
  {
    // Toto, I've a feeling we're not in Sequential Read Mode any more.
    long byteSizeOfPage = sizeof(char) * BufferSize;
    long pagePos = recVars->firstPagePosition + (currentPage * byteSizeOfPage);

    fseek(recVars->in, pagePos, SEEK_SET);
    // fsetpos(recVars->in, &pagePos); //move to new page to read ( random page read )

    // read the page we moved too
    fread(recVars->buf, sizeof(char), BufferSize, recVars->in);
  }

  // 64 bit read
  if (this->Get64())
  {
    double dd;
    for (int v = 0; v < nVars; v++)
    {
      dd = Vars[v].GetDefaultNumerical(); // default value

      if (Vars[v].GetLogicalRecPos() != 0)
      {
        memcpy(&dd, recVars->buf +
            (((pgrecid * GetLogicalDataRecLen()) + (Vars[v].GetLogicalRecPos() - 1)) * WordSize),
          SIZE_OF_DOUBLE);
        if (ByteSwapped)
        {
          VISswap_8_byte_ptr((char*)&dd);
        }
      }
      values[v].v = dd;
    }
  }
  else // 32 bit read
  {
    float df;
    for (int v = 0; v < nVars; v++)
    {
      df = (float)Vars[v].GetDefaultNumerical();

      if (Vars[v].GetLogicalRecPos() != 0)
      {
        memcpy(&df, recVars->buf +
            (((pgrecid * GetLogicalDataRecLen()) + (Vars[v].GetLogicalRecPos() - 1)) * WordSize),
          SIZE_OF_FLOAT);
        if (ByteSwapped)
        {
          VISswap_4_byte_ptr((char*)&df);
        }
      }
      values[v].v = (double)df;
    }
  }

  recVars->lastPage = currentPage; // update the last page we have read
  return 1;
}

bool TDMFile::OpenRecVarFile(const char* filename)
{
  if (!recVars)
  {
    this->recVars = new TDMRecVars();
    recVars->in = fopen(filename, "rb");

    // skip the header by seeking past it
    recVars->firstPagePosition = sizeof(char) * BufferSize;
    fseek(recVars->in, recVars->firstPagePosition, SEEK_SET);

    // read the first page into memory ahead of time
    fread(recVars->buf, sizeof(char), BufferSize, recVars->in);

    recVars->lastPage = 0;

    recVars->np = this->GetNPhysicalPages() - 1;  // number of pages minus the last
    recVars->ldrl = this->GetLogicalDataRecLen(); // record length
    recVars->nrpp = 508 / recVars->ldrl;          // number of records per page
    return true;
  }
  return false;
}

bool TDMFile::CloseRecVarFile()
{
  if (recVars)
  {
    delete recVars;
    recVars = NULL;
    return true;
  }
  return false;
}

/*********************************************************************
 *  New class to support multiple open datamine files with recvars
 **********************************************************************/

TDMRecVars::TDMRecVars()
{
  this->in = NULL;
  this->np = 0;
  this->ldrl = 0;
  this->nrpp = 0;
  this->lastPage = -1;
}
TDMRecVars::~TDMRecVars()
{
  if (this->in)
  {
    fclose(this->in);
  }
}
