/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPrismSurfaceReader.h

=========================================================================*/
// .NAME vtkPrismSurfaceReader - 
// .SECTION Description

#ifndef __vtkPrismSurfaceReader
#define __vtkPrismSurfaceReader

#include "vtkPolyDataAlgorithm.h"

#include "vtkCell.h" // Needed for VTK_CELL_SIZE

class VTK_EXPORT vtkPrismSurfaceReader : public vtkPolyDataAlgorithm 
{
public:
  vtkTypeRevisionMacro(vtkPrismSurfaceReader,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPrismSurfaceReader *New();
 


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

  // Description:

  void SetTableArrayToProcess(const char* name);
  const char* GetTableArrayNameToProcess();
 
  
  // Description:
  // Set whether to read a table array
  void SetTableArrayStatus(const char* name, int flag);
  int GetTableArrayStatus(const char* name);


  //The calculated values used to scale the surface;
  vtkGetVectorMacro(Scale,double,3);
  vtkGetVectorMacro(Range,double,6);


protected:
  vtkPrismSurfaceReader();
  ~vtkPrismSurfaceReader() {}


  double Scale[3];
  double Range[6];


  //BTX 
  class MyInternal;
  MyInternal* Internal;
  //ETX 


  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkPrismSurfaceReader(const vtkPrismSurfaceReader&);  // Not implemented.
  void operator=(const vtkPrismSurfaceReader&);  // Not implemented.
};

#endif
