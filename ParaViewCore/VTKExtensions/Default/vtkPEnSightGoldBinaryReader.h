/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPEnSightGoldBinaryReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
/**
 * @class   vtkPEnSightGoldBinaryReader
 *
 * Parallel vtkEnSightGoldBinaryReader.
 *
 * \verbatim
 * This file has been developed as part of the CARRIOCAS (Distributed
 * computation over ultra high optical internet network ) project (
 * http://www.carriocas.org/index.php?lng=ang ) of the SYSTEM@TIC French ICT
 * Cluster (http://www.systematic-paris-region.org/en/index.html) under the
 * supervision of CEA (http://www.cea.fr) and EDF (http://www.edf.fr) by
 * Oxalya (http://www.oxalya.com)
 *
 *  Copyright (c) CEA
 * \endverbatim
*/

#ifndef vtkPEnSightGoldBinaryReader_h
#define vtkPEnSightGoldBinaryReader_h

#include "vtkPEnSightReader.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class vtkMultiBlockDataSet;
class vtkUnstructuredGrid;
class vtkPoints;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPEnSightGoldBinaryReader : public vtkPEnSightReader
{
public:
  static vtkPEnSightGoldBinaryReader* New();
  vtkTypeMacro(vtkPEnSightGoldBinaryReader, vtkPEnSightReader);
  virtual void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkPEnSightGoldBinaryReader();
  ~vtkPEnSightGoldBinaryReader();

  // Returns 1 if successful.  Sets file size as a side action.
  int OpenFile(const char* filename);

  // Returns 1 if successful.  Handles constructing the filename, opening the file and checking
  // if it's binary
  int InitializeFile(const char* filename);

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
    int partId, char line[80], const char* name, vtkMultiBlockDataSet* output) VTK_OVERRIDE;

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
    int partId, char line[80], const char* name, vtkMultiBlockDataSet* output);

  /**
   * Internal function to read in a line up to 80 characters.
   * Returns zero if there was an error.
   */
  int ReadLine(char result[80]);

  //@{
  /**
   * Internal function to read in a single integer.
   * Returns zero if there was an error.
   */
  int ReadInt(int* result);
  int ReadPartId(int* result);
  //@}

  /**
   * Internal function to read in an integer array.
   * Returns zero if there was an error.
   */
  int ReadIntArray(int* result, int numInts);

  /**
   * Internal function to read in a float array.
   * Returns zero if there was an error.
   */
  int ReadFloatArray(float* result, int numFloats);

  /**
   * Read Coordinates, or just skip the part in the file.
   */
  int ReadOrSkipCoordinates(vtkPoints* points, long offset, int partId, bool skip);

  /**
   * Internal method to inject Coordinates and Global Ids at the end
   * of a part read for Unstructured data.
   */
  int InjectCoordinatesAtEnd(vtkUnstructuredGrid* output, long coordinatesOffset, int partId);

  /**
   * Counts the number of timesteps in the geometry file
   * This function assumes the file is already open and returns the
   * number of timesteps remaining in the file
   * The file will be closed after calling this method
   */
  int CountTimeSteps();

  //@{
  /**
   * Read to the next time step in the geometry file.
   */
  int SkipTimeStep();
  int SkipStructuredGrid(char line[256]);
  int SkipUnstructuredGrid(char line[256]);
  int SkipRectilinearGrid(char line[256]);
  int SkipImageData(char line[256]);
  //@}

  int NodeIdsListed;
  int ElementIdsListed;
  int Fortran;

  ifstream* IFile;
  // The size of the file could be used to choose byte order.
  long FileSize;

  // Float Vector Buffer utils
  void GetVectorFromFloatBuffer(int i, float* vector);
  void UpdateFloatBuffer();
  // The buffer
  float** FloatBuffer;
  // The buffer size. Default is 1000
  int FloatBufferSize;
  // The FloatBuffer store the vectors
  // from FloatBufferIndexBegin to FloatBufferIndexBegin + FloatBufferSize
  int FloatBufferIndexBegin;
  // X variable positions of vector number 0 in file
  long FloatBufferFilePosition;
  // Total number of vectors;
  int FloatBufferNumberOfVectors;

private:
  vtkPEnSightGoldBinaryReader(const vtkPEnSightGoldBinaryReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPEnSightGoldBinaryReader&) VTK_DELETE_FUNCTION;
};

#endif
