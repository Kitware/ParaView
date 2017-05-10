/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPEnSightGoldReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
/**
 * @class   vtkPEnSightGoldReader
 *
 * Parallel version of vtkEnSightGoldReader.
 *
 * \verbatim
 *
 * Thanks:
 * This file has been developed as part of the CARRIOCAS (Distributed
 * computation over ultra high optical internet network ) project (
 * http://www.carriocas.org/index.php?lng=ang ) of the SYSTEM@TIC French ICT
 * Cluster (http://www.systematic-paris-region.org/en/index.html) under the
 * supervision of CEA (http://www.cea.fr) and EDF (http://www.edf.fr) by
 * Oxalya (http://www.oxalya.com)
 *
 * Thanks:
 *  Copyright (c) CEA
 * \endverbatim
*/

#ifndef vtkPEnSightGoldReader_h
#define vtkPEnSightGoldReader_h

#include "vtkPEnSightReader.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class UndefPartialInternal;

class vtkMultiBlockDataSet;
class vtkPoints;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPEnSightGoldReader : public vtkPEnSightReader
{
public:
  static vtkPEnSightGoldReader* New();
  vtkTypeMacro(vtkPEnSightGoldReader, vtkPEnSightReader);
  virtual void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkPEnSightGoldReader();
  ~vtkPEnSightGoldReader();

  /**
   * Read the geometry file.  If an error occurred, 0 is returned; otherwise 1.
   */
  virtual int ReadGeometryFile(
    const char* fileName, int timeStep, vtkMultiBlockDataSet* output) VTK_OVERRIDE;

  /**
   * Read the measured geometry file.  If an error occurred, 0 is returned;
   * otherwise 1.
   */
  virtual int ReadMeasuredGeometryFile(
    const char* fileName, int timeStep, vtkMultiBlockDataSet* output) VTK_OVERRIDE;

  /**
   * Read scalars per node for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.  If there will be more than one component in
   * the data array, it is assumed that 0 is the first component added.
   */
  virtual int ReadScalarsPerNode(const char* fileName, const char* description, int timeStep,
    vtkMultiBlockDataSet* output, int measured = 0, int numberOfComponents = 1,
    int component = 0) VTK_OVERRIDE;

  /**
   * Read vectors per node for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  virtual int ReadVectorsPerNode(const char* fileName, const char* description, int timeStep,
    vtkMultiBlockDataSet* output, int measured = 0) VTK_OVERRIDE;

  /**
   * Read tensors per node for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  virtual int ReadTensorsPerNode(const char* fileName, const char* description, int timeStep,
    vtkMultiBlockDataSet* output) VTK_OVERRIDE;

  /**
   * Read scalars per element for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.  If there will be more than one componenet in the
   * data array, it is assumed that 0 is the first component added.
   */
  virtual int ReadScalarsPerElement(const char* fileName, const char* description, int timeStep,
    vtkMultiBlockDataSet* output, int numberOfComponents = 1, int component = 0) VTK_OVERRIDE;

  /**
   * Read vectors per element for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  virtual int ReadVectorsPerElement(const char* fileName, const char* description, int timeStep,
    vtkMultiBlockDataSet* output) VTK_OVERRIDE;

  /**
   * Read tensors per element for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  virtual int ReadTensorsPerElement(const char* fileName, const char* description, int timeStep,
    vtkMultiBlockDataSet* output) VTK_OVERRIDE;

  /**
   * Read an unstructured part (partId) from the geometry file and create a
   * vtkUnstructuredGrid output.  Return 0 if EOF reached. Return -1 if
   * an error occurred.
   */
  virtual int CreateUnstructuredGridOutput(
    int partId, char line[256], const char* name, vtkMultiBlockDataSet* output) VTK_OVERRIDE;

  /**
   * Read a structured part from the geometry file and create a
   * vtkStructuredGrid output.  Return 0 if EOF reached.
   */
  virtual int CreateStructuredGridOutput(
    int partId, char line[256], const char* name, vtkMultiBlockDataSet* output) VTK_OVERRIDE;

  /**
   * Read a structured part from the geometry file and create a
   * vtkRectilinearGrid output.  Return 0 if EOF reached.
   */
  int CreateRectilinearGridOutput(
    int partId, char line[256], const char* name, vtkMultiBlockDataSet* output);

  /**
   * Read a structured part from the geometry file and create a
   * vtkImageData output.  Return 0 if EOF reached.
   */
  int CreateImageDataOutput(
    int partId, char line[256], const char* name, vtkMultiBlockDataSet* output);

  /**
   * Read Coordinates, or just skip the part in the file.
   */
  int ReadOrSkipCoordinates(
    vtkPoints* points, long offset, int partId, int* lineRead, char* line, bool skip);

  /**
   * Internal method to inject Coordinates and Global Ids at the end
   * of a part read for Unstructured data.
   */
  int InjectCoordinatesAtEnd(vtkUnstructuredGrid* output, long coordinatesOffset, int partId);

  /**
   * Skip next line in file if the 'undef' or 'partial' keyword was
   * specified after a sectional keyword
   */
  int CheckForUndefOrPartial(const char* line);

  /**
   * Handle the undef / partial support for EnSight gold
   */
  UndefPartialInternal* UndefPartial;

  int NodeIdsListed;
  int ElementIdsListed;

private:
  vtkPEnSightGoldReader(const vtkPEnSightGoldReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPEnSightGoldReader&) VTK_DELETE_FUNCTION;
};

#endif
