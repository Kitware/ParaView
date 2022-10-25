/*=========================================================================

  Program:   ParaView
  Module:    vtkSelectArraysExtractBlocks.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSelectArraysExtractBlocks
 * @brief   Selects arrays and extracts blocks.
 *
 * vtkSelectArraysExtractBlocks is a meta-filter combining the
 * vtkPassSelectedArrays and vtkExtractBlockUsingDataAssembly filters. It is intended to be used as
 * a pre-processing filter for certain composite writers. It adds the possibility to choose the
 * blocks to write, in addition to the data arrays.
 */

#ifndef vtkSelectArraysExtractBlocks_h
#define vtkSelectArraysExtractBlocks_h

#include "vtkPVVTKExtensionsExtractionModule.h" // For export macro

#include <vtkCompositeDataSetAlgorithm.h>

#include <memory>

class vtkDataArraySelection;

class VTKPVVTKEXTENSIONSEXTRACTION_EXPORT vtkSelectArraysExtractBlocks
  : public vtkCompositeDataSetAlgorithm
{
public:
  static vtkSelectArraysExtractBlocks* New();
  vtkTypeMacro(vtkSelectArraysExtractBlocks, vtkCompositeDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Enable/disable the internal vtkPassSelectedArrays filter.
   * When disabled, this filter passes all input arrays
   * irrespective of the array selection. Default is `true`.
   */
  vtkSetMacro(PassArraysEnabled, bool);
  vtkGetMacro(PassArraysEnabled, bool);
  vtkBooleanMacro(PassArraysEnabled, bool);
  ///@}

  ///@{
  /**
   * Enable/disable the internal vtkExtractBlockUsingDataAssembly filter.
   * When disabled, this filter passes all input blocks
   * irrespective of the block selection. Default is `true`.
   */
  vtkSetMacro(ExtractBlocksEnabled, bool);
  vtkGetMacro(ExtractBlocksEnabled, bool);
  vtkBooleanMacro(ExtractBlocksEnabled, bool);
  ///@}

  ///@{
  /**
   * Convenience methods that call `GetArraySelection` with corresponding
   * association type.
   *
   * Forwarded to internal vtkPassSelectedArrays filter.
   */
  vtkDataArraySelection* GetPointDataArraySelection();
  vtkDataArraySelection* GetCellDataArraySelection();
  vtkDataArraySelection* GetFieldDataArraySelection();
  vtkDataArraySelection* GetVertexDataArraySelection();
  vtkDataArraySelection* GetEdgeDataArraySelection();
  vtkDataArraySelection* GetRowDataArraySelection();
  ///@}

  ///@{
  /**
   * API to set selectors. Multiple selectors can be added using `AddSelector`.
   * The order in which selectors are specified is not preserved and has no
   * impact on the result.
   *
   * `AddSelector` returns true if the selector was added, false if the selector
   * was already specified and hence not added.
   *
   * Forwarded to internal vtkExtractBlockUsingDataAssembly filter.
   *
   * @sa vtkDataAssembly::SelectNodes
   */
  bool AddSelector(const char* selector);
  void ClearSelectors();
  ///@}

  ///@{
  /**
   * Get/Set the active assembly to use. The chosen assembly is used
   * in combination with the selectors specified to determine which blocks
   * are to be extracted.
   *
   * Forwarded to internal vtkExtractBlockUsingDataAssembly filter.
   *
   * By default, this is set to
   * vtkDataAssemblyUtilities::HierarchyName().
   */
  void SetAssemblyName(const char* assemblyName);
  const char* GetAssemblyName() const;
  ///@}

  /**
   * Override GetMTime because we rely on internal filters that have their own MTime
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkSelectArraysExtractBlocks();
  ~vtkSelectArraysExtractBlocks();

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkSelectArraysExtractBlocks(const vtkSelectArraysExtractBlocks&) = delete;
  void operator=(const vtkSelectArraysExtractBlocks&) = delete;

  bool PassArraysEnabled = true;
  bool ExtractBlocksEnabled = true;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
