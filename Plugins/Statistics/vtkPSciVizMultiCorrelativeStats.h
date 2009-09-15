#ifndef __vtkPSciVizMultiCorrelativeStats_h
#define __vtkPSciVizMultiCorrelativeStats_h

#include "vtkSciVizStatistics.h"

class vtkPSciVizMultiCorrelativeStats : public vtkSciVizStatistics
{
public:
  static vtkPSciVizMultiCorrelativeStats* New();
  vtkTypeRevisionMacro(vtkPSciVizMultiCorrelativeStats,vtkSciVizStatistics);
  virtual void PrintSelf( ostream& os, vtkIndent indent );

protected:
  vtkPSciVizMultiCorrelativeStats();
  virtual ~vtkPSciVizMultiCorrelativeStats();

  virtual const char* GetModelDataTypeName() { return "vtkMultiBlockDataSet"; }
  virtual int RequestModelDataObject( vtkInformation* outInfo );

  virtual int FitModel( vtkDataObject*& model, vtkInformationVector* output, vtkTable* trainingData );
  virtual int AssessData( vtkTable* observations, vtkDataObject* dataset, vtkDataObject* model );

private:
  vtkPSciVizMultiCorrelativeStats( const vtkPSciVizMultiCorrelativeStats& ); // Not implemented.
  void operator = ( const vtkPSciVizMultiCorrelativeStats& ); // Not implemented.
};

#endif // __vtkPSciVizMultiCorrelativeStats_h
