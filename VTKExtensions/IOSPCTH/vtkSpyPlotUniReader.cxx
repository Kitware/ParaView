#include "vtkSpyPlotUniReader.h"
#include "vtkByteSwap.h"
#include "vtkDataArray.h"
#include "vtkDataArraySelection.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkSpyPlotBlock.h"
#include "vtkSpyPlotIStream.h"
#include "vtkUnsignedCharArray.h"

#include "vtksys/FStream.hxx"
#include "vtksys/RegularExpression.hxx"

#include <sstream>
#include <vector>

//=============================================================================
//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkSpyPlotUniReader);
vtkCxxSetObjectMacro(vtkSpyPlotUniReader, CellArraySelection, vtkDataArraySelection);

class vtkSpyPlotWriteString
{
public:
  vtkSpyPlotWriteString(const char* data, size_t length)
    : Data(data)
    , Length(length)
  {
  }

  const char* Data;
  size_t Length;
};

inline ostream& operator<<(ostream& os, const vtkSpyPlotWriteString& c)
{
  os.write(c.Data, c.Length);
  os.flush();
  return os;
}

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
  this->DumpCycle = 0;
  this->DumpTime = 0;
  this->DumpDT = 0;
  this->DumpOffset = 0;

  this->DataDumps = 0;
  this->Blocks = 0;

  this->CellArraySelection = 0;

  this->TimeStepRange[0] = this->TimeStepRange[1] = 0;
  this->TimeRange[0] = this->TimeRange[1] = 0.0;
  this->CurrentTimeStep = 0;
  this->CurrentTime = 0.0;

  this->NumberOfCellFields = 0;
  this->HaveInformation = 0;
  this->DownConvertVolumeFraction = 1;
  this->DataTypeChanged = 0;
  this->GeomTimeStep = -1; // Indicate that geometry will have to be loaded
  this->NeedToCheck = 1;   // Indicates non-geometric data needs to be checked
  if (!this->HaveInformation)
  {
    vtkDebugMacro(<< __LINE__ << " " << this << " Read: " << this->HaveInformation);
  }

  this->MarkersOn = 0;
  this->GenerateMarkers = 1;
}

//-----------------------------------------------------------------------------
vtkSpyPlotUniReader::~vtkSpyPlotUniReader()
{
  // Cleanup header
  delete[] this->CellFields;
  delete[] this->MaterialFields;
  delete[] this->DumpCycle;
  delete[] this->DumpTime;
  delete[] this->DumpDT;
  delete[] this->DumpOffset;

  int dump;
  for (dump = 0; dump < this->NumberOfDataDumps; ++dump)
  {
    vtkSpyPlotUniReader::DataDump* dp = this->DataDumps + dump;
    delete[] dp->SavedVariables;
    delete[] dp->SavedVariableOffsets;
    delete[] dp->SavedBlockAllocatedStates;
    if (dp->NumberOfTracers > 0)
    {
      dp->TracerCoord->Delete();
      dp->TracerBlock->Delete();
    }
    int var;
    for (var = 0; var < dp->NumVars; ++var)
    {
      vtkSpyPlotUniReader::Variable* cv = dp->Variables + var;
      delete[] cv->Name;
      if (cv->DataBlocks)
      {
        int ca;
        for (ca = 0; ca < dp->ActualNumberOfBlocks; ++ca)
        {
          if (cv->DataBlocks[ca])
          {
            cv->DataBlocks[ca]->Delete();
          }
        }
        delete[] cv->DataBlocks;
        delete[] cv->GhostCellsFixed;
      }
    }
    delete[] dp->Variables;
  }
  delete[] this->DataDumps;
  delete[] this->Blocks;
  this->SetFileName(0);
  this->SetCellArraySelection(0);

  if (this->MarkersOn)
  {
    for (int n = 0; n < this->NumberOfMaterials; n++)
    {
      if (this->Markers[n].NumMarks <= 0)
      {
        continue;
      }
      this->MarkersDumps[n].XLoc->Delete();
      this->MarkersDumps[n].ILoc->Delete();
      if (this->NumberOfDimensions > 1)
      {
        this->MarkersDumps[n].YLoc->Delete();
        this->MarkersDumps[n].JLoc->Delete();
      }
      if (this->NumberOfDimensions > 2)
      {
        this->MarkersDumps[n].ZLoc->Delete();
        this->MarkersDumps[n].KLoc->Delete();
      }
      this->MarkersDumps[n].Block->Delete();

      for (int v = 0; v < this->Markers[n].NumVars; v++)
      {
        this->MarkersDumps[n].Variables[v]->Delete();
      }
      delete[] this->MarkersDumps[n].Variables;
      delete[] this->Markers[n].Variables;
    }
    delete[] this->Markers;
  }
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::IsVolumeFraction(Variable* var)
{
  // BUG #13473: Matches old style "Material volume fraction" as well as new
  // style "Volume Fraction" variable names.
  vtksys::RegularExpression re("[vV]olume [fF]raction");
  return re.find(var->Name) ? 1 : 0;
}

//-----------------------------------------------------------------------------
void vtkSpyPlotUniReader::SetDownConvertVolumeFraction(int vf)
{
  if (this->DownConvertVolumeFraction == vf)
  {
    return;
  }
  this->DownConvertVolumeFraction = vf;
  this->DataTypeChanged = 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::MakeCurrent()
{
  if (!(this->NeedToCheck || (this->GeomTimeStep != this->CurrentTimeStep)))
  {
    // Nothing needs to be done
    return 1;
  }

  if (!this->HaveInformation)
  {
    vtkDebugMacro(<< __LINE__ << " " << this << " Read: " << this->HaveInformation);

    if (!this->ReadInformation())
    {
      return 0;
    }
  }

  std::vector<unsigned char> arrayBuffer;
  vtksys::ifstream ifs(this->FileName, ios::binary | ios::in);
  vtkSpyPlotIStream spis;
  spis.SetStream(&ifs);
  int dump;
  vtkSpyPlotUniReader::DataDump* dp;
  int blocksUpdated = 0;
  int needMarkers = this->GenerateMarkers && this->MarkersOn;

  // Do we have to update blocks
  if (this->GeomTimeStep != this->CurrentTimeStep)
  {
    int block;
    this->GeomTimeStep = this->CurrentTimeStep;
    blocksUpdated = 1;
    dump = this->CurrentTimeStep;
    dp = this->DataDumps + dump;
    // vtkDebugMacro( "Dump: " << dump << " / "
    // << this->NumberOfDataDumps << " at time: " << this->DumpTime[dump] );

    // Load in the grid block information
    // Advance the stream to where the block definitions are
    spis.Seek(dp->BlocksOffset);
    for (block = 0; block < dp->NumberOfBlocks; ++block)
    {
      // long l = ifs.tellg();
      vtkSpyPlotBlock* b = &(this->Blocks[block]);
      if (!b->Read(this->IsAMR(), this->FileVersion, &spis))
      {
        vtkErrorMacro("Problem reading the block information");
        return 0;
      }
    }

    // Advance the stream to where the block geometries are
    spis.Seek(dp->SavedBlocksGeometryOffset);
    for (block = 0; block < dp->NumberOfBlocks; ++block)
    {
      vtkSpyPlotBlock* b = &(this->Blocks[block]);
      if (b->IsAllocated())
      {
        int numBytes;
        int component;
        // vtkDebugMacro( "Block: " << block );
        for (component = 0; component < 3; ++component)
        {
          if (!spis.ReadInt32s(&numBytes, 1))
          {
            vtkErrorMacro("Problem reading the number of bytes");
            return 0;
          }
          // vtkDebugMacro( "  Number of bytes for " << component << ": "
          // << numBytes );
          if (static_cast<int>(arrayBuffer.size()) < numBytes)
          {
            arrayBuffer.resize(numBytes);
          }

          if (!spis.ReadString(&*arrayBuffer.begin(), numBytes))
          {
            vtkErrorMacro("Problem reading the bytes");
            return 0;
          }
          if (!b->SetGeometry(component, &*arrayBuffer.begin(), numBytes))
          {
            vtkErrorMacro("Problem RLD decoding rectilinear grid array: " << component);
            return 0;
          }
          vtkDebugMacro(" " << b << " geometry initialized");
        }
      }
    }
  }

  if (!this->NeedToCheck)
  {
    return 1;
  }

  this->NeedToCheck = 0;

  for (dump = 0; dump < this->NumberOfDataDumps; ++dump)
  {
    if (dump != this->CurrentTimeStep)
    {
      dp = this->DataDumps + dump;
      int var;
      for (var = 0; var < dp->NumVars; ++var)
      {
        vtkSpyPlotUniReader::Variable* cv = dp->Variables + var;
        if (cv->DataBlocks)
        {
          int ca;
          for (ca = 0; ca < dp->ActualNumberOfBlocks; ++ca)
          {
            if (cv->DataBlocks[ca])
            {
              cv->DataBlocks[ca]->Delete();
              cv->DataBlocks[ca] = 0;
            }
          }
          vtkDebugMacro("* Delete Data blocks for variable: " << cv->Name);
          delete[] cv->DataBlocks;
          cv->DataBlocks = 0;
          delete[] cv->GhostCellsFixed;
          cv->GhostCellsFixed = 0;
        }
      }
    }
  }

  dump = this->CurrentTimeStep;
  dp = this->DataDumps + dump;

  for (int fieldCnt = 0; fieldCnt < dp->NumVars; ++fieldCnt)
  {
    vtkSpyPlotUniReader::Variable* var = dp->Variables + fieldCnt;
    vtkDebugMacro("Variable: " << var << " (" << var->Name << ") - " << fieldCnt
                               << " (file: " << this->FileName << ") ");

    // Do we need to create new data blocks
    int blocksExists = 0;
    if (var->DataBlocks)
    {
      vtkDebugMacro(" *** Looks like variable: " << var->Name << " is already loaded");
      blocksExists = 1;
    }
    // Did we create data blocks that we do not need any more
    if ((!this->CellArraySelection->ArrayIsEnabled(var->Name)) ||
      (this->DataTypeChanged && this->IsVolumeFraction(var)))
    {
      if (var->DataBlocks)
      {
        vtkDebugMacro(" ** Variable " << var->Name << " was unselected, so remove");
        int dataBlock;
        for (dataBlock = 0; dataBlock < dp->ActualNumberOfBlocks; ++dataBlock)
        {
          var->DataBlocks[dataBlock]->Delete();
          var->DataBlocks[dataBlock] = 0;
        }
        delete[] var->DataBlocks;
        var->DataBlocks = 0;
        delete[] var->GhostCellsFixed;
        var->GhostCellsFixed = 0;
        vtkDebugMacro("* Delete Data blocks for variable: " << var->Name);
      }
      vtkDebugMacro(" *** Ignore variable: " << var->Name);
      if (!this->CellArraySelection->ArrayIsEnabled(var->Name))
      {
        continue;
      }
    }

    if ((needMarkers || this->CellArraySelection->ArrayIsEnabled(var->Name)) && !var->DataBlocks)
    {
      vtkDebugMacro(
        " ** Allocate new space for variable: " << var->Name << " - " << this->FileName);
      var->DataBlocks = new vtkDataArray*[dp->ActualNumberOfBlocks];
      memset(var->DataBlocks, 0, dp->ActualNumberOfBlocks * sizeof(vtkDataArray*));
      var->GhostCellsFixed = new int[dp->ActualNumberOfBlocks];
      memset(var->GhostCellsFixed, 0, dp->ActualNumberOfBlocks * sizeof(int));
      vtkDebugMacro(" Allocate DataBlocks: " << var->DataBlocks);
      blocksExists = 0;
    }

    if (blocksExists)
    {
      vtkDebugMacro(<< var << " Skip reading of variable: " << var->Name << " / "
                    << this->FileName);
      continue;
    }

    // vtkDebugMacro( "  Field: " << fieldCnt << " / " << dp->NumVars
    // << " [" << var->Name << "]" );
    // vtkDebugMacro( "    Jump to: " << dp->SavedVariableOffsets[fieldCnt] );
    spis.Seek(dp->SavedVariableOffsets[fieldCnt]);
    int numBytes;
    int block;
    int actualBlockId = 0;
    for (block = 0; block < dp->NumberOfBlocks; ++block)
    {
      vtkSpyPlotBlock* bk = this->Blocks + block;
      if (bk->IsAllocated())
      {
        vtkFloatArray* floatArray = 0;
        vtkUnsignedCharArray* unsignedCharArray = 0;
        vtkDataArray* dataArray = 0;
        if (this->CellArraySelection->ArrayIsEnabled(var->Name) && !var->DataBlocks[actualBlockId])
        {
          if (this->DownConvertVolumeFraction && this->IsVolumeFraction(var))
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
          dataArray->SetNumberOfTuples(
            bk->GetDimension(0) * bk->GetDimension(1) * bk->GetDimension(2));
          dataArray->SetName(var->Name);
          // vtkDebugMacro( "*** Create data array: "
          // << dataArray->GetNumberOfTuples() );
        }
        int zax;
        int bdims[3];
        bk->GetDimensions(bdims);
        for (zax = 0; zax < bdims[2]; ++zax)
        {
          int planeSize = bdims[0] * bdims[1];
          if (!spis.ReadInt32s(&numBytes, 1))
          {
            vtkErrorMacro("Problem reading the number of bytes");
            return 0;
          }
          if (static_cast<int>(arrayBuffer.size()) < numBytes)
          {
            arrayBuffer.resize(numBytes);
          }
          if (!spis.ReadString(&*arrayBuffer.begin(), numBytes))
          {
            vtkErrorMacro("Problem reading the bytes");
            return 0;
          }
          if (floatArray)
          {
            float* ptr = floatArray->GetPointer(zax * planeSize);
            if (!this->RunLengthDataDecode(&*arrayBuffer.begin(), numBytes, ptr, planeSize))
            {
              vtkErrorMacro("Problem RLD decoding float data array");
              return 0;
            }
          }
          if (unsignedCharArray)
          {
            unsigned char* ptr = unsignedCharArray->GetPointer(zax * planeSize);
            if (!this->RunLengthDataDecode(&*arrayBuffer.begin(), numBytes, ptr, planeSize))
            {
              vtkErrorMacro("Problem RLD decoding unsigned char data array");
              return 0;
            }
          }
        }
        if (dataArray)
        {
          var->DataBlocks[actualBlockId] = dataArray;
          var->GhostCellsFixed[actualBlockId] = 0;
          vtkDebugMacro(" " << dataArray << " initialized: " << dataArray->GetName());
          actualBlockId++;
        }
      }
    }
  }

  if (blocksUpdated && needMarkers)
  {
    if (this->ReadMarkerDumps(&spis) == 0)
    {
      vtkErrorMacro("Problem reading marker data");
      return 0;
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
  for (cc = 0; cc < this->NumberOfPossibleCellFields; ++cc)
  {
    total += sizeof(this->CellFields[cc]);
  }
  cout << "cell fields: " << total << endl;
  total = 0;
  for (cc = 0; cc < this->NumberOfPossibleMaterialFields; ++cc)
  {
    total += sizeof(this->MaterialFields[cc]);
  }
  cout << "material fields: " << total << endl;
}

//-----------------------------------------------------------------------------
void vtkSpyPlotUniReader::PrintInformation()
{
  if (!this->HaveInformation)
  {
    this->ReadInformation();
  }
  vtkDebugMacro("FileDescription: \"");
  size_t cc;
  for (cc = 0; cc < 128; ++cc)
  {
    if (!this->FileDescription[cc])
    {
      break;
    }
  }
  // cout.write(this->FileDescription, cc);
  vtkDebugMacro("\"");

  vtkDebugMacro("FileVersion:        " << this->FileVersion);
  vtkDebugMacro("FileCompressionFlag:    " << this->FileCompressionFlag);
  vtkDebugMacro("FileProcessorId:        " << this->FileProcessorId);
  vtkDebugMacro("NumberOfProcessors: " << this->NumberOfProcessors);
  vtkDebugMacro("IGM:                " << this->IGM);
  vtkDebugMacro("NumberOfDimensions:            " << this->NumberOfDimensions);
  vtkDebugMacro("NumberOfMaterials:             " << this->NumberOfMaterials);
  vtkDebugMacro("MaximumNumberOfMaterials:             " << this->MaximumNumberOfMaterials);
  vtkDebugMacro("GMin:               " << this->GlobalMin[0] << ", " << this->GlobalMin[1] << ", "
                                       << this->GlobalMin[2]);
  vtkDebugMacro("GMax:               " << this->GlobalMax[0] << ", " << this->GlobalMax[1] << ", "
                                       << this->GlobalMax[2]);
  vtkDebugMacro("NumberOfBlocks:          " << this->NumberOfBlocks);
  vtkDebugMacro("MaximumNumberOfLevels:          " << this->MaximumNumberOfLevels);
  vtkDebugMacro("NumberOfPossibleCellFields:      " << this->NumberOfPossibleCellFields);

  vtkDebugMacro("Cell fields: ");
  int fieldCnt;
  for (fieldCnt = 0; fieldCnt < this->NumberOfPossibleCellFields; ++fieldCnt)
  {
    vtkDebugMacro("Cell field " << fieldCnt);
    vtkDebugMacro("  Id:      " << this->CellFields[fieldCnt].Id);
    vtkDebugMacro("  Comment: " << this->CellFields[fieldCnt].Comment);
    vtkDebugMacro("  Index:     " << this->CellFields[fieldCnt].Index);
  }

  vtkDebugMacro("Material fields: ");
  for (fieldCnt = 0; fieldCnt < this->NumberOfPossibleMaterialFields; ++fieldCnt)
  {
    vtkDebugMacro("Material field " << fieldCnt);
    vtkDebugMacro("  Id:      " << this->MaterialFields[fieldCnt].Id);
    vtkDebugMacro("  Comment: " << this->MaterialFields[fieldCnt].Comment);
    vtkDebugMacro("  Index:     " << this->MaterialFields[fieldCnt].Index);
  }

  vtkDebugMacro("Cumulative number of dumps: " << this->NumberOfDataDumps);
  int dump;
  for (dump = 0; dump < this->NumberOfDataDumps; ++dump)
  {
    vtkDebugMacro(" Dump " << dump << " cycle: " << this->DumpCycle[dump] << " time: "
                           << this->DumpTime[dump] << " offset: " << this->DumpOffset[dump]);
  }

  vtkDebugMacro("Headers: ");
  for (dump = 0; dump < this->NumberOfDataDumps; ++dump)
  {
    vtkSpyPlotUniReader::DataDump* dp = this->DataDumps + dump;
    vtkDebugMacro("  " << dump);
    vtkTypeInt64 offset = this->DumpOffset[dump];
    static_cast<void>(offset); // unused-variable warning.
    vtkDebugMacro("    offset:   " << offset << " number of variables: " << dp->NumVars);
    int var;
    for (var = 0; var < dp->NumVars; ++var)
    {
      vtkDebugMacro(
        "      Variable: " << dp->SavedVariables[var] << " -> " << dp->SavedVariableOffsets[var]);
    }

    int block;
    int bdims[3];
    vtkFloatArray* bvecs[3];
    vtkDebugMacro("Blocks: ");
    for (block = 0; block < dp->NumberOfBlocks; ++block)
    {
      vtkSpyPlotBlock* b = this->Blocks + block;
      b->GetDimensions(bdims);
      vtkDebugMacro("  " << block);
      vtkDebugMacro("    Allocated: " << b->IsAllocated());
      vtkDebugMacro("    Active: " << b->IsActive());
      vtkDebugMacro("    Level: " << b->GetLevel());
      vtkDebugMacro("    Dims[0]: " << bdims[0]);
      vtkDebugMacro("    Dims[1]: " << bdims[1]);
      vtkDebugMacro("    Dims[2]: " << bdims[2]);
      vtkDebugMacro("    AMR: " << b->IsAMR());
      if (b->IsAllocated())
      {
        int num;
        b->GetVectors(bvecs);
        vtkDebugMacro("    XYZArrays[0]:");
        for (num = 0; num <= bdims[0]; num++)
        {
          vtkDebugMacro(" " << bvecs[0]->GetValue(num));
        }
        vtkDebugMacro("    XYZArrays[1]:");
        for (num = 0; num <= bdims[1]; num++)
        {
          vtkDebugMacro(" " << bvecs[1]->GetValue(num));
        }
        vtkDebugMacro("    XYZArray[2]:");
        for (num = 0; num <= bdims[2]; num++)
        {
          vtkDebugMacro(" " << bvecs[2]->GetValue(num));
        }
      }
    }

    for (fieldCnt = 0; fieldCnt < dp->NumVars; ++fieldCnt)
    {
      vtkSpyPlotUniReader::Variable* currentVar = dp->Variables + fieldCnt;
      vtkDebugMacro("   Variable: " << fieldCnt << " - \"" << currentVar->Name
                                    << "\" Material: " << currentVar->Material);
      if (currentVar->DataBlocks)
      {
        int dataBlock;
        for (dataBlock = 0; dataBlock < dp->ActualNumberOfBlocks; ++dataBlock)
        {
          vtkDebugMacro("      DataBlock: " << dataBlock);
          if (currentVar->DataBlocks[dataBlock])
          {
            currentVar->DataBlocks[dataBlock]->Print(cout);
          }
          vtkDebugMacro("      Ghost cells fixed: " << currentVar->GhostCellsFixed[dataBlock]);
        }
      }
      else
      {
        vtkDebugMacro("      Not read");
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

//-----------------------------------------------------------------------------
template <class t>
int vtkSpyPlotUniReaderRunLengthDataDecode(
  vtkSpyPlotUniReader* self, const unsigned char* in, int inSize, t* out, int outSize, t scale = 1)
{
  int outIndex = 0, inIndex = 0;

  const unsigned char* ptmp = in;

  /* Run-length decode */
  while ((outIndex < outSize) && (inIndex < inSize))
  {
    // Okay get the run length
    unsigned char runLength = *ptmp;
    ptmp++;
    if (runLength < 128)
    {
      float val;
      memcpy(&val, ptmp, sizeof(float));
      vtkByteSwap::SwapBE(&val);
      ptmp += 4;
      // Now populate the out data
      int k;
      for (k = 0; k < runLength; ++k)
      {
        if (outIndex >= outSize)
        {
          vtkErrorWithObjectMacro(self, "Problem doing RLD decode. "
              << "Too much data generated. Expected: " << outSize);
          return 0;
        }
        out[outIndex] = static_cast<t>(val * scale);
        outIndex++;
      }
      inIndex += 5;
    }
    else // runLength >= 128
    {
      int k;
      for (k = 0; k < runLength - 128; ++k)
      {
        if (outIndex >= outSize)
        {
          vtkErrorWithObjectMacro(self, "Problem doing RLD decode. "
              << "Too much data generated. Expected: " << outSize);
          return 0;
        }
        float val;
        memcpy(&val, ptmp, sizeof(float));
        vtkByteSwap::SwapBE(&val);
        out[outIndex] = static_cast<t>(val * scale);
        outIndex++;
        ptmp += 4;
      }
      inIndex += 4 * (runLength - 128) + 1;
    }
  } // while

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::RunLengthDataDecode(
  const unsigned char* in, int inSize, float* out, int outSize)
{
  return ::vtkSpyPlotUniReaderRunLengthDataDecode(this, in, inSize, out, outSize);
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::RunLengthDataDecode(
  const unsigned char* in, int inSize, int* out, int outSize)
{
  return ::vtkSpyPlotUniReaderRunLengthDataDecode(this, in, inSize, out, outSize);
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::RunLengthDataDecode(
  const unsigned char* in, int inSize, unsigned char* out, int outSize)
{
  return ::vtkSpyPlotUniReaderRunLengthDataDecode(
    this, in, inSize, out, outSize, static_cast<unsigned char>(255));
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::SetCurrentTime(double time)
{
  if (!this->HaveInformation)
  {
    vtkDebugMacro(<< __LINE__ << " " << this << " Read: " << this->HaveInformation);
    this->ReadInformation();
  }

  if (time < this->TimeRange[0] || time > this->TimeRange[1])
  {
    vtkWarningMacro("Requested time: " << time << " is outside of reader's range ["
                                       << this->TimeRange[0] << ", " << this->TimeRange[1] << "]");
    return 0;
  }
  this->CurrentTime = time;
  this->CurrentTimeStep = this->GetTimeStepFromTime(time);
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::SetCurrentTimeStep(int timeStep)
{
  if (!this->HaveInformation)
  {
    vtkDebugMacro(<< __LINE__ << " " << this << " Read: " << this->HaveInformation);
    this->ReadInformation();
  }

  if (timeStep < this->TimeStepRange[0] || timeStep > this->TimeStepRange[1])
  {
    vtkWarningMacro("Requested time step: " << timeStep << " is outside of reader's range ["
                                            << this->TimeStepRange[0] << ", "
                                            << this->TimeStepRange[1] << "]");
    return 0;
  }
  this->CurrentTimeStep = timeStep;
  this->CurrentTime = this->GetTimeFromTimeStep(timeStep);
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::GetTimeStepFromTime(double time)
{
  if (!this->HaveInformation)
  {
    vtkDebugMacro(<< __LINE__ << " " << this << " Read: " << this->HaveInformation);
    this->ReadInformation();
  }

  // int dump;
  // for ( dump = 0; dump < this->NumberOfDataDumps; ++ dump )
  //  {
  //  if ( time < this->DumpTime[dump] )
  //    {
  //    return dump-1;
  //    }
  //  }
  int cnt = 0;
  int closestStep = 0;
  double minDist = -1;
  for (cnt = 0; cnt < this->NumberOfDataDumps; cnt++)
  {
    double tdist = (this->DumpTime[cnt] - time > time - this->DumpTime[cnt])
      ? this->DumpTime[cnt] - time
      : time - this->DumpTime[cnt];
    if (minDist < 0 || tdist < minDist)
    {
      minDist = tdist;
      closestStep = cnt;
    }
  }
  return closestStep;
  // return this->TimeStepRange[1];
}

//-----------------------------------------------------------------------------
double vtkSpyPlotUniReader::GetTimeFromTimeStep(int timeStep)
{
  if (!this->HaveInformation)
  {
    vtkDebugMacro(<< __LINE__ << " " << this << " Read: " << this->HaveInformation);
    this->ReadInformation();
  }

  if (timeStep < this->TimeStepRange[0])
  {
    return this->TimeRange[0];
  }
  if (timeStep > this->TimeStepRange[1])
  {
    return this->TimeRange[1];
  }
  return this->DumpTime[timeStep];
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::GetNumberOfDataBlocks()
{
  if (!this->HaveInformation)
  {
    vtkDebugMacro(<< __LINE__ << " " << this << " Read: " << this->HaveInformation);
    this->ReadInformation();
  }

  return this->DataDumps[this->CurrentTimeStep].ActualNumberOfBlocks;
}

//-----------------------------------------------------------------------------
vtkSpyPlotBlock* vtkSpyPlotUniReader::GetBlock(int block)
{
  if (!this->HaveInformation)
  {
    vtkDebugMacro(<< __LINE__ << " " << this << " Read: " << this->HaveInformation);
    if (!this->ReadInformation())
      return 0;
  }
  int cb = 0;
  int blockId;
  for (blockId = 0; blockId < this->NumberOfBlocks; ++blockId)
  {
    if (this->Blocks[blockId].IsAllocated())
    {
      if (cb == block)
      {
        return this->Blocks + blockId;
      }
      cb++;
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
vtkSpyPlotUniReader::Variable* vtkSpyPlotUniReader::GetCellField(int field)
{
  if (!this->HaveInformation)
  {
    vtkDebugMacro(<< __LINE__ << " " << this << " Read: " << this->HaveInformation);
    this->ReadInformation();
  }

  vtkSpyPlotUniReader::DataDump* dp = this->DataDumps + this->CurrentTimeStep;
  if (field < 0 || field >= dp->NumVars)
  {
    return 0;
  }
  return dp->Variables + field;
}

//-----------------------------------------------------------------------------
const char* vtkSpyPlotUniReader::GetCellFieldName(int field)
{
  vtkSpyPlotUniReader::Variable* var = this->GetCellField(field);
  if (!var)
  {
    return 0;
  }
  return var->Name;
}

//-----------------------------------------------------------------------------
vtkDataArray* vtkSpyPlotUniReader::GetCellFieldData(int block, int field, int* fixed)
{
  vtkSpyPlotUniReader::DataDump* dp = this->DataDumps + this->CurrentTimeStep;
  if (block < 0 || block > dp->ActualNumberOfBlocks)
  {
    return 0;
  }
  vtkSpyPlotUniReader::Variable* var = this->GetCellField(field);
  if (!var)
  {
    return 0;
  }

  *fixed = var->GhostCellsFixed[block];

  vtkDebugMacro(
    "GetCellField(" << block << " " << field << " " << *fixed << ") = " << var->DataBlocks[block]);
  return var->DataBlocks[block];
}

//-----------------------------------------------------------------------------
vtkDataArray* vtkSpyPlotUniReader::GetMaterialField(
  const int& block, const int& materialIndex, const char* id)
{
  vtkSpyPlotUniReader::Variable* var = NULL;
  vtkSpyPlotUniReader::DataDump* dp = this->DataDumps + this->CurrentTimeStep;
  for (int v = 0; v < dp->NumVars; ++v)
  {
    var = &dp->Variables[v];
    if (strcmp(var->MaterialField->Id, id) == 0)
    {
      if (var->Index == materialIndex && var->DataBlocks != NULL)
      {
        return var->DataBlocks[block];
      }
    }
  }
  return NULL;
}
//-----------------------------------------------------------------------------
vtkDataArray* vtkSpyPlotUniReader::GetMaterialMassField(const int& block, const int& materialIndex)
{
  return this->GetMaterialField(block, materialIndex, "M");
}

//-----------------------------------------------------------------------------
vtkDataArray* vtkSpyPlotUniReader::GetMaterialVolumeFractionField(
  const int& block, const int& materialIndex)
{
  return this->GetMaterialField(block, materialIndex, "VOLM");
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::MarkCellFieldDataFixed(int block, int field)
{
  vtkSpyPlotUniReader::DataDump* dp = this->DataDumps + this->CurrentTimeStep;
  if (block < 0 || block > dp->ActualNumberOfBlocks)
  {
    return 0;
  }
  vtkSpyPlotUniReader::Variable* var = this->GetCellField(field);
  if (!var)
  {
    return 0;
  }
  var->GhostCellsFixed[block] = 1;
  vtkDebugMacro(" " << var->DataBlocks[block] << " fixed: " << var->DataBlocks[block]->GetName());
  return 1;
}

//-----------------------------------------------------------------------------
vtkFloatArray* vtkSpyPlotUniReader::GetTracers()
{
  vtkSpyPlotUniReader::DataDump* dp = this->DataDumps + this->CurrentTimeStep;
  if (dp->NumberOfTracers > 0)
  {
    vtkDebugMacro("GetTracers() = " << dp->TracerCoord);
    return dp->TracerCoord;
  }
  else
  {
    vtkDebugMacro("GetTracers() = " << 0);
    return 0;
  }
}
//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::ReadHeader(vtkSpyPlotIStream* spis)
{
  vtkDebugMacro(<< this << " Reading file header: " << this->FileName);

  // Ok, file is open, so read the header
  char magic[8];
  if (!spis->ReadString(magic, 8))
  {
    vtkErrorMacro("Cannot read magic");
    return 0;
  }
  if (strncmp(magic, "spydata", 7) != 0 || magic[7] != 0)
  {
    vtkErrorMacro("Bad magic: " << vtkSpyPlotWriteString(magic, 7));
    return 0;
  }
  if (!spis->ReadString(this->FileDescription, 128))
  {
    vtkErrorMacro("Cannot read FileDescription");
    return 0;
  }
  // printf("here: %ld\n", ifs.tellg());
  if (!spis->ReadInt32s(&(this->FileVersion), 1))
  {
    vtkErrorMacro("Cannot read file version");
    return 0;
  }
  // cout << "File version: " << this->FileVersion << endl;
  if (this->FileVersion >= 102)
  {
    if (!spis->ReadInt32s(&(this->SizeOfFilePointer), 1))
    {
      vtkErrorMacro("Cannot read the seize of file pointer");
      return 0;
    }
    switch (this->SizeOfFilePointer)
    {
      case 32:
      case 64:
        break;
      default:
        vtkErrorMacro("Unknown size of file pointer: " << this->SizeOfFilePointer
                                                       << ". Only handle 32 and 64 bit sizes.");
        return 0;
    }
    // cout << "File pointer size: " << this->SizeOfFilePointer << endl;
  }
  if (!spis->ReadInt32s(&(this->FileCompressionFlag), 1))
  {
    vtkErrorMacro("Cannot read compression flag");
    return 0;
  }
  if (!spis->ReadInt32s(&(this->FileProcessorId), 1))
  {
    vtkErrorMacro("Cannot read processor id");
    return 0;
  }
  if (!spis->ReadInt32s(&(this->NumberOfProcessors), 1))
  {
    vtkErrorMacro("Cannot read number of processors");
    return 0;
  }
  if (!spis->ReadInt32s(&(this->IGM), 1))
  {
    vtkErrorMacro("Cannot read IGM");
    return 0;
  }
  if (!spis->ReadInt32s(&(this->NumberOfDimensions), 1))
  {
    vtkErrorMacro("Cannot read number of dimensions");
    return 0;
  }
  if (!spis->ReadInt32s(&(this->NumberOfMaterials), 1))
  {
    vtkErrorMacro("Cannot read number of materials");
    return 0;
  }
  if (!spis->ReadInt32s(&(this->MaximumNumberOfMaterials), 1))
  {
    vtkErrorMacro("Cannot read maximum number of materials");
    return 0;
  }
  // printf("here: %ld\n", ifs.tellg());
  if (!spis->ReadDoubles(this->GlobalMin, 3))
  {
    vtkErrorMacro("Cannot read global min");
    return 0;
  }
  if (!spis->ReadDoubles(this->GlobalMax, 3))
  {
    vtkErrorMacro("Cannot read global max");
    return 0;
  }
  // printf("here: %ld\n", ifs.tellg());
  if (!spis->ReadInt32s(&(this->NumberOfBlocks), 1))
  {
    vtkErrorMacro("Cannot read number of blocks");
    return 0;
  }
  if (this->FileVersion >= 105)
  {
    if (!spis->ReadInt32s(&(this->MarkersOn), 1))
    {
      vtkErrorMacro("Cannot reader marker flag");
      return 0;
    }
    if (this->MarkersOn)
    {
      this->ReadMarkerHeader(spis);
    }
  }
  if (!spis->ReadInt32s(&(this->MaximumNumberOfLevels), 1))
  {
    vtkErrorMacro("Cannot read maximum number of levels");
    return 0;
  }
  if (this->FileVersion >= 105)
  {
    int junk[5];
    if (!spis->ReadInt32s(junk, 5))
    {
      vtkErrorMacro("Cannot read fill bytes at the end of the header");
      return 0;
    }
  }
  // Done with header
  return 1;
}

int vtkSpyPlotUniReader::ReadMarkerHeader(vtkSpyPlotIStream* spis)
{
  this->Markers = new MaterialMarker[this->NumberOfMaterials];
  this->MarkersDumps = new MarkerDump[this->NumberOfMaterials];
  for (int n = 0; n < this->NumberOfMaterials; n++)
  {
    int numMarks;
    if (!spis->ReadInt32s(&numMarks, 1))
    {
      vtkErrorMacro("Unable to read number of marks");
      return 0;
    }
    int numRealMarks;
    if (!spis->ReadInt32s(&numRealMarks, 1))
    {
      vtkErrorMacro("Unable to read real number of marks");
      return 0;
    }
    int junk[3];
    spis->ReadInt32s(junk, 3);

    this->Markers[n].NumMarks = numMarks;
    this->Markers[n].NumRealMarks = numRealMarks;

    if (numMarks > 0)
    {
      int numVars;
      if (!spis->ReadInt32s(&numVars, 1))
      {
        vtkErrorMacro("Unable to read number of marker variables");
        return 0;
      }
      this->Markers[n].NumVars = numVars;
      this->MarkersDumps[n].XLoc = vtkFloatArray::New();
      this->MarkersDumps[n].ILoc = vtkIntArray::New();
      this->MarkersDumps[n].YLoc = vtkFloatArray::New();
      this->MarkersDumps[n].JLoc = vtkIntArray::New();
      this->MarkersDumps[n].ZLoc = vtkFloatArray::New();
      this->MarkersDumps[n].KLoc = vtkIntArray::New();
      this->MarkersDumps[n].Block = vtkIntArray::New();

      this->Markers[n].Variables = new MarkerMaterialField[this->Markers[n].NumVars];
      this->MarkersDumps[n].Variables = new vtkFloatArray*[this->Markers[n].NumVars];

      for (int v = 0; v < numVars; v++)
      {
        char name[30];
        if (!spis->ReadString(name, 30))
        {
          vtkErrorMacro("Unable to read marker variable name");
          return 0;
        }
        char label[256];
        if (!spis->ReadString(label, 256))
        {
          vtkErrorMacro("Unable to read marker label name");
          return 0;
        }

        strncpy(this->Markers[n].Variables[v].Name, name, 30);
        strncpy(this->Markers[n].Variables[v].Label, label, 256);
        this->MarkersDumps[n].Variables[v] = vtkFloatArray::New();
      }
    }
  }
  return 1;
}

int vtkSpyPlotUniReader::ReadGroupHeaderInformation(vtkSpyPlotIStream* spis)
{
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
    int* DumpCycle;
    double* DumpTime;
    double* DumpDT; // SPCTH 102 What is this anyway?
    vtkTypeInt64* DumpOffset;
  };

  while (1)
  {
    GroupHeader gh;
    if (!spis->ReadInt64s(&(gh.Offset), 1))
    {
      vtkErrorMacro("Cannot get group header offset");
      return 0;
    }
    vtkTypeInt64 cpos = spis->Tell();
    // vtkDebugMacro( "position: " << cpos );
    // vtkDebugMacro( "offset:   " << gh.Offset );
    if (cpos > gh.Offset)
    {
      vtkErrorMacro("The offset is back in file: " << cpos << " > " << gh.Offset);
      return 0;
    }
    spis->Seek(gh.Offset);
    if (!spis->ReadInt32s(&(gh.NumberOfDataDumps), 1))
    {
      vtkErrorMacro("Problem reading the num dumps");
      return 0;
    }
    if (!spis->ReadInt32s(gh.DumpCycle, MAX_DUMPS))
    {
      vtkErrorMacro("Problem reading the dump cycle");
      return 0;
    }
    if (!spis->ReadDoubles(gh.DumpTime, MAX_DUMPS))
    {
      vtkErrorMacro("Problem reading the dump times");
      return 0;
    }
    if (this->FileVersion >= 102)
    {
      // cout << "This is SPCTH " << this->FileVersion
      // << " so read DumpDT's" << endl;
      if (!spis->ReadDoubles(gh.DumpDT, MAX_DUMPS))
      {
        vtkErrorMacro("Problem reading the dump DT's");
        return 0;
      }
    }
    if (!spis->ReadInt64s(gh.DumpOffset, MAX_DUMPS))
    {
      vtkErrorMacro("Problem reading the dump offsets");
      return 0;
    }
    // vtkDebugMacro( "Number of dumps: " << gh.NumberOfDataDumps );
    // for ( dump = 0; dump < gh.NumberOfDataDumps; ++ dump )
    //  {
    //  vtkDebugMacro( " Dump " << dump << " cycle: " << gh.DumpCycle[dump]
    //                          << " time: " << gh.DumpTime[dump]
    //                          << " offset: "
    //                          << gh.DumpOffset[dump] );
    //  }
    CummulativeGroupHeader nch;
    nch.NumberOfDataDumps = this->NumberOfDataDumps + gh.NumberOfDataDumps;
    nch.DumpCycle = new int[nch.NumberOfDataDumps];
    nch.DumpTime = new double[nch.NumberOfDataDumps];
    nch.DumpDT = NULL;
    if (this->FileVersion >= 102)
    {
      nch.DumpDT = new double[nch.NumberOfDataDumps];
    }
    nch.DumpOffset = new vtkTypeInt64[nch.NumberOfDataDumps];
    if (this->DumpCycle)
    {
      memcpy(nch.DumpCycle, this->DumpCycle, this->NumberOfDataDumps * sizeof(int));
      memcpy(nch.DumpTime, this->DumpTime, this->NumberOfDataDumps * sizeof(double));
      if (this->FileVersion >= 102)
      {
        memcpy(nch.DumpDT, this->DumpDT, this->NumberOfDataDumps * sizeof(double));
      }
      memcpy(nch.DumpOffset, this->DumpOffset, this->NumberOfDataDumps * sizeof(vtkTypeInt64));
      delete[] this->DumpCycle;
      delete[] this->DumpTime;
      if (this->FileVersion >= 102)
      {
        delete[] this->DumpDT;
      }
      delete[] this->DumpOffset;
    }
    memcpy(
      nch.DumpCycle + this->NumberOfDataDumps, gh.DumpCycle, gh.NumberOfDataDumps * sizeof(int));
    memcpy(
      nch.DumpTime + this->NumberOfDataDumps, gh.DumpTime, gh.NumberOfDataDumps * sizeof(double));
    if (this->FileVersion >= 102)
    {
      memcpy(
        nch.DumpDT + this->NumberOfDataDumps, gh.DumpDT, gh.NumberOfDataDumps * sizeof(double));
    }
    memcpy(nch.DumpOffset + this->NumberOfDataDumps, gh.DumpOffset,
      gh.NumberOfDataDumps * sizeof(vtkTypeInt64));

    this->NumberOfDataDumps = nch.NumberOfDataDumps;
    this->DumpCycle = nch.DumpCycle;
    this->DumpTime = nch.DumpTime;
    if (this->FileVersion >= 102)
    {
      this->DumpDT = nch.DumpDT;
    }
    this->DumpOffset = nch.DumpOffset;
    memset(&nch, 0, sizeof(nch));
    if (gh.NumberOfDataDumps != MAX_DUMPS)
    {
      break;
    }
  }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSpyPlotUniReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "TimeStepRange: [" << this->TimeStepRange[0] << ", " << this->TimeStepRange[1]
     << "]" << endl;
  os << indent << "CurrentTimeStep: " << this->CurrentTimeStep << endl;
  os << indent << "TimeRange: [" << this->TimeRange[0] << ", " << this->TimeRange[1] << "]" << endl;
  os << indent << "CurrentTime: " << this->CurrentTime << endl;
  os << indent << "DataTypeChanged: " << this->DataTypeChanged << endl;
  os << indent << "NumberOfCellFields: " << this->NumberOfCellFields << endl;
  os << indent << "NeedToCheck: " << this->NeedToCheck << endl;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::ReadInformation()
{

  if (this->HaveInformation)
  {
    return 1;
  }
  // Initial checks
  if (!this->CellArraySelection)
  {
    vtkErrorMacro("Cell array selection not specified");
    return 0;
  }
  if (!this->FileName)
  {
    vtkErrorMacro("FileName not specified");
    return 0;
  }
  vtksys::ifstream ifs(this->FileName, ios::binary | ios::in);
  if (!ifs)
  {
    vtkErrorMacro("Cannot open file: " << this->FileName);
    return 0;
  }
  vtkSpyPlotIStream spis;
  spis.SetStream(&ifs);

  if (!this->ReadHeader(&spis))
  {
    vtkErrorMacro("Invalid Header");
    return 0;
  }

  // Create the array of blocks we are going to use
  this->Blocks = new vtkSpyPlotBlock[this->NumberOfBlocks];

  // Process all the Cell Material  Fields
  if (!this->ReadCellVariableInfo(&spis))
  {
    vtkErrorMacro("Invalid cell variable section");
    return 0;
  }

  // Read all possible material fields
  if (!this->ReadMaterialInfo(&spis))
  {
    vtkErrorMacro("Invalid material section");
    return 0;
  }

  if (!this->ReadGroupHeaderInformation(&spis))
  {
    vtkErrorMacro("Problem reading group header information");
    return 0;
  }

  // now that the group header has been read create the data dumps
  this->DataDumps = new vtkSpyPlotUniReader::DataDump[this->NumberOfDataDumps];

  // clear the memory, instead of using constructors
  const std::size_t length =
    sizeof(vtkSpyPlotUniReader::DataDump) * static_cast<std::size_t>(this->NumberOfDataDumps);
  memset(this->DataDumps, 0, length);

  // Setup time information
  this->TimeStepRange[1] = this->NumberOfDataDumps - 1;
  this->TimeRange[0] = this->DumpTime[0];
  this->TimeRange[1] = this->DumpTime[this->NumberOfDataDumps - 1];

  if (!this->ReadDataDumps(&spis))
  {
    vtkErrorMacro("Problem reading time information");
    return 0;
  }

  this->NumberOfCellFields = this->CellArraySelection->GetNumberOfArrays();
  this->CurrentTime = this->TimeRange[0];
  this->HaveInformation = 1;

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::ReadCellVariableInfo(vtkSpyPlotIStream* spis)
{
  // printf("Before cell fields: %ld\n", ifs.tellg());
  // Read all possible cell fields
  if (!spis->ReadInt32s(&(this->NumberOfPossibleCellFields), 1))
  {
    vtkErrorMacro("Cannot read number of material fields");
    return 0;
  }
  this->CellFields = new vtkSpyPlotUniReader::CellMaterialField[this->NumberOfPossibleCellFields];
  int fieldCnt;
  for (fieldCnt = 0; fieldCnt < this->NumberOfPossibleCellFields; ++fieldCnt)
  {
    vtkSpyPlotUniReader::CellMaterialField* field = this->CellFields + fieldCnt;
    field->Index = 0;
    if (!spis->ReadString(field->Id, 30))
    {
      vtkErrorMacro("Cannot read field " << fieldCnt << " id");
      return 0;
    }
    if (!spis->ReadString(field->Comment, 80))
    {
      vtkErrorMacro("Cannot read field " << fieldCnt << " commenet");
      return 0;
    }
    if (this->FileVersion >= 101)
    {
      if (!spis->ReadInt32s(&(field->Index), 1))
      {
        vtkErrorMacro("Cannot read field " << fieldCnt << " int");
        return 0;
      }
    }
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::ReadMaterialInfo(vtkSpyPlotIStream* spis)
{
  // printf("Before material fields: %ld\n", ifs.tellg());
  // Read all possible material fields
  if (!spis->ReadInt32s(&(this->NumberOfPossibleMaterialFields), 1))
  {
    vtkErrorMacro("Cannot read number of possible material fields");
    return 0;
  }

  this->MaterialFields =
    new vtkSpyPlotUniReader::CellMaterialField[this->NumberOfPossibleMaterialFields];

  for (int fieldCnt = 0; fieldCnt < this->NumberOfPossibleMaterialFields; ++fieldCnt)
  {
    vtkSpyPlotUniReader::CellMaterialField* field = this->MaterialFields + fieldCnt;
    field->Index = 0;
    if (!spis->ReadString(field->Id, 30))
    {
      vtkErrorMacro("Cannot read field " << fieldCnt << " id");
      return 0;
    }
    if (!spis->ReadString(field->Comment, 80))
    {
      vtkErrorMacro("Cannot read field " << fieldCnt << " commenet");
      return 0;
    }
    if (this->FileVersion >= 101)
    {
      if (!spis->ReadInt32s(&(field->Index), 1))
      {
        vtkErrorMacro("Cannot read field " << fieldCnt << " int");
        return 0;
      }
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::ReadDataDumps(vtkSpyPlotIStream* spis)
{
  int dump;
  // Read in the time step information
  for (dump = 0; dump < this->NumberOfDataDumps; ++dump)
  {
    vtkTypeInt64 cpos = spis->Tell();
    vtkTypeInt64 offset = this->DumpOffset[dump];
    if (cpos > offset)
    {
      vtkDebugMacro(<< "The offset is back in file: " << cpos << " > " << offset);
    }
    spis->Seek(offset);
    vtkSpyPlotUniReader::DataDump* dh = &this->DataDumps[dump];

    if (!spis->ReadInt32s(&(dh->NumVars), 1))
    {
      vtkErrorMacro("Cannot read number of variables");
      return 0;
    }
    if (dh->NumVars <= 0)
    {
      vtkErrorMacro("Got bad number of variables: " << dh->NumVars);
      return 0;
    }
    dh->SavedVariables = new int[dh->NumVars];
    dh->SavedVariableOffsets = new vtkTypeInt64[dh->NumVars];
    // printf("Reading saved variables: %ld\n", ifs.tellg());
    if (!spis->ReadInt32s(dh->SavedVariables, dh->NumVars))
    {
      vtkErrorMacro("Cannot read the saved variables");
      return 0;
    }
    if (!spis->ReadInt64s(dh->SavedVariableOffsets, dh->NumVars))
    {
      vtkErrorMacro("Cannot read the saved variable offsets");
      return 0;
    }
    dh->Variables = new vtkSpyPlotUniReader::Variable[dh->NumVars];
    for (int fieldCnt = 0; fieldCnt < dh->NumVars; fieldCnt++)
    {
      vtkSpyPlotUniReader::Variable* variable = dh->Variables + fieldCnt;
      variable->Material = -1;
      variable->Index = -1;
      variable->DataBlocks = 0;
      int var = dh->SavedVariables[fieldCnt];
      if (var >= this->NumberOfPossibleCellFields)
      {
        variable->Index = var % 100 - 1;
        var /= 100;
        var *= 100;
      }
      int cfc;
      if (variable->Index >= 0)
      {
        for (cfc = 0; cfc < this->NumberOfPossibleMaterialFields; ++cfc)
        {
          if (this->MaterialFields[cfc].Index == var)
          {
            variable->Material = cfc;
            variable->MaterialField = this->MaterialFields + cfc;
            break;
          }
        }
      }
      else
      {
        for (cfc = 0; cfc < this->NumberOfPossibleCellFields; ++cfc)
        {
          if (this->CellFields[cfc].Index == var)
          {
            variable->Material = cfc;
            variable->MaterialField = this->CellFields + cfc;
            break;
          }
        }
      }
      if (variable->Material < 0)
      {
        vtkErrorMacro("Cannot found variable or material with ID: " << var);
        return 0;
      }
      if (variable->Index >= 0)
      {
        std::ostringstream ostr;
        ostr << this->MaterialFields[variable->Material].Comment << " - " << variable->Index + 1
             << ends;
        variable->Name = new char[ostr.str().size() + 1];
        strcpy(variable->Name, ostr.str().c_str());
      }
      else
      {
        const char* cname = this->CellFields[variable->Material].Comment;
        variable->Name = new char[strlen(cname) + 1];
        strcpy(variable->Name, cname);
      }
      if (!this->CellArraySelection->ArrayExists(variable->Name))
      {
        // vtkDebugMacro( << __LINE__ << " Disable array: " << variable->Name );
        this->CellArraySelection->DisableArray(variable->Name);
      }
    }

    // printf("Before tracers: %ld\n", ifs.tellg());
    if (!spis->ReadInt32s(&dh->NumberOfTracers, 1))
    {
      vtkErrorMacro("Problem reading the num of tracers");
      return 0;
    }
    if (dh->NumberOfTracers > 0)
    {
      std::vector<unsigned char> tracerBuffer;
      int tracer;
      vtkFloatArray* coords[3];
      for (tracer = 0; tracer < 3; tracer++)
      {
        int numBytes;
        if (!spis->ReadInt32s(&numBytes, 1))
        {
          vtkErrorMacro("Problem reading the num of tracers");
          return 0;
        }
        if (static_cast<int>(tracerBuffer.size()) < numBytes)
        {
          tracerBuffer.resize(numBytes);
        }
        if (!spis->ReadString(&*tracerBuffer.begin(), numBytes))
        {
          vtkErrorMacro("Problem reading the bytes");
          return 0;
        }
        coords[tracer] = vtkFloatArray::New();
        coords[tracer]->SetNumberOfValues(dh->NumberOfTracers);
        float* ptr = coords[tracer]->GetPointer(0);
        if (!this->RunLengthDataDecode(&*tracerBuffer.begin(), numBytes, ptr, dh->NumberOfTracers))
        {
          vtkErrorMacro("Problem RLD decoding float data array");
          return 0;
        }
      }
      dh->TracerCoord = vtkFloatArray::New();
      dh->TracerCoord->SetNumberOfComponents(3);
      dh->TracerCoord->SetNumberOfTuples(dh->NumberOfTracers);
      for (int n = 0; n < dh->NumberOfTracers; n++)
      {
        dh->TracerCoord->SetComponent(n, 0, coords[0]->GetValue(n));
        dh->TracerCoord->SetComponent(n, 1, coords[1]->GetValue(n));
        dh->TracerCoord->SetComponent(n, 2, coords[2]->GetValue(n));
      }
      coords[0]->Delete();
      coords[1]->Delete();
      coords[2]->Delete();

      vtkIntArray* blocks[4];
      for (tracer = 0; tracer < 4; ++tracer) // yes, 7 (3 above + 4) is the magic number
      {
        int numBytes;
        if (!spis->ReadInt32s(&numBytes, 1))
        {
          vtkErrorMacro("Problem reading the num of tracers");
          return 0;
        }
        if (static_cast<int>(tracerBuffer.size()) < numBytes)
        {
          tracerBuffer.resize(numBytes);
        }
        if (!spis->ReadString(&*tracerBuffer.begin(), numBytes))
        {
          vtkErrorMacro("Problem reading the bytes");
          return 0;
        }
        blocks[tracer] = vtkIntArray::New();
        blocks[tracer]->SetNumberOfValues(dh->NumberOfTracers);
        int* ptr = blocks[tracer]->GetPointer(0);
        if (!this->RunLengthDataDecode(&*tracerBuffer.begin(), numBytes, ptr, dh->NumberOfTracers))
        {
          vtkErrorMacro("Problem RLD decoding int data array");
          return 0;
        }
      }
      dh->TracerBlock = vtkIntArray::New();
      dh->TracerBlock->SetNumberOfComponents(4);
      dh->TracerBlock->SetNumberOfTuples(dh->NumberOfTracers);
      for (int n = 0; n < dh->NumberOfTracers; n++)
      {
        dh->TracerBlock->SetComponent(n, 0, blocks[0]->GetValue(n));
        dh->TracerBlock->SetComponent(n, 1, blocks[1]->GetValue(n));
        dh->TracerBlock->SetComponent(n, 2, blocks[2]->GetValue(n));
        dh->TracerBlock->SetComponent(n, 3, blocks[3]->GetValue(n));
      }
      blocks[0]->Delete();
      blocks[1]->Delete();
      blocks[2]->Delete();
      blocks[3]->Delete();
    }

    // Skip Histogram
    int numberOfIndicators;
    if (!spis->ReadInt32s(&numberOfIndicators, 1))
    {
      vtkErrorMacro("Problem reading the num of tracers");
      return 0;
    }
    if (numberOfIndicators > 0)
    {
      spis->Seek(sizeof(int), true);
      int ind;
      for (ind = 0; ind < numberOfIndicators; ++ind)
      {
        spis->Seek(sizeof(int) + sizeof(double) * 6, true);
        int numBins;
        if (!spis->ReadInt32s(&numBins, 1))
        {
          vtkErrorMacro("Problem reading the num of tracers");
          return 0;
        }
        if (numBins > 0)
        {
          int someSize;
          if (!spis->ReadInt32s(&someSize, 1))
          {
            vtkErrorMacro("Problem reading the num of tracers");
            return 0;
          }
          spis->Seek(someSize, true);
        }
      }
    }

    // Now scan the data blocks information
    if (!spis->ReadInt32s(&dh->NumberOfBlocks, 1))
    {
      vtkErrorMacro("Problem reading the num of blocks");
      return 0;
    }
    if (this->NumberOfBlocks != dh->NumberOfBlocks)
    {
      vtkErrorMacro("Different number of blocks...");
    }
    dh->SavedBlockAllocatedStates = new unsigned char[dh->NumberOfBlocks];

    // Record where the state of the block definition is for this
    // time step

    dh->BlocksOffset = spis->Tell();
    //
    // Lets read blocks in 16 at a time.  Performance code.
    //

    int block;
    int totalBlocks = 0;
    for (block = 0; block <= dh->NumberOfBlocks - 16; block += 16)
    {
      int i, j;
      unsigned char allocated_array[16];

      // Skip over the block but remember its allocated state
      if (!vtkSpyPlotBlock::Scan16(spis, allocated_array, this->FileVersion))
      {
        vtkErrorMacro("Problem scanning the block information");
        return 0;
      }

      for (i = block, j = 0; j < 16; i++, j++)
      {
        dh->SavedBlockAllocatedStates[i] = allocated_array[j];
        if (allocated_array[j])
        {
          totalBlocks++;
        }
      }
    }
    //
    // Now pick up any remaining blocks.  Will be 0 to 15.  Performance code.
    //
    for (; block < dh->NumberOfBlocks; ++block)
    {
      // Skip over the block but remember its allocated state
      if (!vtkSpyPlotBlock::Scan(spis, &(dh->SavedBlockAllocatedStates[block]), this->FileVersion))
      {
        vtkErrorMacro("Problem scanning the block information");
        return 0;
      }
      if (dh->SavedBlockAllocatedStates[block])
      {
        totalBlocks++;
      }
    }

    dh->ActualNumberOfBlocks = totalBlocks;
    dh->SavedBlocksGeometryOffset = spis->Tell();

    std::vector<unsigned char> arrayBuffer;
    for (block = 0; block < dh->NumberOfBlocks; ++block)
    {
      if (dh->SavedBlockAllocatedStates[block])
      {
        int numBytes;
        int component;
        // vtkDebugMacro( "Block: " << block );
        for (component = 0; component < 3; ++component)
        {
          if (!spis->ReadInt32s(&numBytes, 1))
          {
            vtkErrorMacro("Problem reading the number of bytes");
            return 0;
          }
          if (static_cast<int>(arrayBuffer.size()) < numBytes)
          {
            arrayBuffer.resize(numBytes);
          }

          if (!spis->ReadString(&*arrayBuffer.begin(), numBytes))
          {
            vtkErrorMacro("Problem reading the bytes");
            return 0;
          }
        }
      }
    }
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotUniReader::ReadMarkerDumps(vtkSpyPlotIStream* spis)
{
  for (int n = 0; n < this->NumberOfMaterials; n++)
  {
    if (this->Markers[n].NumMarks > 0)
    {
      // Reading XLoc array
      int numBytes = 0;
      if (!spis->ReadInt32s(&numBytes, 1))
      {
        vtkErrorMacro("Problem reading the number of bytes");
        return 0;
      }
      std::vector<unsigned char> markerBuffer;
      if (static_cast<int>(markerBuffer.size()) < numBytes)
      {
        markerBuffer.resize(numBytes);
      }
      if (!spis->ReadString(&*markerBuffer.begin(), numBytes))
      {
        vtkErrorMacro("Problem reading the bytes");
        return 0;
      }
      this->MarkersDumps[n].XLoc->SetNumberOfValues(this->Markers[n].NumRealMarks);
      float* ptr = this->MarkersDumps[n].XLoc->GetPointer(0);
      if (!this->RunLengthDataDecode(
            &*markerBuffer.begin(), numBytes, ptr, this->Markers[n].NumRealMarks))
      {
        vtkErrorMacro("Problem RLD decoding float data array 1");
        return 0;
      }
      // Reading ILoc array
      numBytes = 0;
      if (!spis->ReadInt32s(&numBytes, 1))
      {
        vtkErrorMacro("Problem reading the number of bytes");
        return 0;
      }
      if (static_cast<int>(markerBuffer.size()) < numBytes)
      {
        markerBuffer.resize(numBytes);
      }
      if (!spis->ReadString(&*markerBuffer.begin(), numBytes))
      {
        vtkErrorMacro("Problem reading the bytes");
        return 0;
      }
      this->MarkersDumps[n].ILoc->SetNumberOfValues(this->Markers[n].NumRealMarks);
      int* intptr = this->MarkersDumps[n].ILoc->GetPointer(0);
      if (!this->RunLengthDataDecode(
            &*markerBuffer.begin(), numBytes, intptr, this->Markers[n].NumRealMarks))
      {
        vtkErrorMacro("Problem RLD decoding int data array 1");
        return 0;
      }

      if (this->NumberOfDimensions > 1)
      {
        // Reading YLoc array
        numBytes = 0;
        if (!spis->ReadInt32s(&numBytes, 1))
        {
          vtkErrorMacro("Problem reading the number of bytes");
          return 0;
        }
        if (static_cast<int>(markerBuffer.size()) < numBytes)
        {
          markerBuffer.resize(numBytes);
        }
        if (!spis->ReadString(&*markerBuffer.begin(), numBytes))
        {
          vtkErrorMacro("Problem reading the bytes");
          return 0;
        }
        this->MarkersDumps[n].YLoc->SetNumberOfValues(this->Markers[n].NumRealMarks);
        ptr = this->MarkersDumps[n].YLoc->GetPointer(0);
        if (!this->RunLengthDataDecode(
              &*markerBuffer.begin(), numBytes, ptr, this->Markers[n].NumRealMarks))
        {
          vtkErrorMacro("Problem RLD decoding float data array 2");
          return 0;
        }
        // Reading JLoc array
        numBytes = 0;
        if (!spis->ReadInt32s(&numBytes, 1))
        {
          vtkErrorMacro("Problem reading the number of bytes");
          return 0;
        }
        if (static_cast<int>(markerBuffer.size()) < numBytes)
        {
          markerBuffer.resize(numBytes);
        }
        if (!spis->ReadString(&*markerBuffer.begin(), numBytes))
        {
          vtkErrorMacro("Problem reading the bytes");
          return 0;
        }
        this->MarkersDumps[n].JLoc->SetNumberOfValues(this->Markers[n].NumRealMarks);
        intptr = this->MarkersDumps[n].JLoc->GetPointer(0);
        if (!this->RunLengthDataDecode(
              &*markerBuffer.begin(), numBytes, intptr, this->Markers[n].NumRealMarks))
        {
          vtkErrorMacro("Problem RLD decoding int data array 2");
          return 0;
        }
      }
      if (this->NumberOfDimensions > 2)
      {
        // Reading ZLoc array
        if (!spis->ReadInt32s(&numBytes, 1))
        {
          vtkErrorMacro("Problem reading the number of bytes");
          return 0;
        }
        if (static_cast<int>(markerBuffer.size()) < numBytes)
        {
          markerBuffer.resize(numBytes);
        }
        if (!spis->ReadString(&*markerBuffer.begin(), numBytes))
        {
          vtkErrorMacro("Problem reading the bytes");
          return 0;
        }
        this->MarkersDumps[n].ZLoc->SetNumberOfValues(this->Markers[n].NumRealMarks);
        ptr = this->MarkersDumps[n].ZLoc->GetPointer(0);
        if (!this->RunLengthDataDecode(
              &*markerBuffer.begin(), numBytes, ptr, this->Markers[n].NumRealMarks))
        {
          vtkErrorMacro("Problem RLD decoding float data array 3");
          return 0;
        }
        // Reading KLoc array
        if (!spis->ReadInt32s(&numBytes, 1))
        {
          vtkErrorMacro("Problem reading the number of bytes");
          return 0;
        }
        if (static_cast<int>(markerBuffer.size()) < numBytes)
        {
          markerBuffer.resize(numBytes);
        }
        if (!spis->ReadString(&*markerBuffer.begin(), numBytes))
        {
          vtkErrorMacro("Problem reading the bytes");
          return 0;
        }
        this->MarkersDumps[n].KLoc->SetNumberOfValues(this->Markers[n].NumRealMarks);
        intptr = this->MarkersDumps[n].KLoc->GetPointer(0);
        if (!this->RunLengthDataDecode(
              &*markerBuffer.begin(), numBytes, intptr, this->Markers[n].NumRealMarks))
        {
          vtkErrorMacro("Problem RLD decoding int data array 3");
          return 0;
        }
      }
      // Reading Block array
      if (!spis->ReadInt32s(&numBytes, 1))
      {
        vtkErrorMacro("Problem reading the number of bytes");
        return 0;
      }
      if (static_cast<int>(markerBuffer.size()) < numBytes)
      {
        markerBuffer.resize(numBytes);
      }
      if (!spis->ReadString(&*markerBuffer.begin(), numBytes))
      {
        vtkErrorMacro("Problem reading the bytes");
        return 0;
      }
      this->MarkersDumps[n].Block->SetNumberOfValues(this->Markers[n].NumRealMarks);
      intptr = this->MarkersDumps[n].Block->GetPointer(0);
      if (!this->RunLengthDataDecode(
            &*markerBuffer.begin(), numBytes, intptr, this->Markers[n].NumRealMarks))
      {
        vtkErrorMacro("Problem RLD decoding int data array 4");
        return 0;
      }
      for (int v = 0; v < this->Markers[n].NumVars; v++)
      {
        if (!spis->ReadInt32s(&numBytes, 1))
        {
          vtkErrorMacro("Problem reading the number of bytes");
          return 0;
        }
        if (static_cast<int>(markerBuffer.size()) < numBytes)
        {
          markerBuffer.resize(numBytes);
        }
        if (!spis->ReadString(&*markerBuffer.begin(), numBytes))
        {
          vtkErrorMacro("Problem reading the bytes");
          return 0;
        }
        this->MarkersDumps[n].Variables[v]->SetNumberOfValues(this->Markers[n].NumRealMarks);
        ptr = this->MarkersDumps[n].Variables[v]->GetPointer(0);
        if (!this->RunLengthDataDecode(
              &*markerBuffer.begin(), numBytes, ptr, this->Markers[n].NumRealMarks))
        {
          vtkErrorMacro("Problem RLD decoding float data array 5");
          return 0;
        }
      }
    }
  }
  return 1;
}
