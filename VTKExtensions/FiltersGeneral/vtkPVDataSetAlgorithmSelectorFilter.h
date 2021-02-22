/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVDataSetAlgorithmSelectorFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVDataSetAlgorithmSelectorFilter
 * is a generic vtkAlgorithm that allow the user to register
 * several vtkAlgorithm to it and be able to switch the active
 * one on the fly.
 *
 * The idea behind that filter is to merge the usage of any number of existing
 * vtk filter and allow to easily switch from one implementation to another
 * without changing anything in your pipeline.
*/

#ifndef vtkPVDataSetAlgorithmSelectorFilter_h
#define vtkPVDataSetAlgorithmSelectorFilter_h

#include "vtkAlgorithm.h"
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports

class vtkCallbackCommand;

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPVDataSetAlgorithmSelectorFilter
  : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkPVDataSetAlgorithmSelectorFilter, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPVDataSetAlgorithmSelectorFilter* New();

  /**
   * Register a new filter that can be used underneath in the requestData call.
   * The return value is the index of that registered filter that should be use
   * to activate it later on. (This number can became wrong in case you remove
   * some previous registered filter)
   */
  int RegisterFilter(vtkAlgorithm* filter);

  /**
   * UnRegister an existing filter that was previously registered
   */
  void UnRegisterFilter(int index);

  /**
   * Remove all the registered filters.
   */
  void ClearFilters();

  /**
   * Return the current number of registered filters
   */
  int GetNumberOfFilters();

  /**
   * Return the filter that lies at the given index of the filters registration queue.
   */
  vtkAlgorithm* GetFilter(int index);

  /**
   * Return the current active filter if any otherwise return nullptr
   */
  vtkAlgorithm* GetActiveFilter();

  /**
   * Set the active filter based on the given index of the filters registration
   * queue. And return the corresponding active filter.
   */
  virtual vtkAlgorithm* SetActiveFilter(int index);

  /**
   * Override GetMTime because we delegate to other filters to do the real work
   */
  vtkMTimeType GetMTime() override;

  /**
   * Forward those methods to the underneath filters
   */
  int ProcessRequest(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

  /**
   * Forward those methods to the underneath filters
   */
  virtual int ProcessRequest(
    vtkInformation* request, vtkCollection* inInfo, vtkInformationVector* outInfo);

protected:
  vtkPVDataSetAlgorithmSelectorFilter();
  ~vtkPVDataSetAlgorithmSelectorFilter() override;

  virtual int RequestDataObject(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector);
  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  vtkGetMacro(OutputType, int);
  vtkSetMacro(OutputType, int);
  int OutputType;

  // Callback registered with the InternalProgressObserver.
  static void InternalProgressCallbackFunction(vtkObject*, unsigned long, void* clientdata, void*);
  void InternalProgressCallback(vtkAlgorithm* algorithm);
  // The observer to report progress from the internal filters.
  vtkCallbackCommand* InternalProgressObserver;

private:
  vtkPVDataSetAlgorithmSelectorFilter(const vtkPVDataSetAlgorithmSelectorFilter&) = delete;
  void operator=(const vtkPVDataSetAlgorithmSelectorFilter&) = delete;

  class vtkInternals;
  vtkInternals* Internal;
};

#endif
