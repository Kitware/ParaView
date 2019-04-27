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

#include <sstream>

static constexpr std::streamoff headerSize = 6 * sizeof(double) + 4 * sizeof(int);
static constexpr std::streamoff subgridHeaderSize = 9 * sizeof(int);
static constexpr std::streamoff pfbEntrySize = sizeof(double);
static constexpr std::streamoff clmEntrySize =
  11 * sizeof(double); // TODO: This is just a starting point.

vtkStandardNewMacro(vtkParFlowReader);

vtkParFlowReader::vtkParFlowReader()
  : FileName(nullptr)
  , IsCLMFile(0)
  , CLMIrrType(1)
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
  os << indent << "IsCLMFile: " << (this->IsCLMFile ? "true" : "false") << "\n";
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
  if (!fnparts.empty())
  {
    fnparts.pop_back(); // Drop the ".pfb".
    if (!fnparts.empty())
    {
      if (is_number(fnparts.back()))
      {
        fnparts.pop_back(); // Drop the timestep.
      }
      if (!fnparts.empty())
      {
        arrayName = fnparts.back();
      }
    }
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
  std::streamoff entrySize = this->IsCLMFile ? clmEntrySize : pfbEntrySize;
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
  std::streamoff entrySize = this->IsCLMFile ? clmEntrySize : pfbEntrySize;
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
    vtkNew<vtkDoubleArray> field;
    image->SetOrigin(origin.GetData());
    image->SetSpacing(spacing.GetData());
    image->SetExtent(si[0], si[0] + sn[0], si[1], si[1] + sn[1], si[2], si[2] + sn[2]);
    field->SetNumberOfTuples(image->GetNumberOfCells());
    field->SetName(arrayName.c_str());
    image->GetCellData()->SetScalars(field);

    vtkIdType numValues = sn[0] * sn[1] * sn[2];
    pfb.read(reinterpret_cast<char*>(field->GetVoidPointer(0)), sizeof(double) * numValues);
    vtkByteSwap::SwapBERange(reinterpret_cast<double*>(field->GetVoidPointer(0)), numValues);

    output->SetBlock(blockId, image);
  }
}
