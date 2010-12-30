/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPrismSESAMEReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPrismSESAMEReader - read SESAME files
// .SECTION Description
// vtkPrismSESAMEReader is a source object that reads SESAME files.
// Currently supported tables include 301, 304, 502, 503, 504, 505, 602
//
// SESAMEReader creates rectilinear grid datasets. The dimension of the
// dataset depends upon the number of densities and temperatures in the table
// Values at certain temperatures and densities are stored as scalars
//

#ifndef __vtkPrismSESAMEReader_h
#define __vtkPrismSESAMEReader_h

#include <vtkPolyDataSource.h>

class vtkIntArray;

class VTK_EXPORT vtkPrismSESAMEReader : public vtkPolyDataSource
{
public:
  static vtkPrismSESAMEReader *New();
  vtkTypeMacro(vtkPrismSESAMEReader, vtkPolyDataSource);

  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the filename to read
  void SetFileName(const char* file);
  // Description:
  // Get the filename to read
  const char* GetFileName();

  // Description:
  // Return whether this is a valid file
  int IsValidFile();

  // Description:
  // Get the number of tables in this file
  int GetNumberOfTableIds();

  // Description:
  // Get the ids of the tables in this file
  int* GetTableIds();

  // Description:
  // Returns the table ids in a data array.
  vtkIntArray* GetTableIdsAsArray();

  // Description:
  // Set the table to read in
  void SetTable(int tableId);
  // Description:
  // Get the table to read in
  int GetTable();

  // Description:
  // Get the number of arrays for the table to read
  int GetNumberOfTableArrayNames();

  // Description:
  // Get the number of arrays for the table to read
  int GetNumberOfTableArrays()
    { return this->GetNumberOfTableArrayNames(); }
  // Description:
  // Get the names of arrays for the table to read
  const char* GetTableArrayName(int index);

  const char* GetTableXAxisName();
  const char* GetTableYAxisName();

  // Description:
  // Set whether to read a table array
  void SetTableArrayStatus(const char* name, int flag);
  int GetTableArrayStatus(const char* name);

protected:

  vtkPrismSESAMEReader();
  virtual ~vtkPrismSESAMEReader();

  //BTX
  class MyInternal;
  MyInternal* Internal;
  //ETX

  int OpenFile();
  void CloseFile();
  void Execute();
  void ExecuteInformation();

  int ReadTableValueLine ( float *v1, float *v2, float *v3,
      float *v4, float *v5);
  int JumpToTable( int tableID );

  void ReadTable();
  void ReadVaporization401Table();
  void ReadCurveFromTable();
private:
  vtkPrismSESAMEReader(const vtkPrismSESAMEReader&);  // Not implemented.
  void operator=(const vtkPrismSESAMEReader&);  // Not implemented.

};

#endif
