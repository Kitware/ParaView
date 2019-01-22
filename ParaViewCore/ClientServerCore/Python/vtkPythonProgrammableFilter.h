/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonProgrammableFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPythonProgrammableFilter
 * @brief   Executes a user supplied python script
 * on its input dataset to produce an output dataset.
 *
 * This filter will execute a python script to produce an output dataset.
 * An new interpretor is created at the beginning of RequestInformation().
 * The state of the python interpretor is preserved until the
 * next execution of RequestInformation().
 * After the interpretor is creates the vtk module is imported with
 * "from paraview import vtk".
 *
 * Then the interpretor runs the InformationScript during RequestInformation().
 * This script is run in a python function called RequestInformation().
 * An argument named self that refers to the programmable filter is passed
 * to the function.
 * The interpretor also runs the Script during RequestData().
 * This script is run in a python function called RequestData().
 * An argument named self that refers to the programmable filter is passed
 * to the function.
 * Furthermore, a set of parameters passed with the SetParameter()
 * call are defined as Python variables inside both scripts. This allows
 * the developer to keep the scripts the same but change their behaviour
 * using parameters.
*/

#ifndef vtkPythonProgrammableFilter_h
#define vtkPythonProgrammableFilter_h

#include "vtkPVClientServerCorePythonModule.h" //needed for exports
#include "vtkProgrammableFilter.h"

class vtkPythonProgrammableFilterImplementation;

class VTKPVCLIENTSERVERCOREPYTHON_EXPORT vtkPythonProgrammableFilter : public vtkProgrammableFilter
{
public:
  vtkTypeMacro(vtkPythonProgrammableFilter, vtkProgrammableFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkPythonProgrammableFilter* New();

  //@{
  /**
   * Set the text of the python script to execute.
   */
  vtkSetStringMacro(Script);
  vtkGetStringMacro(Script);
  //@}

  //@{
  /**
   * Set the text of the python script to execute in RequestInformation().
   */
  vtkSetStringMacro(InformationScript);
  vtkGetStringMacro(InformationScript);
  //@}

  //@{
  /**
   * Set the text of the python script to execute in RequestUpdateExtent().
   */
  vtkSetStringMacro(UpdateExtentScript);
  vtkGetStringMacro(UpdateExtentScript);
  //@}

  //@{
  /**
   * For "live" sources, this script, if provided, if used to determine
   * if the source needs an update.
   */
  vtkSetStringMacro(CheckNeedsUpdateScript);
  vtkGetStringMacro(CheckNeedsUpdateScript);
  //@}

  //@{
  /**
   * Set a name-value parameter that will be available to the script
   * when it is run
   */
  void SetParameterInternal(const char* name, const char* value);
  void SetParameter(const char* name, const char* value);
  void SetParameter(const char* name, int value);
  void SetParameter(const char* name, double value);
  void SetParameter(const char* name, double value1, double value2);
  void SetParameter(const char* name, double value1, double value2, double value3);
  //@}

  //@{
  /**
   * To support repeatable-parameters.
   */
  void AddParameter(const char* name, const char* value);
  void ClearParameter(const char* name);
  //@}

  /**
   * Clear all name-value parameters
   */
  void ClearParameters();

  /**
   * For internal use only.
   */
  static void ExecuteScript(void*);

  //@{
  /**
   * Changes the output data set type.
   * Allowable values are defined in vtkType.h
   */
  vtkSetMacro(OutputDataSetType, int);
  vtkGetMacro(OutputDataSetType, int);
  //@}

  //@{
  /**
   * A semi-colon (;) separated list of directories to add to the python library
   * search path.
   */
  vtkSetStringMacro(PythonPath);
  vtkGetStringMacro(PythonPath);
  //@}

  /**
   * Set the number of input ports
   * This function is explicitly exposed to enable a vtkClientServerInterpreter to call it
   */
  void SetNumberOfInputPorts(int numberOfInputPorts) override
  {
    this->Superclass::SetNumberOfInputPorts(numberOfInputPorts);
  }

  /**
   * Application code can call `GetNeedsUpdate` to check if the algorithm can use
   * an update to show updated information.
   */
  bool GetNeedsUpdate();

  /**
   * CheckNeedsUpdateScript should call `SetNeedsUpdate(true)` to indicate that
   * the algorithm has new data and hence may be updated.
   */
  vtkSetMacro(NeedsUpdate, bool);

protected:
  vtkPythonProgrammableFilter();
  ~vtkPythonProgrammableFilter() override;

  /**
   * For internal use only.
   */
  void Exec(const char*, const char*);

  int FillOutputPortInformation(int port, vtkInformation* info) override;

  // overridden to allow multiple inputs to port 0
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Creates whatever output data set type is selected.
   */
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * We want to temporarilly cache request to be used in the Python
   * code so we override this method to store request for later use
   * since otherwise we won't have access to it.
   */
  int ProcessRequest(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

  char* Script;
  char* InformationScript;
  char* UpdateExtentScript;
  char* CheckNeedsUpdateScript;
  char* PythonPath;
  int OutputDataSetType;
  bool NeedsUpdate;

private:
  vtkPythonProgrammableFilter(const vtkPythonProgrammableFilter&) = delete;
  void operator=(const vtkPythonProgrammableFilter&) = delete;

  /**
   * When there is a request, cache it so that we can use it inside the Python
   * source code of the filter. It is set at the beginning of ProcessRequest
   * and removed at the end of that method.
   */
  vtkInformation* Request;

  vtkPythonProgrammableFilterImplementation* const Implementation;
};

#endif
