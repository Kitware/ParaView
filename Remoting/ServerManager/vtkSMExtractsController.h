/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExtractsController.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMExtractsController
 * @brief controller for extract generation
 *
 * vtkSMExtractsController is a controllers that provides API to handle various
 * aspects of extractors and extract generation mechanisms supported by
 * ParaView.
 *
 * It provides API to query, create extractors of known types. It also
 * provides API to generate extracts using the defined extractors.
 *
 * @section GeneratingExtractsSummary Summary of generated extracts
 *
 * vtkSMExtractsController generates a summary table for all
 * extracts generated. Each row in this summary table corresponds to an extract
 * generated. Each column provides information about that extract. Filename for
 * the extract is stored in a column named `FILE_[type]` where `[type]` is
 * replaced by the extension of the file. Other columns are used to stored
 * named values that characterize the extract.
 *
 * Currently, this summary table is used to generated a Cinema specification
 * which can be used to explore the generated extracts using Cinema tools
 * (https://cinemascience.github.io/).
 */

#ifndef vtkSMExtractsController_h
#define vtkSMExtractsController_h

#include "vtkObject.h"
#include "vtkRemotingServerManagerModule.h" // for exports
#include "vtkSmartPointer.h"                // for vtkSmartPointer

#include <map>    // for std::map
#include <string> // for std::string
#include <vector> // for std::vector

class vtkCollection;
class vtkSMExtractWriterProxy;
class vtkSMProxy;
class vtkSMSessionProxyManager;
class vtkTable;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMExtractsController : public vtkObject
{
public:
  static vtkSMExtractsController* New();
  vtkTypeMacro(vtkSMExtractsController, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Information about current time/timestep.
   * This must be set correctly before using `Extract` to generate extracts.
   */
  vtkSetClampMacro(TimeStep, int, 0, VTK_INT_MAX);
  vtkGetMacro(TimeStep, int);
  vtkSetMacro(Time, double);
  vtkGetMacro(Time, double);
  //@}

  //@{
  /**
   * Get/Set the root directory to use for writing extracts.
   * This must be set correctly before using `Extract` to generate extracts.
   *
   * @note
   * This can be overridden at runtime using environment variable
   * `PARAVIEW_OVERRIDE_EXTRACTS_OUTPUT_DIRECTORY`. If set, the value specified
   * here ignored and the environment variable is used instead.
   */
  vtkSetStringMacro(ExtractsOutputDirectory);
  vtkGetStringMacro(ExtractsOutputDirectory);
  //@}

  /**
   * Returns the extract output directory to use. If
   * `PARAVIEW_OVERRIDE_EXTRACTS_OUTPUT_DIRECTORY` is not set, this will be same
   * as `GetExtractsOutputDirectory` else this will be the value of the
   * `PARAVIEW_OVERRIDE_EXTRACTS_OUTPUT_DIRECTORY` environment variable.
   */
  const char* GetRealExtractsOutputDirectory() const;

  /**
   * Generate the extract for the current state. Returns true if extract was
   * generated, false if skipped or failed.
   *
   * This overload generates extract from the specific extractor.
   */
  bool Extract(vtkSMProxy* extractor);

  /**
   * Generate the extract for the current state. Returns true if extract was
   * generated, false if skipped or failed.
   *
   * This overload generates extracts from all extractors registered
   * with the proxy-manager.
   */
  bool Extract(vtkSMSessionProxyManager* pxm);

  /**
   * Generate the extract for the current state. Returns true if extract was
   * generated, false if skipped or failed.
   *
   * This overload locates the active session and then simply calls
   * Extract(vtkSMSessionProxyManager*).
   */
  bool Extract();

  /**
   * Generate extract s for the current state using only the extractors
   * in the collection.
   */
  bool Extract(vtkCollection* collection);

  //@{
  /**
   * Check if any of the extractors registered with the chosen
   * proxy-manager (or active proxy-manager, is none specified) has their
   * trigger activated given the current state of the application and the values
   * for Time and TimeStep set on the controller.
   */
  bool IsAnyTriggerActivated(vtkSMSessionProxyManager* pxm);
  bool IsAnyTriggerActivated();
  bool IsAnyTriggerActivated(vtkCollection* collection);
  //@}

  /**
   * Same as `IsAnyTriggerActivated` except only check the selected extractor's
   * trigger definition.
   */
  bool IsTriggerActivated(vtkSMProxy* extractor);

  /**
   * Returns a list of extractors associated with the proxy.
   */
  std::vector<vtkSMProxy*> FindExtractors(vtkSMProxy* proxy) const;

  /**
   * Returns a list of prototype proxies for extractors that can be
   * attached to the given `proxy`.
   */
  std::vector<vtkSMProxy*> GetSupportedExtractorPrototypes(vtkSMProxy* proxy) const;

  /**
   * Returns true is the given extractor (or prototype of the same) can
   * extract the selected set of proxies at the same time.
   */
  bool CanExtract(vtkSMProxy* extractor, const std::vector<vtkSMProxy*>& inputs) const;
  bool CanExtract(vtkSMProxy* extractor, vtkSMProxy* input) const
  {
    return this->CanExtract(extractor, std::vector<vtkSMProxy*>{ input });
  }

  //@{
  /**
   * Creates, initializes and registers a new extractor of the chosen type.
   */
  vtkSMProxy* CreateExtractor(
    vtkSMProxy* proxy, const char* xmlname, const char* registrationName = nullptr) const;
  //@}

  /**
   * Returns true is the `extractor` is an extractor for the `proxy`.
   */
  static bool IsExtractor(vtkSMProxy* extractor, vtkSMProxy* proxy);

  /**
   * Given an extractor proxy, returns the producer for the extractor,
   * if any. May return a vtkSMViewProxy or a vtkSMOutputPort.
   */
  vtkSMProxy* GetInputForExtractor(vtkSMProxy* extractor) const;

  /**
   * Get access to the summary table generated so far. This will be nullptr
   * until the first extract is generated.
   *
   * See @ref GeneratingExtractsSummary for information about summary table.
   */
  vtkTable* GetSummaryTable() const;

  /**
   * Reset summary table.
   *
   * Generally not needed since there is not much use for reusing
   * vtkSMExtractsController, one should just create a new one when needed.
   */
  void ResetSummaryTable();

  /**
   * Saves summary table to a file. Path is relative to the
   * ExtractsOutputDirectory.
   */
  bool SaveSummaryTable(const std::string& fname, vtkSMSessionProxyManager* pxm);

  //@{
  /**
   * Called by vtkSMExtractWriterProxy subclasses to add an entry to the summary table.
   * Note, this must be called for every extract written out by the extract
   * writer.
   */
  using SummaryParametersT = std::map<std::string, std::string>;
  bool AddSummaryEntry(vtkSMExtractWriterProxy* writer, const std::string& filename,
    const SummaryParametersT& params = SummaryParametersT{});
  //@}

  /**
   * Returns true of the extractor is enabled.
   */
  static bool IsExtractorEnabled(vtkSMProxy* extractor);

  /**
   * Enable/disable an extractor.
   */
  static void SetExtractorEnabled(vtkSMProxy* extractor, bool val);

protected:
  vtkSMExtractsController();
  ~vtkSMExtractsController();

  //@{
  /**
   * These methods are intended to be use by vtkSMExtractWriterProxy and
   * subclasses to ensure chosen output directories are created before
   * attempting to generate extracts.
   *
   * Return true on success, or false to failed to create writeable directories.
   * Extract writers should not attempt to write any extracts when that happens.
   */
  bool CreateExtractsOutputDirectory(vtkSMSessionProxyManager* pxm) const;
  bool CreateDirectory(const std::string& dname, vtkSMSessionProxyManager* pxm) const;
  //@}

  /**
   * Returns a friendly name derived from the extract writer.
   */
  std::string GetName(vtkSMExtractWriterProxy* writer);

private:
  vtkSMExtractsController(const vtkSMExtractsController&) = delete;
  void operator=(const vtkSMExtractsController&) = delete;

  /**
   * Returns the name of the column used in the summary table to save extract
   * filenames. This is currently based on the file's extension.
   */
  static std::string GetSummaryTableFilenameColumnName(const std::string& fname);

  int TimeStep;
  double Time;
  char* ExtractsOutputDirectory;
  char* EnvironmentExtractsOutputDirectory;
  vtkSmartPointer<vtkTable> SummaryTable;
  mutable std::string LastExtractsOutputDirectory;
  mutable bool ExtractsOutputDirectoryValid;

  vtkSetStringMacro(EnvironmentExtractsOutputDirectory);
};

#endif
