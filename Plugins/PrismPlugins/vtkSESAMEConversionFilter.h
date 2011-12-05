/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSESAMEConversionFilter.h

=========================================================================*/
// .NAME vtkSESAMEConversionFilter
// .SECTION Description 

#ifndef __vtkSESAMEConversionFilter_h
#define __vtkSESAMEConversionFilter_h

#include "vtkPolyDataAlgorithm.h"
#include "vtkSmartPointer.h"

class vtkStringArray;
class vtkDoubleArray;

class vtkSESAMEConversionFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkSESAMEConversionFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with initial extent (0,100, 0,100, 0,0) (i.e., a k-plane).
  static vtkSESAMEConversionFilter *New();

  void SetVariableConversionValues(int i, double value);
  void SetNumberOfVariableConversionValues(int);
  double GetVariableConversionValue(int i);

  void AddVariableConversionNames( char*  value);
  void RemoveAllVariableConversionNames();
  const char * GetVariableConversionName(int i);

protected:
  vtkSESAMEConversionFilter();
  ~vtkSESAMEConversionFilter() {};

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  
  vtkSmartPointer<vtkStringArray> VariableConversionNames;
  vtkSmartPointer<vtkDoubleArray> VariableConversionValues;

private:
  vtkSESAMEConversionFilter(const vtkSESAMEConversionFilter&);  // Not implemented.
  void operator=(const vtkSESAMEConversionFilter&);  // Not implemented.
};

#endif
