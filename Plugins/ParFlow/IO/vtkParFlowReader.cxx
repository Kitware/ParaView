// See license.md for copyright information.
#include "vtkParFlowReader.h"

#include "vtkByteSwap.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <cctype>
#include <sstream>

static constexpr std::streamoff headerSize = 6 * sizeof(double) + 4 * sizeof(int);
static constexpr std::streamoff subgridHeaderSize = 9 * sizeof(int);
static constexpr std::streamoff pfbEntrySize = sizeof(double);
static const char* clmBaseComponentNames[] = { "eflx_lh_tot", "eflx_lwrad_out", "eflx_sh_tot",
  "eflx_soil_grnd", "qflx_evap_tot", "qflx_evap_grnd", "qflx_evap_soi", "qflx_evap_veg",
  "qflx_tran_veg", "qflx_infl", "swe_out", "t_grnd" };
static constexpr int clmBaseComponents =
  static_cast<int>(sizeof(clmBaseComponentNames) / sizeof(clmBaseComponentNames[0]));

static std::streamoff computeEntrySize(bool isCLM, int nz)
{
  std::streamoff sz;
  if (!isCLM)
  {
    sz = pfbEntrySize;
  }
  else
  {
    if (nz < clmBaseComponents)
    {
      std::cerr << "Invalid CLM file: k axis must have at least " << clmBaseComponents
                << " entries but has " << nz << "\n";
    }
    sz = nz * sizeof(double);
  }
  return sz;
}

vtkStandardNewMacro(vtkParFlowReader);

vtkParFlowReader::vtkParFlowReader()
  : FileName(nullptr)
  , IsCLMFile(-1)
  , CLMIrrType(0)
  , NZ(0)
  , InferredAsCLM(-1)
{
  this->SetNumberOfInputPorts(0);
}

vtkParFlowReader::~vtkParFlowReader()
{
  this->SetFileName(nullptr);
}

void vtkParFlowReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(null)") << "\n";
  os << indent
     << "IsCLMFile: " << (this->IsCLMFile > 0 ? "true" : this->IsCLMFile < 0 ? "infer" : "false")
     << "\n";
  os << indent << "CLMIrrType: " << this->CLMIrrType << "\n";
  os << indent << "IJKDivs:\n";
  vtkIndent i2 = indent.GetNextIndent();
  for (int ijk = 0; ijk < 3; ++ijk)
  {
    os << i2 << "axis " << ijk << ":";
    for (auto div : this->IJKDivs[ijk])
    {
      os << " " << div;
    }
    os << "\n";
  }
}

static bool is_number(const std::string& s)
{
  return !s.empty() &&
    std::find_if(s.begin(), s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

int vtkParFlowReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inInfo), vtkInformationVector* outInfo)
{
  if (!this->FileName || !this->FileName[0])
  {
    vtkErrorMacro("No filename provided.");
    return 0;
  }

  std::ifstream pfb(this->FileName, std::ios::binary);
  if (!pfb.good())
  {
    vtkErrorMacro("Unable to open file.");
    return 0;
  }

  auto output = vtkMultiBlockDataSet::GetData(outInfo, 0);
  if (!output)
  {
    vtkErrorMacro("No output data object created.");
    return 0;
  }

  // Try to tease out the array name from the filename.
  std::string filename(this->FileName);
  std::vector<std::string> fnparts;
  std::istringstream tokenizer(filename);
  std::string token;
  while (std::getline(tokenizer, token, '.'))
  {
    fnparts.push_back(token);
  }
  std::string arrayName("data");
  this->InferredAsCLM = 0;
  if (!fnparts.empty())
  {
    fnparts.pop_back(); // Drop the ".pfb".
    if (!fnparts.empty())
    {
      if (fnparts.back() == "C" || fnparts.back() == "c")
      {
        this->InferredAsCLM = 1;
      }
      if (is_number(fnparts.back().substr(0, 1)))
      {
        fnparts.pop_back(); // Drop the timestep.
      }
      if (!fnparts.empty())
      {
        arrayName = fnparts.back();
      }
    }
  }
  if (this->IsCLMFile >= 0)
  {
    this->InferredAsCLM = this->IsCLMFile;
  }

  // When run in parallel, we choose a range of blocks
  // to load from those available.
  int rank = 1;
  int jbsz = 1;
  auto mpc = vtkMultiProcessController::GetGlobalController();
  if (mpc)
  {
    rank = mpc->GetLocalProcessId();
    jbsz = mpc->GetNumberOfProcesses();
  }

  vtkVector3d xx;
  vtkVector3i nn;
  vtkVector3d dx;
  int numSubGrids;

  pfb.seekg(0);
  pfb.read(reinterpret_cast<char*>(xx.GetData()), 3 * sizeof(double));
  pfb.read(reinterpret_cast<char*>(nn.GetData()), 3 * sizeof(int));
  pfb.read(reinterpret_cast<char*>(dx.GetData()), 3 * sizeof(double));
  pfb.read(reinterpret_cast<char*>(&numSubGrids), sizeof(int));

  // Swap bytes as required knowing that we started with big-endian data:
  vtkByteSwap::SwapBERange(xx.GetData(), 3);
  vtkByteSwap::SwapBERange(nn.GetData(), 3);
  vtkByteSwap::SwapBERange(dx.GetData(), 3);
  vtkByteSwap::SwapBE(&numSubGrids);

  this->NZ = nn[2]; // For computing size of CLM state.

  std::streamoff measuredHeaderSize = pfb.tellg();
  if (measuredHeaderSize != headerSize)
  {
    vtkErrorMacro("Header size mismatch");
    return 0;
  }

#if 0
  std::cout
    << "File summary:\n"
    << "  origin     " << xx << "\n"
    << "  resolution " << nn << "\n"
    << "  spacing    " << dx << "\n"
    << "  subgrids   " << numSubGrids << "\n";
#endif

  // Update {I,J,K}Divs on rank 0 by reading file:
  this->ScanBlocks(pfb, numSubGrids);
  // Update {I,J,K}Divs on ranks > 0 via network:
  this->BroadcastBlocks();

  int gridLo = (rank * numSubGrids) / jbsz;
  int gridHi = ((rank + 1) * numSubGrids) / jbsz;
  // std::cout << "Rank " << rank << " owns subgrids " << gridLo << " -- " << gridHi << "\n";

  output->SetNumberOfBlocks(numSubGrids);
  for (int ni = gridLo; ni < gridHi; ++ni)
  {
    this->ReadBlock(pfb, output, xx, dx, arrayName, ni);
  }

  // Prevent accidents; don't preserve across calls to RequestData:
  this->NZ = 0;
  this->InferredAsCLM = -1;

  return 1;
}

bool vtkParFlowReader::ReadSubgridHeader(
  ifstream& pfb, vtkVector3i& si, vtkVector3i& sn, vtkVector3i& sr)
{
  pfb.read(reinterpret_cast<char*>(si.GetData()), 3 * sizeof(int));
  pfb.read(reinterpret_cast<char*>(sn.GetData()), 3 * sizeof(int));
  pfb.read(reinterpret_cast<char*>(sr.GetData()), 3 * sizeof(int));

  // Swap bytes as required knowing that we started with big-endian data:
  vtkByteSwap::SwapBERange(si.GetData(), 3);
  vtkByteSwap::SwapBERange(sn.GetData(), 3);
  vtkByteSwap::SwapBERange(sr.GetData(), 3);

  return pfb.good() && !pfb.eof();
}

void vtkParFlowReader::ScanBlocks(std::ifstream& pfb, int vtkNotUsed(numSubGrids))
{
  auto mpc = vtkMultiProcessController::GetGlobalController();
  if (mpc && mpc->GetLocalProcessId() > 0)
  {
    return; // only rank 0 should read subgrid info
  }

  for (int ii = 0; ii < 3; ++ii)
  {
    this->IJKDivs[ii].clear();
    this->IJKDivs[ii].push_back(0);
  }

  // Find all the axis block-dividers by repeatedly
  // reading the block just past the end of known space
  // and inspecting its coordinates.
  vtkVector3i si;
  vtkVector3i sn;
  vtkVector3i sr;
  while (pfb.good() && !pfb.eof())
  {
    auto offset = this->GetEndOffset();
    pfb.seekg(offset, std::ios::beg);
    if (this->ReadSubgridHeader(pfb, si, sn, sr))
    {
      // If this extends a division, record it:
      for (int ijk = 0; ijk < 3; ++ijk)
      {
        int div = si[ijk] + sn[ijk];
        if (div > this->IJKDivs[ijk].back())
        {
          // std::cout << offset << ": axis " << ijk << " extend " << div << "\n";
          this->IJKDivs[ijk].push_back(div);
        }
      }
    }
  }

  // Skip back to the start of the first block.
  pfb.clear();
  pfb.seekg(headerSize, std::ios::beg);
}

void vtkParFlowReader::BroadcastBlocks()
{
  auto mpc = vtkMultiProcessController::GetGlobalController();
  if (!mpc)
  {
    return;
  }
  std::size_t len[3] = { this->IJKDivs[0].size(), this->IJKDivs[1].size(),
    this->IJKDivs[2].size() };
  mpc->Broadcast(len, 3, 0);
  if (mpc->GetLocalProcessId() > 0)
  {
    for (int ijk = 0; ijk < 3; ++ijk)
    {
      this->IJKDivs[ijk].resize(len[ijk]);
    }
  }
  mpc->Broadcast(&this->IJKDivs[0][0], len[0], 0);
  mpc->Broadcast(&this->IJKDivs[1][0], len[1], 0);
  mpc->Broadcast(&this->IJKDivs[2][0], len[2], 0);
}

std::streamoff vtkParFlowReader::GetBlockOffset(int blockId) const
{
  int gridTopo[3] = { static_cast<int>(this->IJKDivs[0].size() - 1),
    static_cast<int>(this->IJKDivs[1].size() - 1), static_cast<int>(this->IJKDivs[2].size() - 1) };
  vtkVector3i blockIJK(blockId % gridTopo[0], (blockId / gridTopo[0]) % gridTopo[1],
    blockId / gridTopo[0] / gridTopo[1]);

  return this->GetBlockOffset(blockIJK);
}

std::streamoff vtkParFlowReader::GetBlockOffset(const vtkVector3i& blockIJK) const
{
  std::streamoff offset;
  std::streamoff entrySize = computeEntrySize(!!this->InferredAsCLM, this->NZ);
  int ni = static_cast<int>(this->IJKDivs[0].size()) - 1;
  int nj = static_cast<int>(this->IJKDivs[1].size()) - 1;
  int blockId = blockIJK[0] + ni * (blockIJK[1] + nj * blockIJK[2]);

  // Account for file and subgrid headers:
  offset = headerSize + subgridHeaderSize * blockId;

  // Account for points in all whole layers of subgrids "beneath" this location:
  offset +=
    entrySize * this->IJKDivs[2][blockIJK[2]] * this->IJKDivs[1].back() * this->IJKDivs[0].back();

  // Account for points in all whole i-rows of subgrids "beneath" this location:
  int dk = this->IJKDivs[2][blockIJK[2] + 1] - this->IJKDivs[2][blockIJK[2]];
  offset += entrySize * dk * this->IJKDivs[1][blockIJK[1]] * this->IJKDivs[0].back();

  // Account for points in the current i-row of subgrids "beneath" this location:
  int dj = this->IJKDivs[1][blockIJK[1] + 1] - this->IJKDivs[1][blockIJK[1]];
  offset += entrySize * dk * dj * this->IJKDivs[0][blockIJK[0]];

  return offset;
}

std::streamoff vtkParFlowReader::GetEndOffset() const
{
  std::streamoff offset;
  std::streamoff entrySize = computeEntrySize(!!this->InferredAsCLM, this->NZ);
  int ni = static_cast<int>(this->IJKDivs[0].size()) - 1;
  int nj = static_cast<int>(this->IJKDivs[1].size()) - 1;
  int nk = static_cast<int>(this->IJKDivs[2].size()) - 1;
  int numBlocks = ni * nj * nk;

  // Account for file and subgrid headers:
  offset = headerSize + subgridHeaderSize * numBlocks;

  // Account for all entries in all blocks
  offset += entrySize * this->IJKDivs[0].back() * this->IJKDivs[1].back() * this->IJKDivs[2].back();

  return offset;
}

void vtkParFlowReader::ReadBlock(std::ifstream& pfb, vtkMultiBlockDataSet* output,
  vtkVector3d& origin, vtkVector3d& spacing, const std::string& arrayName, int blockId)
{
  vtkVector3i si;
  vtkVector3i sn;
  vtkVector3i sr;

  // Move file cursor to start of block.
  std::streamoff blockOffset = this->GetBlockOffset(blockId);
  pfb.seekg(blockOffset);

  if (this->ReadSubgridHeader(pfb, si, sn, sr))
  {
    vtkNew<vtkImageData> image;
    image->SetOrigin(origin.GetData());
    image->SetSpacing(spacing.GetData());

    if (this->InferredAsCLM)
    {
      // The CLM files have the full simulation extent listed but only
      // provide data on the top 2-d surface:
      image->SetExtent(si[0], si[0] + sn[0], si[1], si[1] + sn[1], si[2], si[2]);

      const int numCLMVars = sn[2] - si[2];
      int numComponents = 0;
      for (int cc = 0; cc < clmBaseComponents && cc < numCLMVars; ++cc, ++numComponents)
      {
        vtkNew<vtkDoubleArray> field;
        field->SetName(clmBaseComponentNames[cc]);
        this->ReadBlockIntoArray(pfb, image, field);
      }
      switch (this->CLMIrrType)
      {
        case 1:
        {
          vtkNew<vtkDoubleArray> field;
          field->SetName("qflx_qirr");
          this->ReadBlockIntoArray(pfb, image, field);
          ++numComponents;
        }
        break;
        case 3:
        {
          vtkNew<vtkDoubleArray> field;
          field->SetName("qflx_qirr_inst");
          this->ReadBlockIntoArray(pfb, image, field);
          ++numComponents;
        }
        break;
        default:
          break;
      }
      for (int cz = 0; numComponents < numCLMVars; ++cz, ++numComponents)
      {
        vtkNew<vtkDoubleArray> field;
        std::ostringstream name;
        name << "tsoil_" << cz;
        field->SetName(name.str().c_str());
        this->ReadBlockIntoArray(pfb, image, field);
      }
    }
    else
    {
      // Read a single PFB state variable:
      vtkNew<vtkDoubleArray> field;
      image->SetExtent(si[0], si[0] + sn[0], si[1], si[1] + sn[1], si[2], si[2] + sn[2]);
      field->SetName(arrayName.c_str());
      this->ReadBlockIntoArray(pfb, image, field);
    }

    output->SetBlock(blockId, image);
  }
}

void vtkParFlowReader::ReadBlockIntoArray(
  std::ifstream& file, vtkImageData* img, vtkDoubleArray* arr)
{
  arr->SetNumberOfTuples(img->GetNumberOfCells());
  auto cellData = img->GetCellData();
  // Calling cellData->SetScalars(arr) multiple times removes
  // previous arrays set as scalars, so be careful not to:
  cellData->AddArray(arr);
  if (!cellData->GetScalars())
  {
    cellData->SetScalars(arr);
  }

  vtkIdType numValues = arr->GetNumberOfTuples() * arr->GetNumberOfComponents();
  file.read(reinterpret_cast<char*>(arr->GetVoidPointer(0)), sizeof(double) * numValues);
  vtkByteSwap::SwapBERange(reinterpret_cast<double*>(arr->GetVoidPointer(0)), numValues);
  // std::cout << arr->GetNumberOfComponents() << " " << numValues << "Read to byte " << pfb.tellg()
  // << "\n";
}
