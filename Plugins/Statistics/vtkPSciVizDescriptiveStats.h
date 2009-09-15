#ifndef __vtkPSciVizDescriptiveStats_h
#define __vtkPSciVizDescriptiveStats_h

#include "vtkSciVizStatistics.h"

class vtkPSciVizDescriptiveStats : public vtkSciVizStatistics
{
public:
  static vtkPSciVizDescriptiveStats* New();
  vtkTypeRevisionMacro(vtkPSciVizDescriptiveStats,vtkSciVizStatistics);
  virtual void PrintSelf( ostream& os, vtkIndent indent );

  vtkSetMacro(SignedDeviations,int);
  vtkGetMacro(SignedDeviations,int);

protected:
  vtkPSciVizDescriptiveStats();
  virtual ~vtkPSciVizDescriptiveStats();

  virtual int FitModel( vtkDataObject*& model, vtkInformationVector* output, vtkTable* trainingData );
  virtual int AssessData( vtkTable* observations, vtkDataObject* dataset, vtkDataObject* model );

  int SignedDeviations;

private:
  vtkPSciVizDescriptiveStats( const vtkPSciVizDescriptiveStats& ); // Not implemented.
  void operator = ( const vtkPSciVizDescriptiveStats& ); // Not implemented.
};

#endif // __vtkPSciVizDescriptiveStats_h
