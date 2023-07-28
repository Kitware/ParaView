// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSpyPlotReaderMap
 * @brief   Maps strings to vtkSpyPlotUniReaders
 *
 * Extracted from vtkSpyPlotReader
 *-----------------------------------------------------------------------------
 *=============================================================================
 */

#ifndef vtkSpyPlotReaderMap_h
#define vtkSpyPlotReaderMap_h

#include "vtkPVVTKExtensionsIOSPCTHModule.h" //needed for exports

#include <map>    // for std::map
#include <string> // for std::string
#include <vector> // for std::vector

class vtkMultiProcessStream;
class vtkSpyPlotReader;
class vtkSpyPlotUniReader;

class VTKPVVTKEXTENSIONSIOSPCTH_EXPORT vtkSpyPlotReaderMap
{
public:
  typedef std::map<std::string, vtkSpyPlotUniReader*> MapOfStringToSPCTH;
  typedef std::vector<std::string> VectorOfStrings;
  MapOfStringToSPCTH Files;

  // Initialize the file-map. The filename can either be a case file or a spcth
  // file. In case of later, we detect fileseries automatically.
  // This method should ideally be called only on the 0th node to avoid reading
  // reads for meta-data on all the nodes.
  bool Initialize(const char* filename);

  void Clean(vtkSpyPlotUniReader* save);
  vtkSpyPlotUniReader* GetReader(MapOfStringToSPCTH::iterator& it, vtkSpyPlotReader* parent);
  void TellReadersToCheck(vtkSpyPlotReader* parent);

  bool Save(vtkMultiProcessStream& stream);
  bool Load(vtkMultiProcessStream& stream);

private:
  /**
   * This does the updating of the meta data of the case file. Similar to
   * InitializeFromCaseFile, this method builds the vtkSpyPlotReaderMap using the
   * case file.
   */
  bool InitializeFromSpyFile(const char*);

  ///@{
  /**
   * This does the updating of the meta data for a series, when no case file
   * provided. The main role of this method is to build the vtkSpyPlotReaderMap
   * based on the files.
   */
  bool InitializeFromCaseFile(const char*);
};
//@}

#endif

// VTK-HeaderTest-Exclude: vtkSpyPlotReaderMap.h
