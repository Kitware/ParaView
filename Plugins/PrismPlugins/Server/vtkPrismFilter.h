/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPrismFilter.h

=========================================================================*/
// .NAME vtkPrismFilter - 
// .SECTION Description

#ifndef __vtkPrismFilter
#define __vtkPrismFilter



#include "vtkCell.h" // Needed for VTK_CELL_SIZE
#include "vtkStringArray.h"
#include "vtkMultiBlockDataSetAlgorithm.h"
class vtkIntArray;
class vtkDoubleArray;

class VTK_EXPORT vtkPrismFilter : public vtkMultiBlockDataSetAlgorithm 
{
public:
  vtkTypeMacro(vtkPrismFilter,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPrismFilter *New();

  unsigned long GetMTime();

 
  
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

  void SetXAxisVarName( const char *name );
  void SetYAxisVarName( const char *name );
  void SetZAxisVarName( const char *name );
  const char *GetXAxisVarName();
  const char *GetYAxisVarName();
  const char *GetZAxisVarName();


 vtkDoubleArray* GetRanges();



  void SetSESAMEXAxisVarName( const char *name );
  void SetSESAMEYAxisVarName( const char *name );
  void SetSESAMEZAxisVarName( const char *name );
  const char *GetSESAMEXAxisVarName();
  const char *GetSESAMEYAxisVarName(); 
  const char *GetSESAMEZAxisVarName(); 

  void SetSESAMEXLogScaling(bool);
  void SetSESAMEYLogScaling(bool);
  void SetSESAMEZLogScaling(bool);
  bool GetSESAMEXLogScaling();
  bool GetSESAMEYLogScaling();
  bool GetSESAMEZLogScaling();

  void SetSimulationDataThreshold(bool);
  bool GetSimulationDataThreshold();

  void SetSESAMEVariableConversionValues(int i, double value);
  void SetNumberOfSESAMEVariableConversionValues(int);
  double GetSESAMEVariableConversionValue(int i);

  void AddSESAMEVariableConversionNames(char*  value);
  void RemoveAllSESAMEVariableConversionNames();
  const char * GetSESAMEVariableConversionName(int i);

  vtkDoubleArray* GetSESAMEXRange();
  vtkDoubleArray* GetSESAMEYRange();

  void SetThresholdSESAMEXBetween(double lower, double upper);
  void SetThresholdSESAMEYBetween(double lower, double upper);

 virtual double *GetSESAMEXThresholdBetween();
 virtual void GetSESAMEXThresholdBetween (double &_arg1, double &_arg2);
 virtual void GetSESAMEXThresholdBetween (double _arg[2]);


 virtual double *GetSESAMEYThresholdBetween();
 virtual void GetSESAMEYThresholdBetween (double &_arg1, double &_arg2);
 virtual void GetSESAMEYThresholdBetween (double _arg[2]);



 



  void SetWarpSESAMESurface(bool);
  void SetDisplaySESAMEContours(bool);
  void SetSESAMEContourVarName( const char *name );
  const char *GetSESAMEContourVarName(); 
  vtkDoubleArray* GetSESAMEContourVarRange();


  void SetSESAMEContourValue(int i, double value);
  double GetSESAMEContourValue(int i);
  double *GetSESAMEContourValues();
  void GetSESAMEContourValues(double *contourValues);

  void SetNumberOfSESAMEContours(int);


  vtkStringArray* GetSESAMEAxisVarNames();





protected:
  vtkPrismFilter();
  ~vtkPrismFilter();


  //BTX 
  class MyInternal;
  MyInternal* Internal;
  //ETX 

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);



  // see algorithm for more info
  virtual int FillOutputPortInformation(int port, vtkInformation* info);
private:
 // int CalculateValues( double *x, double *f );
  vtkPrismFilter(const vtkPrismFilter&);  // Not implemented.
  void operator=(const vtkPrismFilter&);  // Not implemented.
  int RequestSESAMEData(
    vtkInformation *vtkNotUsed(request),
    vtkInformationVector **vtkNotUsed(inputVector),
    vtkInformationVector *outputVector);
  int RequestGeometryData(
    vtkInformation *vtkNotUsed(request),
    vtkInformationVector **inputVector,
    vtkInformationVector *outputVector);
};

#endif
