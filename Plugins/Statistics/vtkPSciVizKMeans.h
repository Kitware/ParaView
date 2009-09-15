#ifndef __vtkPSciVizKMeans_h
#define __vtkPSciVizKMeans_h

#include "vtkSciVizStatistics.h"

class vtkPSciVizKMeans : public vtkSciVizStatistics
{
public:
  static vtkPSciVizKMeans* New();
  vtkTypeRevisionMacro(vtkPSciVizKMeans,vtkSciVizStatistics);
  virtual void PrintSelf( ostream& os, vtkIndent indent );

  // Description:
  // The number of cluster centers.
  // The initial centers will be chosen randomly.
  // In the future the filter will accept an input table of initial cluster positions.
  // The default value of \a K is 5.
  vtkSetMacro(K,int);
  vtkGetMacro(K,int);

  // Description:
  // The maximum number of iterations to perform when converging on cluster centers.
  // The default value is 50 iterations.
  vtkSetMacro(MaxNumIterations,int);
  vtkGetMacro(MaxNumIterations,int);

  // Description:
  // The relative tolerance on cluster centers that will cause early termination of the algorithm.
  // The default value is 0.01: a 1 percent change in cluster coordinates.
  vtkSetMacro(Tolerance,double);
  vtkGetMacro(Tolerance,double);

protected:
  vtkPSciVizKMeans();
  virtual ~vtkPSciVizKMeans();
  virtual const char* GetModelDataTypeName() { return "vtkMultiBlockDataSet"; }
  virtual int RequestModelDataObject( vtkInformation* outInfo );

  virtual int FitModel( vtkDataObject*& model, vtkInformationVector* output, vtkTable* trainingData );
  virtual int AssessData( vtkTable* observations, vtkDataObject* dataset, vtkDataObject* model );

  int K;
  int MaxNumIterations;
  double Tolerance;

private:
  vtkPSciVizKMeans( const vtkPSciVizKMeans& ); // Not implemented.
  void operator = ( const vtkPSciVizKMeans& ); // Not implemented.
};

#endif // __vtkPSciVizKMeans_h
