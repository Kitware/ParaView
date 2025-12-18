// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright 2011 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
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
#include "vtkPassInputTypeAlgorithm.h"

class vtkCompositeDataSet;
class vtkDataObjectToTable;
class vtkFieldData;
class vtkGenerateStatistics;
class vtkInformationIntegerKey;
class vtkPartitionedDataSetCollection;
class vtkMultiProcessController;
class vtkSciVizStatisticsP;
class vtkStatisticsAlgorithm;

class VTKPVVTKEXTENSIONSFILTERSSTATISTICS_EXPORT vtkSciVizStatistics
  : public vtkPassInputTypeAlgorithm
{
public:
  vtkTypeMacro(vtkSciVizStatistics, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/get the type of field attribute (cell, point, field)
   */
  vtkGetMacro(AttributeMode, int);
  vtkSetMacro(AttributeMode, int);
  ///@}

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

  ///@{
  /**
   * An alternate interface for preparing a selection of arrays in ParaView.
   */
  void EnableAttributeArray(const char* arrName);
  void ClearAttributeArrays();
  ///@}

  ///@{
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
  ///@}

  ///@{
  /**
   * Get/Set the multiprocess controller. If no controller is set, single process is assumed.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

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

  ///@{
  /**
   * Set/get whether this filter should create a model of the input or assess the input or both.
   * This should take on a value from the Tasks enum.
   * The default is MODEL_AND_ASSESS.
   */
  vtkSetMacro(Task, int);
  vtkGetMacro(Task, int);
  ///@}

  /**
   * A key used to mark the output model data object (output port 0) when it is a partitioned
   * dataset collection holding multiple models as opposed to a partitationed dataset collection
   * containing a single model.
   */
  vtkInformationIntegerKey* MULTIPLE_MODELS();

protected:
  vtkSciVizStatistics();
  ~vtkSciVizStatistics() override;

  /**
   * Configure the vtkGenerateStatistics instance on each rank with
   * identical model requests.
   *
   * This requires communication to ensure that:
   * + arrays present on only a subset of ranks are not the subject of a request
   *   (vtkGhostArray is one example, as rank 0 typically will not have this array).
   * + arrays with multiple components only have components present on all ranks
   *   in the request.
   * + ranks with no data have a valid request (that will produce an empty model
   *   rather than an early exit).
   */
  virtual bool PrepareInputArrays(vtkDataObject* inData, vtkGenerateStatistics* filter);

  /**
   * Subclasses must override this method and configure \a filter
   * by calling filter->SetStatisticsAlgorithm().
   */
  virtual bool PrepareAlgorithm(vtkGenerateStatistics* filter) = 0;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  int RequestData(
    vtkInformation* request, vtkInformationVector** input, vtkInformationVector* output) override;

  int AttributeMode;
  int Task;
  double TrainingFraction;
  vtkSciVizStatisticsP* P;
  vtkMultiProcessController* Controller;

private:
  vtkSciVizStatistics(const vtkSciVizStatistics&) = delete;
  void operator=(const vtkSciVizStatistics&) = delete;
};

#endif // vtkSciVizStatistics_h
