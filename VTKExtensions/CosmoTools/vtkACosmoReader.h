// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2007, 2009 Los Alamos National Security, LLC
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-LANL-USGov
/**
 * @class   vtkACosmoReader
 * @brief   Adaptively read a binary cosmology data file
 *
 */

#ifndef vtkACosmoReader_h
#define vtkACosmoReader_h

#include "vtkMultiBlockDataSetAlgorithm.h" // Base class
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkPVVTKExtensionsCosmoToolsModule.h" // For export macro

// C/C++ includes
#include <set>    // For STL set
#include <string> // For C++ string
#include <vector> // For STL vector

// Forward declarations

class vtkInformation;
class vtkInformationVector;
class vtkMultiBlockDataSet;
class vtkUnstructuredGrid;

class VTKPVVTKEXTENSIONSCOSMOTOOLS_EXPORT vtkACosmoReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkACosmoReader* New();
  vtkTypeMacro(vtkACosmoReader, vtkMultiBlockDataSetAlgorithm);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  ///@{
  /**
   * Add/Remove files to read. These files are treated as set in the adaptive
   * cosmo files.
   */
  void AddFileName(const char*);
  void RemoveAllFileNames();
  ///@}

  // Set/Get the box size for the simulation (range along x,y,z)
  // Negative x,y,z values are subtracted from this for wraparound
  vtkSetMacro(BoxSize, double);
  vtkGetMacro(BoxSize, double);

  ///@{
  /**
   * Set/Get the endian-ness of the binary file
   */
  vtkSetMacro(ByteSwap, int);
  vtkGetMacro(ByteSwap, int);
  ///@}

  ///@{
  /**
   * When false (default) 32-bit tags are read from the file.  When
   * on, 64-bit tags are read from the file.
   */
  vtkSetMacro(TagSize, int);
  vtkGetMacro(TagSize, int);
  ///@}

  ///@{
  /**
   * Sets the level of resolution
   */
  vtkSetMacro(Level, int);
  vtkGetMacro(Level, int);
  ///@}

protected:
  vtkACosmoReader();
  ~vtkACosmoReader();

  // Standard pipeline methods
  virtual int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  /**
   * Loads the metadata
   */
  void LoadMetaData();

  /**
   * Processes the user-supplied FileName and extracts the
   * base file name, as well as, the total number of levels.
   */
  void ExtractInfoFromFileNames();

  /**
   * Reads the metadata file with the given filename at the specified level.
   */
  void ReadMetaDataFile(const int levelIdx, std::string file);

  /**
   * Given the level and index of the block within that level, this method
   * returns the block index.
   */
  int GetBlockIndex(const int level, const int idx);

  /**
   * Given an output information object, this method will populate
   * the vector of block Ids to read in.
   */
  void SetupBlockRequest(vtkInformation* outInfo);

  /**
   * Read in the block corresponding to the given index
   */
  void ReadBlock(const int blockIdx, vtkMultiBlockDataSet* mbds);

  /**
   * Given the block level and index within the level, this method returns
   * the block's starting offset within the file.
   */
  int GetBlockStartOffSetInFile(const int level, const int index);

  /**
   * Given the file and start/end offsets of a block, this method reads in
   * the particles for a contiguous block.
   */
  void ReadBlockFromFile(
    std::string file, const int start, const int end, vtkUnstructuredGrid* particles);

  std::string BaseFileName; // Base path and file name
  char* FileName;           // Name of binary particle file
  bool MetadataIsLoaded;    // Indicates if the meta data has been loaded

  double BoxSize;          // Maximum of x,y,z locations from simulation
  int ByteSwap;            // indicates whether to swap data or not
  int TagSize;             // Size of the tag, 0 = 32-bit or 1 = 64-bit
  int Level;               // level of resolution to load (staring from 1)
  int TotalNumberOfLevels; // The total number of levels

  vtkMultiBlockDataSet* MetaData;

  struct block_t; // defined in the implementation

  std::vector<int> NBlocks;            // Number of blocks at level "i"
  std::vector<block_t> ParticleBlocks; // stores block info for each block
  std::vector<int> RequestedBlocks;    // list of blocks to load
  std::set<std::string> FileNames;

private:
  vtkACosmoReader(const vtkACosmoReader&) = delete;
  void operator=(const vtkACosmoReader&) = delete;
};

#endif
