// See license.md for copyright information.
#ifndef vtkParFlowReader_h
#define vtkParFlowReader_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkParFlowIOModule.h"

#include "vtkVector.h"

#include <fstream>
#include <vector>

class vtkDoubleArray;
class vtkImageData;
class vtkMultiBlockDataSet;

/**\brief Read ParFlow simulation output.
  *
  * Data is output as a multiblock of image data.
  *
  * This reader will work in parallel settings by
  * splitting existing blocks among ranks.
  * If there are fewer blocks than ranks, some processes will do no work.
  * You may use "pftools dist" to repartition the data into a different
  * number of blocks (known in ParFlow as subgrids).
  *
  * You may indicate files contain CLM data or a single PFB state variable.
  * CLM files have multiple values per cell while PFB files hold only one.
  * The per-cell values are stored as i-j image stacks (i.e., each subgrid
  * stores a sequence of 2-d images, one per CLM state variable); the k-index
  * extent of the PFB file corresponds to the number of CLM state variables
  * per cell.
  */
class VTKPARFLOWIO_EXPORT vtkParFlowReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkParFlowReader, vtkMultiBlockDataSetAlgorithm);
  static vtkParFlowReader* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Set/get the name of the ".pfb" file to read.
  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileName);

  /// Set/get whether the file is a CLM output (i.e., ".c.pfb").
  ///
  /// This variable accepts 3 values:
  /// + 0 indicates the file is not a CLM file
  /// + 1 indicates the file is a CLM file
  /// + -1 indicates the filetype should be inferred from the filename.
  ///
  /// The default is -1.
  vtkGetMacro(IsCLMFile, int);
  vtkSetMacro(IsCLMFile, int);
  vtkBooleanMacro(IsCLMFile, int);

  /// Set/get the CLM irrigation type.
  ///
  /// The CLM irrigation types are enumerated by ParFlow:
  /// 0=None, 1=Spray, 2=Drip, 3=Instant.
  /// The default is 0.
  vtkGetMacro(CLMIrrType, int);
  vtkSetMacro(CLMIrrType, int);

protected:
  vtkParFlowReader();
  virtual ~vtkParFlowReader();

  /// Update the reader's output.
  int RequestData(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

  /// Get grid topology on rank 0.
  ///
  /// This sets IJKDivs on rank 0.
  void ScanBlocks(std::ifstream& file, int nblocks);

  /// Broadcast grid topology from rank 0.
  ///
  /// This sets IJKDivs on all ranks.
  void BroadcastBlocks();

  /// Use grid topology to compute a block (subgrid) offset.
  ///
  /// Only call this after IJKDivs has been set.
  std::streamoff GetBlockOffset(int blockId) const;
  std::streamoff GetBlockOffset(const vtkVector3i& blockIJK) const;

  /// Use current grid topology to compute a just-past-the-end subgrid offset.
  ///
  /// This method is used inside ScanBlocks to quickly advance
  /// to the next subgrid header that will provide a new value
  /// for IJKDivs.
  std::streamoff GetEndOffset() const;

  static bool ReadSubgridHeader(ifstream& pfb, vtkVector3i& si, vtkVector3i& sn, vtkVector3i& sr);

  /// Read a single block from the file
  void ReadBlock(std::ifstream& file, vtkMultiBlockDataSet* output, vtkVector3d& origin,
    vtkVector3d& spacing, const std::string& arrayName, int block);

  /// Given the size of the whole grid, the number of subgrids on each axis, and a block IJK
  /// return the min and max node coordinates for that block.
  static void GetBlockExtent(const vtkVector3i& wholeExtentIn, const vtkVector3i& numberOfBlocksIn,
    const vtkVector3i& blockIJKIn, vtkVector3i& blockExtentMinOut, vtkVector3i& blockExtentMaxOut);

  static void ReadBlockIntoArray(std::ifstream& file, vtkImageData* img, vtkDoubleArray* arr);

  /// The filename, which must be a valid path before RequestData is called.
  char* FileName;
  int IsCLMFile;
  int CLMIrrType;
  /// IJKDivs, NZ, and InferredAsCLM are only valid inside RequestData; used to compute subgrid
  /// offsets.
  std::vector<int> IJKDivs[3];
  int NZ;
  int InferredAsCLM;
};

#endif // vtkParflowReader_h
