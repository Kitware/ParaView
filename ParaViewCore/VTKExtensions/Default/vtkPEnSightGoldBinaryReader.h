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
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPEnSightGoldBinaryReader();
  ~vtkPEnSightGoldBinaryReader() override;

  // Returns 1 if successful.  Sets file size as a side action.
  int OpenFile(const char* filename);

  // Returns 1 if successful.  Handles constructing the filename, opening the file and checking
  // if it's binary
  int InitializeFile(const char* filename);

  /**
   * Read the geometry file.  If an error occurred, 0 is returned; otherwise 1.
   */
  int ReadGeometryFile(const char* fileName, int timeStep, vtkMultiBlockDataSet* output) override;

  /**
   * Read the measured geometry file.  If an error occurred, 0 is returned;
   * otherwise 1.
   */
  int ReadMeasuredGeometryFile(
    const char* fileName, int timeStep, vtkMultiBlockDataSet* output) override;

  /**
   * Read scalars per node for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.  If there will be more than one component in
   * the data array, it is assumed that 0 is the first component added.
   */
  int ReadScalarsPerNode(const char* fileName, const char* description, int timeStep,
    vtkMultiBlockDataSet* output, int measured = 0, int numberOfComponents = 1,
    int component = 0) override;

  /**
   * Read vectors per node for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  int ReadVectorsPerNode(const char* fileName, const char* description, int timeStep,
    vtkMultiBlockDataSet* output, int measured = 0) override;

  /**
   * Read tensors per node for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  int ReadTensorsPerNode(const char* fileName, const char* description, int timeStep,
    vtkMultiBlockDataSet* output) override;

  /**
   * Read scalars per element for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.  If there will be more than one componenet in the
   * data array, it is assumed that 0 is the first component added.
   */
  int ReadScalarsPerElement(const char* fileName, const char* description, int timeStep,
    vtkMultiBlockDataSet* output, int numberOfComponents = 1, int component = 0) override;

  /**
   * Read vectors per element for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  int ReadVectorsPerElement(const char* fileName, const char* description, int timeStep,
    vtkMultiBlockDataSet* output) override;

  /**
   * Read tensors per element for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  int ReadTensorsPerElement(const char* fileName, const char* description, int timeStep,
    vtkMultiBlockDataSet* output) override;

  /**
   * Read an unstructured part (partId) from the geometry file and create a
   * vtkUnstructuredGrid output.  Return 0 if EOF reached. Return -1 if
   * an error occurred.
   */
  int CreateUnstructuredGridOutput(
    int partId, char line[80], const char* name, vtkMultiBlockDataSet* output) override;

  /**
   * Read a structured part from the geometry file and create a
   * vtkStructuredGrid output.  Return 0 if EOF reached.
   */
  int CreateStructuredGridOutput(
    int partId, char line[256], const char* name, vtkMultiBlockDataSet* output) override;

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
  void GetVectorFromFloatBuffer(vtkIdType i, float* vector);
  void UpdateFloatBuffer();
  // The buffer
  float** FloatBuffer;
  // The buffer size. Default is 1000
  vtkIdType FloatBufferSize;
  // The FloatBuffer store the vectors
  // from FloatBufferIndexBegin to FloatBufferIndexBegin + FloatBufferSize
  vtkIdType FloatBufferIndexBegin;
  // X variable positions of vector number 0 in file
  vtkIdType FloatBufferFilePosition;
  // Total number of vectors;
  vtkIdType FloatBufferNumberOfVectors;

private:
  vtkPEnSightGoldBinaryReader(const vtkPEnSightGoldBinaryReader&) = delete;
  void operator=(const vtkPEnSightGoldBinaryReader&) = delete;
};

#endif
