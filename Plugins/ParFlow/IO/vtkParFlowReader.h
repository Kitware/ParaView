// See license.md for copyright information.
#ifndef vtkParFlowReader_h
#define vtkParFlowReader_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkParFlowIOModule.h"

#include "vtkVector.h"

#include <fstream>
#include <vector>

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
  /// The default is false.
  /// CLM files have multiple values per cell while PFB files have only one.
  vtkGetMacro(IsCLMFile, int);
  vtkSetMacro(IsCLMFile, int);
  vtkBooleanMacro(IsCLMFile, int);

  /// Set/get the CLM irradiance type.
  ///
  /// The CLM irradiance types are enumerated by ParFlow.
  /// At least 1 and 3 are valid values.
  /// The default is 1.
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

  /// The filename, which must be a valid path before RequestData is called.
  char* FileName;
  int IsCLMFile;
  int CLMIrrType;
  /// IJKDiv only valid inside RequestData; used to compute subgrid offsets.
  std::vector<int> IJKDivs[3];
};

#endif // vtkParflowReader_h
