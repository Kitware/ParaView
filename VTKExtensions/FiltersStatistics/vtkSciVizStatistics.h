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
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
/**
 * @class   vtkSciVizStatistics
 * @brief   Abstract base class for computing statistics with vtkStatistics
 *
 * This filter either computes a statistical model of
 * a dataset or takes such a model as its second input.
 * Then, the model (however it is obtained) may
 * optionally be used to assess the input dataset.
 *
 * This class serves as a base class that handles table conversion,
 * interfacing with the array selection in the ParaView user interface,
 * and provides a simplified interface to vtkStatisticsAlgorithm.
 * @par Thanks:
 * Thanks to David Thompson and Philippe Pebay from Sandia National Laboratories
 * for implementing this class. Updated by Philippe Pebay, Kitware SAS 2012
*/

#ifndef vtkSciVizStatistics_h
#define vtkSciVizStatistics_h

#include "vtkPVVTKExtensionsFiltersStatisticsModule.h" //needed for exports
#include "vtkTableAlgorithm.h"

class vtkCompositeDataSet;
class vtkDataObjectToTable;
class vtkFieldData;
class vtkInformationIntegerKey;
class vtkMultiBlockDataSet;
class vtkSciVizStatisticsP;
class vtkStatisticsAlgorithm;

class VTKPVVTKEXTENSIONSFILTERSSTATISTICS_EXPORT vtkSciVizStatistics : public vtkTableAlgorithm
{
public:
  vtkTypeMacro(vtkSciVizStatistics, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/get the type of field attribute (cell, point, field)
   */
  vtkGetMacro(AttributeMode, int);
  vtkSetMacro(AttributeMode, int);
  //@}

  /**
   * Return the number of columns available for the current value of \a AttributeMode.
   */
  int GetNumberOfAttributeArrays();

  /**
   * Get the name of the \a n-th array ffor the current value of \a AttributeMode.
   */
  const char* GetAttributeArrayName(int n);

  /**
   * Get the status of the specified array (i.e., whether or not it is a column of interest).
   */
  int GetAttributeArrayStatus(const char* arrName);

  //@{
  /**
   * An alternate interface for preparing a selection of arrays in ParaView.
   */
  void EnableAttributeArray(const char* arrName);
  void ClearAttributeArrays();
  //@}

  //@{
  /**
   * Set/get the amount of data to be used for training.
   * When 0.0 < \a TrainingFraction < 1.0, a randomly-sampled subset of the data is used for
   training.
   * When an assessment is requested, all data (including the training data) is assessed,
   * regardless of the value of TrainingFraction.
   * The default value is 0.1.

   * The random sample of the original dataset (say, of size N) is obtained by choosing N random
   numbers in [0,1).
   * Any sample where the random number is less than \a TrainingFraction is included in the training
   data.
   * Samples are then randomly added or removed from the training data until it is the desired size.
   */
  vtkSetClampMacro(TrainingFraction, double, 0.0, 1.0);
  vtkGetMacro(TrainingFraction, double);
  //@}

  /**\brief Possible tasks the filter can perform.
    *
    * The MODEL_AND_ASSESS task is not recommended;
    * you should never evaluate data with a model if that data was used to create the model.
    * Doing so can result in a too-liberal estimate of model error, especially if overfitting
   * occurs.
    * Because we expect that MODEL_AND_ASSESS, despite being ill-advised, will be frequently used
    * the TrainingFraction parameter has been created.
    */
  enum Tasks
  {
    MODEL_INPUT,     //!< Execute Learn and Derive operations of a statistical engine on the input
                     //! dataset
    CREATE_MODEL,    //!< Create a statistical model from a random subset the input dataset
    ASSESS_INPUT,    //!< Assess the input dataset using a statistical model from input port 1
    MODEL_AND_ASSESS //!< Create a statistical model of the input dataset and use it to assess the
                     //! dataset. This is a bad idea.
  };

  //@{
  /**
   * Set/get whether this filter should create a model of the input or assess the input or both.
   * This should take on a value from the Tasks enum.
   * The default is MODEL_AND_ASSESS.
   */
  vtkSetMacro(Task, int);
  vtkGetMacro(Task, int);
  //@}

  /**
   * A key used to mark the output model data object (output port 0) when it is a multiblock
   * of models (any of which may be multiblock dataset themselves) as opposed to a multiblock
   * dataset containing a single model.
   */
  vtkInformationIntegerKey* MULTIPLE_MODELS();

protected:
  vtkSciVizStatistics();
  ~vtkSciVizStatistics() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  int ProcessRequest(
    vtkInformation* request, vtkInformationVector** input, vtkInformationVector* output) override;
  virtual int RequestDataObject(
    vtkInformation* request, vtkInformationVector** input, vtkInformationVector* output);
  int RequestData(
    vtkInformation* request, vtkInformationVector** input, vtkInformationVector* output) override;
  virtual int RequestData(vtkCompositeDataSet* compDataOu, vtkCompositeDataSet* compModelOu,
    vtkCompositeDataSet* compDataIn, vtkCompositeDataSet* compModelIn, vtkDataObject* singleModel);
  virtual int RequestData(vtkDataObject* observationsOut, vtkDataObject* modelOut,
    vtkDataObject* observationsIn, vtkDataObject* modelIn);

  virtual int PrepareFullDataTable(vtkTable* table, vtkFieldData* dataAttrIn);
  virtual int PrepareTrainingTable(
    vtkTable* trainingTable, vtkTable* fullDataTable, vtkIdType numObservations);

  /**
   * Method subclasses <b>must</b> override to calculate a full model from the given input data.
   * The model should be placed on the first output port of the passed vtkInformationVector
   * as well as returned in the \a model parameter.
   */
  virtual int LearnAndDerive(vtkMultiBlockDataSet* model, vtkTable* inData) = 0;

  /**
   * Method subclasses <b>must</b> override to assess an input table given a model of the proper
   type.
   * The \a dataset parameter contains a shallow copy of input port 0 and should be modified to
   include the assessment.

   * Adding new arrays to point/cell/vertex/edge data should not pose a problem, but any alterations
   * to the dataset itself will probably require that you create a deep copy before modification.

   * @param observations - a table containing the field data of the \a dataset converted to a table
   * @param dataset - a shallow copy of the input dataset that should be altered to include an
   assessment of the output.
   * @param model - the statistical model with which to assess the \a observations.
   */
  virtual int AssessData(
    vtkTable* observations, vtkDataObject* dataset, vtkMultiBlockDataSet* model) = 0;

  /**
   * Subclasses <b>may</b> (but need not) override this function to guarantee that
   * some minimum number of observations are included in the training data.
   * By default, it returns the maximum of:
   * observations->GetNumberOfRows() * this->TrainingFraction and
   * min( observations->GetNumberOfRows(), 100 ).
   * Thus, it will require the entire set of observations unless there are more than 100.

   * @param[in] observations - a table containing the full number of available observations (in
   this process).
   */
  virtual vtkIdType GetNumberOfObservationsForTraining(vtkTable* observations);

  /**
   * A variant of shallow copy that calls vtkDataObject::ShallowCopy() and then
   * for composite datasets, creates clones for each leaf node that then shallow
   * copies the fields and geometry.
   */
  void ShallowCopy(vtkDataObject* out, vtkDataObject* in);

  int AttributeMode;
  int Task;
  double TrainingFraction;
  vtkSciVizStatisticsP* P;

private:
  vtkSciVizStatistics(const vtkSciVizStatistics&) = delete;
  void operator=(const vtkSciVizStatistics&) = delete;
};

#endif // vtkSciVizStatistics_h
