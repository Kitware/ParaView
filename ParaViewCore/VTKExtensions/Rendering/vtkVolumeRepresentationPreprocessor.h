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
// .NAME vtkVolumeRepresentationPreprocessor - prepare data object for volume rendering
// .SECTION Description
// vtkVolumeRepresentationPreprocessor prepares data objects for volume rendering.  If
// the data object is a data set, then the set is passed through a vtkDataSetTriangleFilter
// before being output as a vtkUnstructuredGrid.  If the data object is a multiblock
// dataset with at least one unstructured grid leaf node, then the unstructured grid
// is extracted using vtkExtractBlock before being passed to the vtkDataSetTriangleFilter.
// If the multiblock dataset contains more than one unstructured grid, the ExtractedBlockIndex
// property may by set to indicate which unstructured grid to volume render.  The TetrahedraOnly
// property may be set and it will be passed to the vtkDataSetTriangleFilter.
//
// .SECTION See Also
// vtkExtractBlock vtkTriangleFilter

#ifndef __vtkVolumeRepresentationPreprocessor_h
#define __vtkVolumeRepresentationPreprocessor_h

#include "vtkUnstructuredGridAlgorithm.h"

class vtkMultiBlockDataSet;
class vtkDataSetTriangleFilter;
class vtkExtractBlock;


class VTK_EXPORT vtkVolumeRepresentationPreprocessor :
  public vtkUnstructuredGridAlgorithm
{
public:
  static vtkVolumeRepresentationPreprocessor *New();
  vtkTypeMacro(vtkVolumeRepresentationPreprocessor, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // When On, the internal triangle filter will cull all 1D and 2D cells from the output.
  // The default is Off.
  void SetTetrahedraOnly(int);
  vtkGetMacro(TetrahedraOnly, int);

  // Description:
  // Sets which block will be extracted for volume rendering.
  // Ignored if input is not multiblock.  Default is 0.
  void SetExtractedBlockIndex(int);
  vtkGetMacro(ExtractedBlockIndex, int);

protected:

  vtkVolumeRepresentationPreprocessor();
  ~vtkVolumeRepresentationPreprocessor();

  vtkUnstructuredGrid *TriangulateDataSet(vtkDataSet *);
  vtkDataSet *MultiBlockToDataSet(vtkMultiBlockDataSet *);

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  int TetrahedraOnly;
  int ExtractedBlockIndex;

  vtkDataSetTriangleFilter *DataSetTriangleFilter;
  vtkExtractBlock *ExtractBlockFilter;

private:

  vtkVolumeRepresentationPreprocessor(const vtkVolumeRepresentationPreprocessor&);  // Not implemented.
  void operator=(const vtkVolumeRepresentationPreprocessor&);  // Not implemented.
};



#endif


