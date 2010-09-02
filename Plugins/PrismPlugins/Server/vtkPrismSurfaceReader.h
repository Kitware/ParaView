/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPrismSurfaceReader.h

=========================================================================*/
// .NAME vtkPrismSurfaceReader - 
// .SECTION Description

#ifndef __vtkPrismSurfaceReader
#define __vtkPrismSurfaceReader

#include "vtkPolyDataAlgorithm.h"
#include "vtkStringArray.h"

#include "vtkCell.h" // Needed for VTK_CELL_SIZE

class VTK_EXPORT vtkPrismSurfaceReader : public vtkPolyDataAlgorithm 
{
public:
  vtkTypeMacro(vtkPrismSurfaceReader,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPrismSurfaceReader *New();
 
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


  //The calculated values used to scale the surface;
 // vtkGetVectorMacro(Scale,double,3);
 // vtkGetVectorMacro(Range,double,6);

  void SetXAxisVarName( const char *name );
  void SetYAxisVarName( const char *name );
  void SetZAxisVarName( const char *name );
  const char *GetXAxisVarName();
  const char *GetYAxisVarName(); 
  const char *GetZAxisVarName(); 


  void SetConversions(double dc,double tc,double pc,double ec);
  virtual double *GetConversions();
  virtual void GetConversions (double &_arg1, double &_arg2,double &_arg3,double &_arg4);
  virtual void GetConversions (double _arg[4]);


  void SetXLogScaling(bool);
  void SetYLogScaling(bool);
  void SetZLogScaling(bool);
  bool GetXLogScaling();
  bool GetYLogScaling();
  bool GetZLogScaling();


 
  vtkDoubleArray* GetXRange();
  vtkDoubleArray* GetYRange();
  vtkDoubleArray* GetZRange();

  void SetThresholdXBetween(double lower, double upper);
  void SetThresholdYBetween(double lower, double upper);

 virtual double *GetXThresholdBetween();
 virtual void GetXThresholdBetween (double &_arg1, double &_arg2);
 virtual void GetXThresholdBetween (double _arg[2]);

  vtkGetVector2Macro(YThresholdBetween,double);


  vtkGetVector6Macro(ActualThresholdBounds,double);


  void SetWarpSurface(bool);
  void SetDisplayContours(bool);
  void SetContourVarName( const char *name );
  const char *GetContourVarName(); 
  vtkDoubleArray* GetContourVarRange();


  void SetContourValue(int i, double value);
  double GetContourValue(int i);
  double *GetContourValues();
  void GetContourValues(double *contourValues);

  void SetNumberOfContours(int);


  vtkStringArray* GetAxisVarNames();

 void GetRanges(vtkDoubleArray* RangeArray);

  vtkGetVectorMacro(AspectScale,double,3);


protected:
  vtkPrismSurfaceReader();
  ~vtkPrismSurfaceReader();


  double AspectScale[3];
  //double Range[6];
  double VariableRange[2];
  double XThresholdBetween[2];
  double YThresholdBetween[2];

  double ActualThresholdBounds[6];


 
  bool GetVariableRange (const char *name,vtkDoubleArray*);
  //BTX 
  class MyInternal;
  MyInternal* Internal;
  //ETX 

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  //int FillOutputPortInformation(int port, vtkInformation* info);

private:
  vtkPrismSurfaceReader(const vtkPrismSurfaceReader&);  // Not implemented.
  void operator=(const vtkPrismSurfaceReader&);  // Not implemented.
};

#endif
