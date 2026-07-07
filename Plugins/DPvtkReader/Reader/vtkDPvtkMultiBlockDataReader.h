// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDPvtkMultiBlockDataReader
 * @brief   Reads a multiblock dataset (.vtm) stored in a DPvtk archive.
 *
 * Extends `vtkXMLMultiBlockDataReader`. Two virtual overrides hook into
 * the base class: `ReadXMLInformation` supplies the `.vtm` XML read from
 * the DPvtk archive instead of reading it from disk, and
 * `ReaderSetFileNameOrData` supplies partition VTK XML files read from the DPvtk archive
 *
 * @sa
 * Distributed Parallel Visualization with Zlib compression (DPvz) at
 * https://github.com/sandialabs/dpvz
 */

#ifndef vtkDPvtkMultiBlockDataReader_h
#define vtkDPvtkMultiBlockDataReader_h

#include <vtkDPvtkReaderModule.h> // for export macro
#include <vtkXMLMultiBlockDataReader.h>

VTK_ABI_NAMESPACE_BEGIN

class VTKDPVTKREADER_EXPORT vtkDPvtkMultiBlockDataReader : public vtkXMLMultiBlockDataReader
{
public:
  vtkDPvtkMultiBlockDataReader(const vtkDPvtkMultiBlockDataReader&) = delete;
  void operator=(const vtkDPvtkMultiBlockDataReader&) = delete;

  static vtkDPvtkMultiBlockDataReader* New();
  vtkTypeMacro(vtkDPvtkMultiBlockDataReader, vtkXMLMultiBlockDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  int CanReadFile(VTK_FILEPATH const char* name) override;

protected:
  vtkDPvtkMultiBlockDataReader();
  ~vtkDPvtkMultiBlockDataReader() override;

  ///@{
  /**
   * Overrides the function in base class to supply data read from the DPvtk file
   * instead of the .vtm file.
   */
  int ReadXMLInformation() override;
  /**
   * Overrides the function in the base class to pass the data in the DPvtk container
   * instead of fullFileName.
   * It uses a map between the file name and the rank used at writing and
   * the index inside the rank data where the file was written.
   */
  bool ReaderSetFileNameOrData(vtkXMLReader* reader, const char* fullFileName) override;
  ///@}

  /**
   * Reads meta information about time steps, ranks used at writing
   * and VTK files stored in the DPvtk file. Each node (rank) reads a rank data
   * then the list of files is GatherV to all nodes.
   * The .vtm is broadcast to all nodes.
   * Reading the file names cannot be done without reading the actual
   * rank data (the VTK files are stored in the rank data).  One rank
   * data is cached at each node.
   */
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDPvtkMultiBlockDataReader_h
