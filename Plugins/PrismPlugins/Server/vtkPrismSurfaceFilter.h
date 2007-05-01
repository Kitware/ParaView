/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPrismSurfaceFilter.h

=========================================================================*/
// .NAME vtkPrismSurfaceFilter - 
// .SECTION Description

#ifndef __vtkPrismSurfaceFilter
#define __vtkPrismSurfaceFilter

#include "vtkPolyDataAlgorithm.h"

#include "vtkCell.h" // Needed for VTK_CELL_SIZE

class VTK_EXPORT vtkPrismSurfaceFilter : public vtkPolyDataAlgorithm 
{
public:
  vtkTypeRevisionMacro(vtkPrismSurfaceFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPrismSurfaceFilter *New();
 


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


protected:
  vtkPrismSurfaceFilter();
  ~vtkPrismSurfaceFilter() {}


  //BTX 
  class MyInternal;
  MyInternal* Internal;
  //ETX 


  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkPrismSurfaceFilter(const vtkPrismSurfaceFilter&);  // Not implemented.
  void operator=(const vtkPrismSurfaceFilter&);  // Not implemented.
};

#endif
