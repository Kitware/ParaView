// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDPvtkMultiBlockDataReader
 * @brief   Reads a time series of multiblock datasets (.vtm) stored in a DPvtk file.
 *
 * A DPvtk file, written with the DPvz library, stores all the time steps of
 * a simulation. For each time step it stores, compressed, the data written
 * by each simulation rank. That data is a set of VTK XML files: the data of
 * rank 0 contains the `.vtm` file that describes the block structure, and
 * the partition files it references (`.vtr`, `.vtu`, ...) are spread over
 * all ranks. A `.pvd` file stored by rank 0 is ignored.
 *
 * The reader extends `vtkXMLMultiBlockDataReader` and reads the VTK XML files
 * from memory instead of from disk: `ReadXMLInformation` supplies the `.vtm`
 * and `ReaderSetFileNameOrData` supplies the partition files.
 *
 * Time steps are read from the DPvtk table of contents. For a requested
 * time, the reader loads the first time step with a time greater than or
 * equal to it, or the last time step, as `vtkPVDReader` does.
 *
 * In parallel, the number of reader processes does not need to match
 * the number of ranks used for writing. The file lists are then
 * gathered on all processes and the `.vtm` is broadcast from process
 * 0. Partition files are then distributed among processes by the
 * superclass. Each process caches the data of one writer
 * rank. Loading a time step is a collective operation: if it fails on
 * one process, all processes report an error.
 *
 * When DPvz is built with MPI, the global controller must be a
 * `vtkMPIController`.
 *
 * @warning
 * The block structure reported to the pipeline (`COMPOSITE_DATA_META_DATA`)
 * is the one of the time step loaded in `RequestInformation`. Changes of the
 * block structure between time steps are not reported to downstream filters.
 *
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
  /**
   * Returns 1 if `name` can be opened as a DPvtk file.
   * With MPI, this is called only on process 0, so the file is opened with
   * `MPI_COMM_SELF`.
   */
  int CanReadFile(VTK_FILEPATH const char* name) override;

protected:
  vtkDPvtkMultiBlockDataReader();
  ~vtkDPvtkMultiBlockDataReader() override;

  /**
   * Parses the `.vtm` of the loaded time step, read from the DPvtk file,
   * instead of reading a `.vtm` file from disk.
   */
  int ReadXMLInformation() override;
  /**
   * Passes the partition file `fullFileName`, read from the DPvtk file, to
   * `reader` as an input string. The file is found using a map from its name
   * to the writer rank that stores it and its index in that rank's data.
   * If that rank's data is not cached, it is read and replaces the cached one.
   */
  bool ReaderSetFileNameOrData(vtkXMLReader* reader, const char* fullFileName) override;

  /**
   * Opens the DPvtk file and reads its table of contents to report the time
   * steps. Loads a time step to provide the `.vtm` structure: it reads the
   * data of all writer ranks, distributed among processes, to find which
   * rank stores each partition file. This is a collective operation.
   */
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  /**
   * Loads the requested time step, if it is not already loaded, parses its
   * `.vtm` and reads the partition files assigned to this process.
   * Loading a time step is a collective operation.
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDPvtkMultiBlockDataReader_h
