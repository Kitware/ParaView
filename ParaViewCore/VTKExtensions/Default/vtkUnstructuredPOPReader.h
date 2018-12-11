/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredPOPReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkUnstructuredPOPReader
 * @brief   read NetCDF files
 *
 * vtkUnstructuredPOPReader reads NetCDF POP files into a spherical shaped grid.
 * The input file has topologically structured data. The striding and VOI are done with
 * respect to the topologically structured ordering.  Additionally, the
 * z coordinates of the output grid are negated so that the
 * first slice/plane has the highest z-value and the last slice/plane
 * has the lowest z-value.  Note that depth_t is used for the z
 * location of the points.  For VOI and striding, striding is done
 * first and then the VOI is done.  For example, if stride was [1, 2, 3] for
 * a [3600, 2400, 42] grid then the wholeExtent would be [0, 3600, 0, 1200, 0, 14]
 * and then a VOI of [10, 300, 0, 1400, 2 8] would result in a whole
 * extent of [10, 300, 0, 1200, 2, 8] with the first point being [10, 0, 6]
 * in the [3600, 2400, 42] original grid.  The reader also requires
 * a GRID.nc file in the same directory as the main file.  This is used
 * to map from tripolar logical coordinates to lat-lon coordinates.
*/

#ifndef vtkUnstructuredPOPReader_h
#define vtkUnstructuredPOPReader_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkUnstructuredGridAlgorithm.h"

#include <cstddef> // for ptrdiff_t

class vtkCallbackCommand;
class vtkDataArraySelection;
class vtkIdList;
class vtkUnstructuredPOPReaderInternal;
class VTKPointIterator;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkUnstructuredPOPReader
  : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkUnstructuredPOPReader, vtkUnstructuredGridAlgorithm);
  static vtkUnstructuredPOPReader* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * The NetCDF file to open.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Enable subsampling in i,j and k dimensions for the topologically
   * structured input data. Note that if number of points in the
   * z-direction are reduced that the vertical velocity will not be computed.
   */
  vtkSetVector3Macro(Stride, int);
  vtkGetVector3Macro(Stride, int);
  //@}

  //@{
  /**
   * Set the VOI of for the topologically structured input data.
   * Note that if number of points in the z-direction are reduced
   * that the vertical velocity will not be computed.
   */
  vtkSetVector6Macro(VOI, int);
  vtkGetVector6Macro(VOI, int);
  //@}

  //@{
  /**
   * Variable array selection.
   */
  virtual int GetNumberOfVariableArrays();
  virtual const char* GetVariableArrayName(int idx);
  virtual int GetVariableArrayStatus(const char* name);
  virtual void SetVariableArrayStatus(const char* name, int status);
  //@}

  //@{
  /**
   * Set the outer radius of the Earth. By default it is 6371000
   * which assumes the length is in meters.
   */
  vtkSetMacro(Radius, double);
  vtkGetMacro(Radius, double);
  //@}

  //@{
  /**
   * Determine whether or not the input data is being interpolated
   * at the U/vector points or T/scalar points.
   * 0 means unset, 2 means vector field, and 1 means scalar field.
   */
  vtkGetMacro(VectorGrid, int);
  //@}

  //@{
  /**
   * Specify whether or not to compute the vertical velocity component
   * from the horizontal velocity components.  Default is false
   * which signifies do not compute.
   */
  vtkSetMacro(VerticalVelocity, bool);
  vtkGetMacro(VerticalVelocity, bool);
  //@}

protected:
  vtkUnstructuredPOPReader();
  ~vtkUnstructuredPOPReader() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  static void SelectionModifiedCallback(
    vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  static void EventCallback(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  vtkCallbackCommand* SelectionObserver;

  /**
   * The name of the file to be opened.
   */
  char* FileName;

  //@{
  /**
   * If a file is opened, the file name of the opened file.
   */
  char* OpenedFileName;
  vtkSetStringMacro(OpenedFileName);
  //@}

  int Stride[3];

  /**
   * The NetCDF file descriptor.
   */
  int NCDFFD;

  /**
   * The radius of the sphere to be outputted in meters. The default
   * value is 6371000.
   */
  double Radius;

  int VOI[6];

  //@{
  /**
   * State variables so that we know whether or not we are only reading
   * in part of the grid. SubsettingXMin and SubsettingXMax are used
   * so that we can properly connect the grid in the longitudinal
   * direction. We only compute the vertical velocity if we have
   * the full height resolution of the grid.
   */
  bool SubsettingXMin;
  bool SubsettingXMax;
  bool ReducedHeightResolution;
  //@}

  /**
   * Specify whether the grid points are at the vector field (U_LAT and U_LON) locations
   * or the scalar field (T_LAT and T_LON) locations or unset.
   * 0 means unset, 2 means vector field, and 1 means scalar field.
   */
  int VectorGrid;

  /**
   * If it is a vector grid (i.e. VectorGrid=2) then specify whether or not
   * to compute the vertical velocity.  This can be a costly computation
   * so if the vertical velocity is not needed then keep this off
   * (the default).
   */
  bool VerticalVelocity;

  /**
   * Transform the grid from a topologically structured grid to a sphere
   * shaped grid and do any vector transformations on field data that
   * is needed.
   */
  bool Transform(vtkUnstructuredGrid* grid, size_t* start, size_t* count, int* wholeExtent,
    int* subExtent, int numberOfGhostLevels, int wrapped, int piece, int numberOfPieces);

  /**
   * Given the meta data about the grid partitioning, read in the
   * data from the file and create the unstructured grid.
   */
  int ProcessGrid(
    vtkUnstructuredGrid* grid, int piece, int numberOfPieces, int numberOfGhostLevels);

  /**
   * Reads the meta data from the NetCDF file for information like what
   * variables exist and the dimensions of the grids and variables.
   * Returns true for success.  Also fills out wholeExtent.
   */
  bool ReadMetaData(int wholeExtent[6]);

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Adds a point data field given by varidp in the NetCDF file
   * to the grid's point data.  If the units is "centimeter/s"
   * it scales the value by 1/100 (i.e. meter/s).
   */
  void LoadPointData(vtkUnstructuredGrid* grid, int netCDFFD, int varidp, size_t* start,
    size_t* count, ptrdiff_t* rStride, const char* arrayName);

  /**
   * Compute the vertical velocity component and add it into
   * the velocity field.
   */
  void ComputeVerticalVelocity(vtkUnstructuredGrid* grid, int* wholeExtent, int* subExtent,
    int numberOfGhostLevels, int latlonFileId);

  /**
   * If the reader is being run in parallel, do the necessary
   * communication to finish the vertical velocity integration
   * on each process.
   */
  void CommunicateParallelVerticalVelocity(int* wholeExtent, int* subExtent,
    int numberOfGhostLevels, VTKPointIterator& pointIterator, double* w);

  /**
   * Given ijk indices with respect to WholeExtent, return the
   * process that is considered owning this point. Here the process
   * that owns the piece will have compuated a valid vertical velocity
   * value to share with other processes that need the integrated
   * vertical velocity value. This method returns -1 if no
   * appropriate piece is found.
   */
  int GetPointOwnerPiece(int iIndex, int jIndex, int kKindex, int numberOfPieces,
    int numberOfGhostLevels, int* wholeExtent);

  /**
   * Given ijk indices, fill pieceIds with the pieces which
   * contain need this point for computing the vertical velocity.
   * Note that the pieces with ghost points only used the reader are not
   * included in the returned Id list.  The indices are with
   * respect to WholeExtent.
   */
  void GetPiecesNeedingPoint(int iIndex, int jIndex, int kKindex, int numberOfPieces,
    int numberOfGhostLevels, int* wholeExtent, vtkIdList* pieceIds);

  /**
   * Given piece, numberOfPieces, numberOfGhostLevels, and wholeExtent,
   * this method fills in subExtent. This method returns true for success.
   */
  bool GetExtentInformation(
    int piece, int numberOfPieces, int numberOfGhostLevels, int* wholeExtent, int* subExtent);

  /**
   * Build up the requested ghost information.  Note that if the
   * vertical velocity is computed that the reader needs an extra
   * layer of ghost cells.  This extra layer is removed before
   * RequestData() is exited.  The method returns true for success.
   */
  bool BuildGhostInformation(vtkUnstructuredGrid* grid, int numberOfGhostLevels, int* wholeExtent,
    int* subExtent, int wrapped, int piece, int numberOfPieces);

private:
  vtkUnstructuredPOPReader(const vtkUnstructuredPOPReader&) = delete;
  void operator=(const vtkUnstructuredPOPReader&) = delete;

  vtkUnstructuredPOPReaderInternal* Internals;
};
#endif
