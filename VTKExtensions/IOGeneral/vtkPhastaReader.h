/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPhastaReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPhastaReader
 * @brief   Reader for RPI's PHASTA software
 *
 * vtkPhastaReader reads RPI's Scorec's PHASTA (Parallel Hierarchic
 * Adaptive Stabilized Transient Analysis) dumps.  See
 * http://www.scorec.rpi.edu/software_products.html or contact Scorec for
 * information on PHASTA.
*/

#ifndef vtkPhastaReader_h
#define vtkPhastaReader_h

#include "vtkPVVTKExtensionsIOGeneralModule.h" //needed for exports
#include "vtkUnstructuredGridAlgorithm.h"

class vtkUnstructuredGrid;
class vtkPoints;
class vtkDataSetAttributes;
class vtkInformationVector;

struct vtkPhastaReaderInternal;

class VTKPVVTKEXTENSIONSIOGENERAL_EXPORT vtkPhastaReader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkPhastaReader* New();
  vtkTypeMacro(vtkPhastaReader, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify file name of Phasta geometry file to read.
   */
  vtkSetStringMacro(GeometryFileName);
  vtkGetStringMacro(GeometryFileName);
  //@}

  //@{
  /**
   * Specify file name of Phasta field file to read.
   */
  vtkSetStringMacro(FieldFileName);
  vtkGetStringMacro(FieldFileName);
  //@}

  //@{
  /**
   * Clear/Set info. in FieldInfoMap for object of vtkPhastaReaderInternal
   */
  void ClearFieldInfo();
  void SetFieldInfo(const char* paraviewFieldTag, const char* phastaFieldTag, int index,
    int numOfComps, int dataDependency, const char* dataType);
  //@}

  void SetCachedGrid(vtkUnstructuredGrid*);
  vtkGetObjectMacro(CachedGrid, vtkUnstructuredGrid);

protected:
  vtkPhastaReader();
  ~vtkPhastaReader() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  void ReadGeomFile(
    char* GeomFileName, int& firstVertexNo, vtkPoints* points, int& noOfNodes, int& noOfCells);
  void ReadFieldFile(
    char* fieldFileName, int firstVertexNo, vtkDataSetAttributes* field, int& noOfNodes);
  void ReadFieldFile(
    char* fieldFileName, int firstVertexNo, vtkUnstructuredGrid* output, int& noOfDatas);

private:
  char* GeometryFileName;
  char* FieldFileName;
  vtkUnstructuredGrid* CachedGrid;

  int NumberOfVariables; // number of variable in the field file

  static char* StringStripper(const char istring[]);
  static int cscompare(const char teststring[], const char targetstring[]);
  static void isBinary(const char iotype[]);
  static size_t typeSize(const char typestring[]);
  static int readHeader(FILE* fileObject, const char phrase[], int* params, int expect);
  static void SwapArrayByteOrder(void* array, int nbytes, int nItems);
  static void openfile(const char filename[], const char mode[], int* fileDescriptor);
  static void closefile(int* fileDescriptor, const char mode[]);
  static void readheader(int* fileDescriptor, const char keyphrase[], void* valueArray, int* nItems,
    const char datatype[], const char iotype[]);
  static void readdatablock(int* fileDescriptor, const char keyphrase[], void* valueArray,
    int* nItems, const char datatype[], const char iotype[]);

private:
  vtkPhastaReaderInternal* Internal;

  vtkPhastaReader(const vtkPhastaReader&) = delete;
  void operator=(const vtkPhastaReader&) = delete;
};

#endif
