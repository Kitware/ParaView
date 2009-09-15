#ifndef __vtkPSciVizContingencyStats_h
#define __vtkPSciVizContingencyStats_h

#include "vtkSciVizStatistics.h"

class vtkPSciVizContingencyStats : public vtkSciVizStatistics
{
public:
  static vtkPSciVizContingencyStats* New();
  vtkTypeRevisionMacro(vtkPSciVizContingencyStats,vtkSciVizStatistics);
  virtual void PrintSelf( ostream& os, vtkIndent indent );

protected:
  vtkPSciVizContingencyStats();
  virtual ~vtkPSciVizContingencyStats();
  virtual const char* GetModelDataTypeName() { return "vtkMultiBlockDataSet"; }
  virtual int RequestModelDataObject( vtkInformation* outInfo );

  virtual int FitModel( vtkDataObject*& model, vtkInformationVector* output, vtkTable* trainingData );
  virtual int AssessData( vtkTable* observations, vtkDataObject* dataset, vtkDataObject* model );

private:
  vtkPSciVizContingencyStats( const vtkPSciVizContingencyStats& ); // Not implemented.
  void operator = ( const vtkPSciVizContingencyStats& ); // Not implemented.
};

#endif // __vtkPSciVizContingencyStats_h
