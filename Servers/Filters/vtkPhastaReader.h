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
// .NAME vtkPhastaReader - Reader for RPI's PHASTA software
// .SECTION Description
// vtkPhastaReader reads RPI's Scorec's PHASTA (Parallel Hierarchic
// Adaptive Stabilized Transient Analysis) dumps.  See
// http://www.scorec.rpi.edu/software_products.html or contact Scorec for
// information on PHASTA.

#ifndef __vtkPhastaReader_h
#define __vtkPhastaReader_h

#include "vtkUnstructuredGridAlgorithm.h"

class vtkPoints;
class vtkDataSetAttributes;
class vtkInformationVector;

class VTK_EXPORT vtkPhastaReader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkPhastaReader *New();
  vtkTypeRevisionMacro(vtkPhastaReader,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify file name of Phasta geometry file to read.
  vtkSetStringMacro(GeometryFileName);
  vtkGetStringMacro(GeometryFileName);

  // Description:
  // Specify file name of Phasta field file to read.
  vtkSetStringMacro(FieldFileName);
  vtkGetStringMacro(FieldFileName);

protected:
  vtkPhastaReader();
  ~vtkPhastaReader();

  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  void ReadGeomFile(char *GeomFileName, 
                    int &firstVertexNo,
                    vtkPoints *points, 
                    int &noOfNodes);
  void ReadFieldFile(char *FieldFileName , 
                     int firstVertexNo, 
                     vtkDataSetAttributes *field, 
                     int &noOfNodes);

private:
  char *GeometryFileName;
  char *FieldFileName;

  int NumberOfVariables; //number of variable in the field file

  static char* StringStripper( const char  istring[] );
  static int cscompare( const char teststring[], 
                        const char targetstring[] );
  static void isBinary( const char iotype[] );
  static size_t typeSize( const char typestring[] );
  static int readHeader( FILE*       fileObject,
                         const char  phrase[],
                         int*        params,
                         int         expect );
  static void SwapArrayByteOrder( void* array, 
                                  int   nbytes, 
                                  int   nItems );
  static void openfile( const char filename[],
                        const char mode[],
                        int*  fileDescriptor );
  static void closefile( int* fileDescriptor, 
                         const char mode[] );
  static void readheader( int* fileDescriptor,
                          const char keyphrase[],
                          void* valueArray,
                          int*  nItems,
                          const char  datatype[],
                          const char  iotype[] );
  static void readdatablock( int*  fileDescriptor,
                             const char keyphrase[],
                             void* valueArray,
                             int*  nItems,
                             const char  datatype[],
                             const char  iotype[] );


  
  
private:
  vtkPhastaReader(const vtkPhastaReader&); // Not implemented
  void operator=(const vtkPhastaReader&); // Not implemented
};

#endif


