/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPrismGeometryFilter.h

=========================================================================*/
// .NAME vtkPrismGeometryFilter - 
// .SECTION Description

#ifndef __vtkPrismGeometryFilter
#define __vtkPrismGeometryFilter

#include "vtkPointDataToCellData.h"

//#include "vtkCell.h" // Needed for VTK_CELL_SIZE

class VTK_EXPORT vtkPrismGeometryFilter : public vtkPointDataToCellData 
{
public:
  vtkTypeMacro(vtkPrismGeometryFilter,vtkPointDataToCellData);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPrismGeometryFilter *New();
 

  // Description:
  // Set the table to read in
  void SetTable(int tableId);
  // Description:
  // Get the table to read in
  int GetTable();

  void SetXAxisVarName( const char *name );
  void SetYAxisVarName( const char *name );
  void SetZAxisVarName( const char *name );
  const char *GetXAxisVarName();
  const char *GetYAxisVarName(); 
  const char *GetZAxisVarName(); 

protected:
  vtkPrismGeometryFilter();
  ~vtkPrismGeometryFilter() {}

   int CalculateValues( double *x, double *f );
  //BTX 
  class MyInternal;
  MyInternal* Internal;
  //ETX 


  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkPrismGeometryFilter(const vtkPrismGeometryFilter&);  // Not implemented.
  void operator=(const vtkPrismGeometryFilter&);  // Not implemented.
};

#endif
