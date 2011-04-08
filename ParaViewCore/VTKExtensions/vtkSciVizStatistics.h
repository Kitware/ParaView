/*=========================================================================

  Program:   ParaView
  Module:    vtkSciVizStatistics.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSciVizStatistics - Abstract base class for computing statistics on scientific datasets.
// .SECTION Description
// This filter either computes a statistical model of
// a dataset or takes such a model as its second input.
// Then, the model (however it is obtained) may
// optionally be used to assess the input dataset.
//
// This class serves as a base class that handles table conversion,
// interfacing with the array selection in the ParaView user interface,
// and provides a simplified interface to vtkStatisticsAlgorithm.

#ifndef __vtkSciVizStatistics_h
#define __vtkSciVizStatistics_h

#include "vtkTableAlgorithm.h"

class vtkCompositeDataSet;
class vtkDataObjectToTable;
class vtkFieldData;
class vtkInformationIntegerKey;
class vtkMultiBlockDataSet;
class vtkSciVizStatisticsP;
class vtkStatisticsAlgorithm;

class VTK_EXPORT vtkSciVizStatistics : public vtkTableAlgorithm
{
public:
  vtkTypeMacro(vtkSciVizStatistics,vtkTableAlgorithm);
  virtual void PrintSelf( ostream& os, vtkIndent indent );

  // Description:
  // Set/get the type of field attribute (cell, point, field)
  vtkGetMacro(AttributeMode,int);
  vtkSetMacro(AttributeMode,int);

  /// Return the number of columns available for the current value of \a AttributeMode.
  int GetNumberOfAttributeArrays();

  /// Get the name of the \a n-th array ffor the current value of \a AttributeMode.
  const char* GetAttributeArrayName( int n );
  /// Get the status of the specified array (i.e., whether or not it is a column of interest).
  int GetAttributeArrayStatus( const char* arrName );
  /// Set the status of the specified array (i.e., whether or not it is a column of interest).
  void SetAttributeArrayStatus( const char* arrName, int stat );

  /// An alternate interface for preparing a selection of arrays in ParaView.
  void EnableAttributeArray( const char* arrName );
  void ClearAttributeArrays();

  // Description:
  // Set/get the amount of data to be used for training.
  // When 0.0 < \a TrainingFraction < 1.0, a randomly-sampled subset of the data is used for training.
  // When an assessment is requested, all data (including the training data) is assessed,
  // regardless of the value of TrainingFraction.
  // The default value is 0.1.
  //
  // The random sample of the original dataset (say, of size N) is obtained by choosing N random numbers in [0,1).
  // Any sample where the random number is less than \a TrainingFraction is included in the training data.
  // Samples are then randomly added or removed from the training data until it is the desired size.
  vtkSetClampMacro(TrainingFraction,double,0.0,1.0);
  vtkGetMacro(TrainingFraction,double);

  //BTX
  /**\brief Possible tasks the filter can perform.
    *
    * The MODEL_AND_ASSESS task is not recommended;
    * you should never evaluate data with a model if that data was used to create the model.
    * Doing so can result in a too-liberal estimate of model error, especially if overfitting occurs.
    * Because we expect that MODEL_AND_ASSESS, despite being ill-advised, will be frequently used
    * the TrainingFraction parameter has been created.
    * By fitting a model using only a small fraction of the data present,
    * the hope is that the remaining data will indicate whether overfitting occurred.
    */
  enum Tasks
    {
    FULL_STATISTICS,  //!< Compute statistics on all of the data
    CREATE_MODEL,     //!< Create a statistical model from a random subset the input dataset
    ASSESS_INPUT,     //!< Assess the input dataset using a statistical model from input port 1
    MODEL_AND_ASSESS  //!< Create a statistical model of the input dataset and use it to assess the dataset. This is a bad idea.
    };
  //ETX

  // Description:
  // Set/get whether this filter should create a model of the input or assess the input or both.
  // This should take on a value from the Tasks enum.
  // The default is MODEL_AND_ASSESS.
  vtkSetMacro(Task,int);
  vtkGetMacro(Task,int);

  // Description:
  // A key used to mark the output model data object (output port 0) when it is a multiblock
  // of models (any of which may be multiblock dataset themselves) as opposed to a multiblock
  // dataset containing a single model.
  vtkInformationIntegerKey* MULTIPLE_MODELS();

protected:
  vtkSciVizStatistics();
  virtual ~vtkSciVizStatistics();

  virtual int FillInputPortInformation( int port, vtkInformation* info );
  virtual int FillOutputPortInformation( int port, vtkInformation* info );

  virtual int ProcessRequest( vtkInformation* request, vtkInformationVector** input, vtkInformationVector* output );
  virtual int RequestDataObject( vtkInformation* request, vtkInformationVector** input, vtkInformationVector* output );
  virtual int RequestData( vtkInformation* request, vtkInformationVector** input, vtkInformationVector* output );
  virtual int RequestData(
    vtkCompositeDataSet* compDataOu, vtkCompositeDataSet* compModelOu,
    vtkCompositeDataSet* compDataIn, vtkCompositeDataSet* compModelIn,
    vtkDataObject* singleModel );
  virtual int RequestData(
    vtkDataObject* observationsOut, vtkDataObject* modelOut,
    vtkDataObject* observationsIn, vtkDataObject* modelIn );

  virtual int PrepareFullDataTable( vtkTable* table, vtkFieldData* dataAttrIn );
  virtual int PrepareTrainingTable( vtkTable* trainingTable, vtkTable* fullDataTable, vtkIdType numObservations );

  // Description:
  // Method subclasses <b>may</b> override to change the output model type from a vtkTable to some other Type.
  //
  // The name of a vtkDataObject subclass should be returned.
  // If you override this method, you <b>may</b> also override CreateModelDataType();
  // if you don't, then the instantiator will be used to create an object of the type
  // specified by GetModelDataTypeName().
  virtual const char* GetModelDataTypeName() { return "vtkMultiBlockDataSet"; }

  // Description:
  // Method subclasses <b>may</b> override to change the output model type from a vtkTable to some other Type.
  //
  // A new instance of a vtkDataObject subclass should be returned.
  // If you override this method, you must also override GetModelDataTypeName().
  virtual vtkDataObject* CreateModelDataType();

  // Description:
  // Method subclasses <b>must</b> override to fit a model to the given training data.
  // The model should be placed on the first output port of the passed vtkInformationVector
  // as well as returned in the \a model parameter.
  virtual int FitModel( vtkMultiBlockDataSet* model, vtkTable* trainingData ) = 0;

  // Description:
  // Method subclasses <b>must</b> override to assess an input table given a model of the proper type.
  // The \a dataset parameter contains a shallow copy of input port 0 and should be modified to include the assessment.
  //
  // Adding new arrays to point/cell/vertex/edge data should not pose a problem, but any alterations
  // to the dataset itself will probably require that you create a deep copy before modification.
  //
  // @param observations - a table containing the field data of the \a dataset converted to a table
  // @param dataset - a shallow copy of the input dataset that should be altered to include an assessment of the output.
  // @param model - the statistical model with which to assess the \a observations.
  virtual int AssessData( vtkTable* observations, vtkDataObject* dataset, vtkMultiBlockDataSet* model ) = 0;

  // Description:
  // Subclasses <b>may</b> (but need not) override this function to guarantee that
  // some minimum number of observations are included in the training data.
  // By default, it returns the maximum of:
  //   observations->GetNumberOfRows() * this->TrainingFraction and
  //   min( observations->GetNumberOfRows(), 100 ).
  // Thus, it will require the entire set of observations unless there are more than 100.
  //
  // @params[in] observations - a table containing the full number of available observations (in this process).
  virtual vtkIdType GetNumberOfObservationsForTraining( vtkTable* observations );

  int AttributeMode;
  int Task;
  double TrainingFraction;
  vtkSciVizStatisticsP* P;

private:
  vtkSciVizStatistics( const vtkSciVizStatistics& ); // Not implemented.
  void operator = ( const vtkSciVizStatistics& ); // Not implemented.
};

#endif // __vtkSciVizStatistics_h
