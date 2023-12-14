// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkCPPythonScriptV2Helper
 * @brief an internal class encapsulating logic for Catalyst Python scripts.
 *
 * This class is an internal helper to share code between `vtkInSituPipelinePython`
 * and `vtkCPPythonScriptV2Pipeline`.
 */

#ifndef vtkCPPythonScriptV2Helper_h
#define vtkCPPythonScriptV2Helper_h

#include "vtkObject.h"
#include "vtkPVPythonCatalystModule.h" // For windows import/export of shared libraries
#include <string>                      // for std::string
#include <vector>                      //for std::string

class vtkSMProxy;
class vtkCPDataDescription;
class vtkStringList;

class VTKPVPYTHONCATALYST_EXPORT vtkCPPythonScriptV2Helper : public vtkObject
{
public:
  static vtkCPPythonScriptV2Helper* New();
  vtkTypeMacro(vtkCPPythonScriptV2Helper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Prepares the Python package/module for import. This doesn't actually
   * import it yet. Returns false if the prep failed for some reason.
   */
  bool PrepareFromScript(const std::string& fname);

  /**
   * Returns the name of the script passed in to PrepareFromScript.
   */
  const std::string& GetScriptFileName() const;

  /**
   * Returns if the package/module has been imported.
   */
  bool IsImported() const;

  /**
   * Imports the module/package. This overload is used by
   * `vtkInSituPipelinePython` which in turn is used in ParaView-Catalyst, the
   * ParaView-based implementation of the Catalyst In Situ API.
   *
   * Returns true on success.
   */
  bool Import(const std::vector<std::string>& args = {});

  /**
   * Calls `catalyst_initialize`.
   */
  bool CatalystInitialize();

  /**
   * Calls `catalyst_finalize`.
   */
  bool CatalystFinalize();

  /**
   * Calls `catalyst_execute`.
   */
  bool CatalystExecute(int timestep, double time, const std::vector<std::string>& params = {});

  /**
   * Calls `catalyst_results`.
   */
  bool CatalystResults();

  ///@{
  /**
   * There are overloads intended for vtkCPPythonScriptV2Pipeline i.e. legacy
   * Catalyst adaptors that don't use Conduit.
   */
  bool Import(vtkCPDataDescription* desc);
  bool CatalystInitialize(vtkCPDataDescription* desc);
  bool RequestDataDescription(vtkCPDataDescription* desc);
  bool CatalystExecute(vtkCPDataDescription* desc);
  ///@}

  ///@{
  /**
   * Internal methods. These are called by Python modules internal to ParaView
   * and may change without notice. Should not be considered as part of ParaView
   * API.
   */
  static vtkCPPythonScriptV2Helper* GetActiveInstance();
  void RegisterExtractor(vtkSMProxy* extractor);
  void RegisterView(vtkSMProxy* view);
  void SetOptions(vtkSMProxy* catalystOptions);
  vtkGetObjectMacro(Options, vtkSMProxy);
  vtkCPDataDescription* GetDataDescription() const { return this->DataDescription; }
  vtkSMProxy* GetTrivialProducer(const char* inputname);
  vtkStringList* GetArgumentsAsStringList() const;
  vtkStringList* GetParametersAsStringList() const;
  ///@}

protected:
  vtkCPPythonScriptV2Helper();
  ~vtkCPPythonScriptV2Helper() override;

  bool IsActivated(int timestep, double time);
  bool IsLiveActivated();
  void DoLive(int, double);

private:
  vtkCPPythonScriptV2Helper(const vtkCPPythonScriptV2Helper&) = delete;
  void operator=(const vtkCPPythonScriptV2Helper&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
  vtkSMProxy* Options;
  std::string Filename;
  vtkCPDataDescription* DataDescription;

  static vtkCPPythonScriptV2Helper* ActiveInstance;
};

#endif
