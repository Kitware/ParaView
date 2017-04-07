/*=========================================================================

  Program:   ParaView
  Module:    vtkCGNSFileSeriesReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCGNSFileSeriesReader.h"

#include "vtkCGNSReader.h"
#include "vtkCommand.h"
#include "vtkCommunicator.h"
#include "vtkDataSet.h"
#include "vtkFileSeriesHelper.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtksys/RegularExpression.hxx"
#include "vtksys/SystemTools.hxx"

#include <cassert>
#include <map>
#include <set>
#include <sstream>
#include <string>

namespace
{
template <class T>
class SCOPED_SET
{
  T& Var;
  T Prev;

public:
  SCOPED_SET(T& var, const T& val)
    : Var(var)
    , Prev(var)
  {
    this->Var = val;
  }
  ~SCOPED_SET() { this->Var = this->Prev; }
private:
  SCOPED_SET(const SCOPED_SET&) VTK_DELETE_FUNCTION;
  void operator=(const SCOPED_SET&) VTK_DELETE_FUNCTION;
};
}

vtkStandardNewMacro(vtkCGNSFileSeriesReader);
vtkCxxSetObjectMacro(vtkCGNSFileSeriesReader, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkCGNSFileSeriesReader::vtkCGNSFileSeriesReader()
  : FileSeriesHelper()
  , Reader(NULL)
  , IgnoreReaderTime(false)
  , Controller(NULL)
  , ReaderObserverId(0)
  , InProcessRequest(false)
  , ActiveFiles()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkCGNSFileSeriesReader::~vtkCGNSFileSeriesReader()
{
  this->SetReader(NULL);
  this->SetController(NULL);
}

//----------------------------------------------------------------------------
int vtkCGNSFileSeriesReader::CanReadFile(const char* filename)
{
  return this->Reader ? this->Reader->CanReadFile(filename) : 0;
}

//----------------------------------------------------------------------------
void vtkCGNSFileSeriesReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Reader: " << this->Reader << endl;
  os << indent << "IgnoreReaderTime: " << this->IgnoreReaderTime << endl;
}

//----------------------------------------------------------------------------
void vtkCGNSFileSeriesReader::AddFileName(const char* fname)
{
  this->FileSeriesHelper->AddFileName(fname);
}

//----------------------------------------------------------------------------
void vtkCGNSFileSeriesReader::RemoveAllFileNames()
{
  this->FileSeriesHelper->RemoveAllFileNames();
}

//----------------------------------------------------------------------------
const char* vtkCGNSFileSeriesReader::GetCurrentFileName() const
{
  return this->Reader ? this->Reader->GetFileName() : nullptr;
}

//----------------------------------------------------------------------------
void vtkCGNSFileSeriesReader::SetReader(vtkCGNSReader* reader)
{
  if (this->Reader == reader)
  {
    return;
  }

  if (this->Reader)
  {
    this->RemoveObserver(this->ReaderObserverId);
  }
  vtkSetObjectBodyMacro(Reader, vtkCGNSReader, reader);
  if (this->Reader)
  {
    this->ReaderObserverId = this->Reader->AddObserver(
      vtkCommand::ModifiedEvent, this, &vtkCGNSFileSeriesReader::OnReaderModifiedEvent);
  }
}

//----------------------------------------------------------------------------
void vtkCGNSFileSeriesReader::OnReaderModifiedEvent()
{
  if (this->InProcessRequest == false)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkCGNSFileSeriesReader::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Reader)
  {
    vtkErrorMacro("`Reader` cannot be NULL.");
    return 0;
  }

  int requestFromPort = request->Has(vtkStreamingDemandDrivenPipeline::FROM_OUTPUT_PORT())
    ? request->Get(vtkStreamingDemandDrivenPipeline::FROM_OUTPUT_PORT())
    : 0;
  assert(requestFromPort < this->GetNumberOfOutputPorts());
  vtkInformation* outInfo = outputVector->GetInformationObject(requestFromPort);

  assert(this->InProcessRequest == false);
  SCOPED_SET<bool>(this->InProcessRequest, true);

  // Since we are dealing with potentially temporal or partitioned fileseries, a
  // single rank may have to read more than 1 file. Before processing any
  // pipeline pass, let's make sure we have built up the set of active files.
  if (!this->UpdateActiveFileSet(outInfo))
  {
    return 0;
  }

  // Before we continue processing the request, let's decide what mode should
  // the internal vtkCGNSReader work i.e. should it handle parallel processing
  // by splitting blocks across ranks, or are we letting
  // vtkCGNSReaderFileSeriesReader split files among ranks.
  if (this->FileSeriesHelper->GetPartitionedFiles())
  {
    this->Reader->SetController(NULL);
    this->Reader->SetDistributeBlocks(false);
  }
  else
  {
    this->Reader->SetController(this->Controller);
    this->Reader->SetDistributeBlocks(true);
  }

  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_DATA()))
  {
    // For REQUEST_DATA(), we need to iterate over all files in the active
    // set.
    if (!this->RequestData(request, inputVector, outputVector))
    {
      return 0;
    }
  }
  else
  {
    // For most pipeline passes, it's sufficient to choose the first file in the
    // active set, if any, and then pass the request to the internal reader.
    if (this->ActiveFiles.size() > 0)
    {
      this->ChooseActiveFile(0);
      if (!this->Reader->ProcessRequest(request, inputVector, outputVector))
      {
        return 0;
      }
    }
  }

  // restore time information.
  this->FileSeriesHelper->FillTimeInformation(outInfo);
  return 1;
}

namespace vtkCGNSFileSeriesReaderNS
{
static bool SetFileNameCallback(vtkAlgorithm* reader, const std::string& fname)
{
  if (vtkCGNSReader* cgnsReader = vtkCGNSReader::SafeDownCast(reader))
  {
    cgnsReader->SetFileName(fname.c_str());
    return true;
  }
  return false;
};
}

//----------------------------------------------------------------------------
bool vtkCGNSFileSeriesReader::UpdateActiveFileSet(vtkInformation* outInfo)
{
  // Pass ivars to vtkFileSeriesHelper.
  this->FileSeriesHelper->SetIgnoreReaderTime(this->IgnoreReaderTime);

  // We pass a new instance of the reader to vtkFileSeriesHelper to avoid
  // mucking with this->Reader to just gather the file's time meta-data.
  vtkSmartPointer<vtkCGNSReader> reader =
    vtkSmartPointer<vtkCGNSReader>::Take(this->Reader->NewInstance());
  reader->SetController(nullptr);
  reader->SetDistributeBlocks(false);

  // Update vtkFileSeriesHelper. Make it process all the filenames provided and
  // collect useful metadata from it. This is a no-op if the vtkFileSeriesHelper
  // wasn't modified.
  if (!this->FileSeriesHelper->UpdateInformation(
        reader.Get(), vtkCGNSFileSeriesReaderNS::SetFileNameCallback))
  {
    return false;
  }

  // For current time/local partition, we need to determine which files to
  // read. Let's determine that.
  this->ActiveFiles = this->FileSeriesHelper->GetActiveFiles(outInfo);

  // For temporal file series, the active set should only have 1 file. If more than 1
  // file matches the timestep, it means that we may have invalid time information
  // in the file series. Warn about it.
  if (!this->FileSeriesHelper->GetPartitionedFiles() && this->ActiveFiles.size() > 1)
  {
    vtkWarningMacro(
      "The CGNS file series may have incorrect (or duplicate) "
      "time values for a temporal file series. You may want to turn on 'IgnoreReaderTime'.");
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkCGNSFileSeriesReader::ChooseActiveFile(int index)
{
  std::string fname =
    (index < static_cast<int>(this->ActiveFiles.size())) ? this->ActiveFiles[index] : std::string();
  if (this->Reader->GetFileName() == NULL || this->Reader->GetFileName() != fname)
  {
    this->Reader->SetFileName(fname.c_str());
    this->Reader->UpdateInformation();
  }
}

//----------------------------------------------------------------------------
#if defined(_MSC_VER)
// VS generates `warning C4503 : decorated name length exceeded, name was truncated`
// warning for the `BasesMap` data structure defined below. Since it's a private
// data structure, we ignore this warning.
#pragma warning(disable : 4503)
#endif

namespace
{
/**
 * This helps me sync up the multiblock structure across ranks.
 * This is a little hard-coded to the output of CGNS reader. It may be worthwhile to
 * generalize this to a filter and then simply use that.
 */
struct vtkCGNSData
{
  typedef std::vector<vtkSmartPointer<vtkDataSet> > DatasetsVector;
  typedef std::map<std::string, DatasetsVector> ZonesMap;
  typedef std::map<std::string, ZonesMap> BasesMap;

  BasesMap Bases;

  bool Add(vtkMultiBlockDataSet* mb)
  {
    vtksys::RegularExpression zoneNameRe("^(.*)_proc-([0-9]+)$");
    for (unsigned int base = 0, maxbase = mb->GetNumberOfBlocks(); base < maxbase; ++base)
    {
      vtkMultiBlockDataSet* baseMB = vtkMultiBlockDataSet::SafeDownCast(mb->GetBlock(base));
      const char* baseName = mb->GetMetaData(base)->Get(vtkCompositeDataSet::NAME());
      ZonesMap& zones = Bases[baseName];

      for (unsigned int zone = 0, maxzone = baseMB->GetNumberOfBlocks(); zone < maxzone; ++zone)
      {
        vtkDataSet* zoneDS = vtkDataSet::SafeDownCast(baseMB->GetBlock(zone));
        if (!zoneDS)
        {
          continue;
        } // raise error.

        std::string zoneName = baseMB->GetMetaData(zone)->Get(vtkCompositeDataSet::NAME());
        int piece = -1;
        if (zoneNameRe.find(zoneName))
        {
          piece = atoi(zoneNameRe.match(2).c_str());
          zoneName = zoneNameRe.match(1);
        }

        DatasetsVector& datasets = zones[zoneName];
        if (piece >= 0 && static_cast<int>(datasets.size()) <= piece)
        {
          datasets.resize(piece + 1);
        }
        if (piece >= 0)
        {
          if (datasets[piece] != NULL)
          {
            vtkGenericWarningMacro(
              "There may be a mismatch in the block names in partitioned CGNS file. "
              "Blocks may get overwritten.");
          }
          datasets[piece] = zoneDS;
        }
        else
        {
          datasets.push_back(zoneDS);
        }
      }
    }
    return true;
  }

  bool Get(vtkMultiBlockDataSet* mb)
  {
    unsigned int baseBlockId = 0;
    for (auto baseIter = this->Bases.begin(); baseIter != this->Bases.end();
         ++baseIter, ++baseBlockId)
    {
      vtkNew<vtkMultiBlockDataSet> blockMB;
      blockMB->SetNumberOfBlocks(static_cast<unsigned int>(baseIter->second.size()));

      mb->SetBlock(baseBlockId, blockMB.Get());
      mb->GetMetaData(baseBlockId)->Set(vtkCompositeDataSet::NAME(), baseIter->first.c_str());

      unsigned int zoneBlockId = 0;
      for (auto zoneIter = baseIter->second.begin(); zoneIter != baseIter->second.end();
           ++zoneIter, ++zoneBlockId)
      {
        blockMB->GetMetaData(zoneBlockId)
          ->Set(vtkCompositeDataSet::NAME(), zoneIter->first.c_str());
        if (zoneIter->second.size() > 1)
        {
          vtkNew<vtkMultiPieceDataSet> zoneMB;
          blockMB->SetBlock(zoneBlockId, zoneMB.Get());

          unsigned int dsBlockId = 0;
          for (auto dsIter = zoneIter->second.begin(); dsIter != zoneIter->second.end();
               ++dsIter, ++dsBlockId)
          {
            zoneMB->SetPiece(dsBlockId, dsIter->GetPointer());
          }
        }
        else if (zoneIter->second.size() == 1)
        {
          blockMB->SetBlock(zoneBlockId, zoneIter->second[0].GetPointer());
        }
      }
    }
    return true;
  }

  bool SyncMetadata(vtkMultiProcessController* controller)
  {
    // sync up base names among all ranks.
    std::set<std::string> bnames;
    for (auto biter = this->Bases.begin(); biter != this->Bases.end(); ++biter)
    {
      bnames.insert(biter->first);
    }
    this->AllReduce(bnames, controller);

    // ensure this->Bases has base for all known base names.
    for (auto bniter = bnames.begin(); bniter != bnames.end(); ++bniter)
    {
      this->Bases[*bniter];
    }

    // Now do the same for zones under each base.
    for (auto biter = this->Bases.begin(); biter != this->Bases.end(); ++biter)
    {
      std::set<std::string> znames;
      for (auto ziter = biter->second.begin(); ziter != biter->second.end(); ++ziter)
      {
        znames.insert(ziter->first);
      }
      this->AllReduce(znames, controller);
      for (auto zniter = znames.begin(); zniter != znames.end(); ++zniter)
      {
        biter->second[*zniter];
      }
    }

    // Now sync number of datasets for each zone.
    std::vector<int> dscounts;
    for (auto biter = this->Bases.begin(); biter != this->Bases.end(); ++biter)
    {
      for (auto ziter = biter->second.begin(); ziter != biter->second.end(); ++ziter)
      {
        dscounts.push_back(static_cast<int>(ziter->second.size()));
      }
    }
    std::vector<int> maxcounts(dscounts.size());
    controller->AllReduce(&dscounts[0], &maxcounts[0], static_cast<vtkIdType>(dscounts.size()),
      vtkCommunicator::MAX_OP);

    auto countsiter = maxcounts.begin();
    for (auto biter = this->Bases.begin(); biter != this->Bases.end(); ++biter)
    {
      for (auto ziter = biter->second.begin(); ziter != biter->second.end(); ++ziter, ++countsiter)
      {
        ziter->second.resize(*countsiter);
      }
    }
    return true;
  }

  void AllReduce(std::set<std::string>& names, vtkMultiProcessController* controller)
  {
    std::ostringstream str;
    for (auto iter = names.begin(); iter != names.end(); ++iter)
    {
      str << (*iter) << '\n';
    }

    int slen = static_cast<int>(str.str().size()) + 1; // includes room for null terminator.
    int maxslen = 0;
    controller->AllReduce(&slen, &maxslen, 1, vtkCommunicator::MAX_OP);

    int numRanks = controller->GetNumberOfProcesses();
    std::vector<char> buffer(numRanks * maxslen, '\0');
    std::vector<char> sbuffer(maxslen, '\0');
    strcpy(&sbuffer[0], str.str().c_str());

    controller->AllGather(&sbuffer[0], &buffer[0], maxslen);
    names.clear();
    for (int cc = 0; cc < numRanks; ++cc)
    {
      std::vector<std::string> rnames;
      vtksys::SystemTools::Split(std::string(&buffer[cc * maxslen]), rnames, '\n');
      names.insert(rnames.begin(), rnames.end());
    }
  }
};
}

//----------------------------------------------------------------------------
int vtkCGNSFileSeriesReader::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->FileSeriesHelper->GetPartitionedFiles())
  {
    // for non-partitioned files, we're letting vtkCGNSReader do the reading as appropriate.
    this->ChooseActiveFile(0);
    return this->Reader->ProcessRequest(request, inputVector, outputVector);
  }

  // iterate over all files in the active set and collect the data.
  vtkCGNSData cgnsdata;

  int success = 1;
  for (size_t cc = 0, max = this->ActiveFiles.size(); cc < max; ++cc)
  {
    this->ChooseActiveFile(static_cast<int>(cc));
    if (this->Reader->ProcessRequest(request, inputVector, outputVector))
    {
      vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outputVector, 0);
      assert(output);
      cgnsdata.Add(output);
      output->Initialize();
    }
    else
    {
      vtkErrorMacro("Failed to read '" << this->GetCurrentFileName() << "'");
      success = 0;
      break;
    }
  }

  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    int allSuccess = 0;
    this->Controller->AllReduce(&success, &allSuccess, 1, vtkCommunicator::MIN_OP);
    if (allSuccess == 0)
    {
      return 0;
    }
  }

  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    // Ensure all ranks have same meta-data about the number of bases and zones.
    cgnsdata.SyncMetadata(this->Controller);
  }

  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outputVector, 0);
  output->Initialize();
  return cgnsdata.Get(output) ? 1 : 0;
}
