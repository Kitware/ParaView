/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRepresentationPreprocessor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVolumeRepresentationPreprocessor
 * @brief   prepare data object for volume rendering
 *
 * vtkVolumeRepresentationPreprocessor prepares data objects for volume rendering.  If
 * the data object is a data set, then the set is passed through a vtkDataSetTriangleFilter
 * before being output as a vtkUnstructuredGrid.  If the data object is a multiblock
 * dataset with at least one unstructured grid leaf node, then the unstructured grid
 * is extracted using vtkExtractBlockUsingDataAssembly and vtkMergeBlocks.  The TetrahedraOnly
 * property may be set and it will be passed to the vtkDataSetTriangleFilter.
 *
 * @sa
 * vtkExtractBlockUsingDataAssembly vtkTriangleFilter
*/

#ifndef vtkVolumeRepresentationPreprocessor_h
#define vtkVolumeRepresentationPreprocessor_h

#include "vtkNew.h"                                   // for vtkNew
#include "vtkPVVTKExtensionsFiltersRenderingModule.h" // needed for export macro
#include "vtkSmartPointer.h"                          // for vtkSmartPointer
#include "vtkUnstructuredGridAlgorithm.h"

class vtkCompositeDataSet;
class vtkExtractBlockUsingDataAssembly;

class VTKPVVTKEXTENSIONSFILTERSRENDERING_EXPORT vtkVolumeRepresentationPreprocessor
  : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkVolumeRepresentationPreprocessor* New();
  vtkTypeMacro(vtkVolumeRepresentationPreprocessor, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * When On, the internal triangle filter will cull all 1D and 2D cells from the output.
   * The default is Off.
   */
  vtkSetMacro(TetrahedraOnly, int);
  vtkGetMacro(TetrahedraOnly, int);
  //@}

  //@{
  /**
   * Forwarded to internal vtkExtractBlockUsingDataAssembly to subset the input
   * composite dataset. This has no effect if input is not a composite dataset.
   */
  bool AddSelector(const char* selector);
  void SetSelector(const char* selector);
  void ClearSelectors();
  void SetAssemblyName(const char*);
  //@}

protected:
  vtkVolumeRepresentationPreprocessor();
  ~vtkVolumeRepresentationPreprocessor() override;

  vtkSmartPointer<vtkUnstructuredGrid> Tetrahedralize(vtkDataObject*);
  vtkSmartPointer<vtkDataSet> ExtractDataSet(vtkCompositeDataSet*);

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  int TetrahedraOnly;

private:
  vtkVolumeRepresentationPreprocessor(const vtkVolumeRepresentationPreprocessor&) = delete;
  void operator=(const vtkVolumeRepresentationPreprocessor&) = delete;

  vtkNew<vtkExtractBlockUsingDataAssembly> Extractor;
};

#endif
