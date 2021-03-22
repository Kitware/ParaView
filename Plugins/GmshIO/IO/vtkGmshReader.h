/*=========================================================================

  Program:   ParaView
  Module:    vtkGmshReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkGmshReader
 *
 * Reader for visualization of high-order polynomial solutions under
 * the Gmsh format. It returns a multiblock dataset.
 * If physical groups are detected, the reader creates an unstructured grid
 * under the root node for each group. Else 4 groups are created, 1 for each
 * dimension.
 */

#ifndef vtkGmshReader_h
#define vtkGmshReader_h

#include "vtkGmshIOModule.h"
#include "vtkMultiBlockDataSetAlgorithm.h"

struct GmshReaderInternal;
struct PhysicalGroup;
class vtkUnstructuredGrid;
class vtkInformation;

class VTKGMSHIO_EXPORT vtkGmshReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkGmshReader* New();
  vtkTypeMacro(vtkGmshReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify file name of Gmsh geometry file to read.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Specify if we create a cell data array containing Gmsh cell IDs.
   * Default is false.
   */
  vtkSetMacro(CreateGmshCellIDArray, bool);
  vtkGetMacro(CreateGmshCellIDArray, bool);
  //@}

  //@{
  /**
   * Specify if we create a point data array containing Gmsh node IDs.
   * Default is false.
   */
  vtkSetMacro(CreateGmshNodeIDArray, bool);
  vtkGetMacro(CreateGmshNodeIDArray, bool);
  //@}

  //@{
  /**
   * Specify if we create a cell data array containing Gmsh entity IDs.
   * Default is false.
   */
  vtkSetMacro(CreateGmshEntityIDArray, bool);
  vtkGetMacro(CreateGmshEntityIDArray, bool);
  //@}

  //@{
  /**
   * Specify if we create a field data array containing the dimension for a dataset.
   * Default is false.
   */
  vtkSetMacro(CreateGmshDimensionArray, bool);
  vtkGetMacro(CreateGmshDimensionArray, bool);
  //@}

  //@{
  /**
   * If true physical groups will be grouped by dimensions with a vtkMultiBlockDataSet
   * for each dimension. Else every physical group will be directly under the root node.
   * Default is true.
   */
  vtkSetMacro(GroupByDimension, bool);
  vtkGetMacro(GroupByDimension, bool);
  //@}

protected:
  vtkGmshReader();
  ~vtkGmshReader() override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  int LoadNodes();
  int FetchData();
  void LoadPhysicalGroups();
  void LoadPhysicalGroupsData();
  void FillGroupElements(PhysicalGroup& group) const;
  void FillGroupEntities(PhysicalGroup& group) const;
  void FillOutputTimeInformation(vtkInformation* outInfo) const;

  void FillSubDataArray(int tag, int idx, int step);
  void FillGrid(vtkUnstructuredGrid* grid, int groupIdx, double time) const;
  double GetActualTime(vtkInformation* outputVector) const;

  char* FileName = nullptr;
  bool CreateGmshCellIDArray = false;
  bool CreateGmshNodeIDArray = false;
  bool CreateGmshEntityIDArray = false;
  bool CreateGmshDimensionArray = false;
  bool GroupByDimension = true;
  GmshReaderInternal* Internal = nullptr;

  vtkGmshReader(const vtkGmshReader&) = delete;
  void operator=(const vtkGmshReader&) = delete;
};

#endif // vtkGmshReader_h
