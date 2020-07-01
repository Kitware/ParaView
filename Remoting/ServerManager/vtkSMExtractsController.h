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
 * aspects of extract generators and extract generation mechanisms supported by
 * ParaView.
 *
 * It provides API to query, create extract generators of known types. It also
 * provides API to generate extracts using the defined extract generators.
 *
 */

#ifndef vtkSMExtractsController_h
#define vtkSMExtractsController_h

#include "vtkObject.h"
#include "vtkRemotingServerManagerModule.h" // for exports

#include <string> // for std::string
#include <vector> // for std::vector

class vtkCollection;
class vtkSMProxy;
class vtkSMSessionProxyManager;

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
   * Get/Set the root directory to use for writing data extracts.
   * This must be set correctly before using `Extract` to generate extracts.
   */
  vtkSetStringMacro(DataExtractsOutputDirectory);
  vtkGetStringMacro(DataExtractsOutputDirectory);
  //@}

  //@{
  /**
   * Get/Set the root directory to use for writing image extracts.
   * This must be set correctly before using `Extract` to generate extracts.
   */
  vtkSetStringMacro(ImageExtractsOutputDirectory);
  vtkGetStringMacro(ImageExtractsOutputDirectory);
  //@}

  /**
   * Generate the extract for the current state. Returns true if extract was
   * generated, false if skipped or failed.
   *
   * This overload generates extract from the specific extract-generator.
   */
  bool Extract(vtkSMProxy* extractor);

  /**
   * Generate the extract for the current state. Returns true if extract was
   * generated, false if skipped or failed.
   *
   * This overload generates extracts from all extract-generators registered
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
   * Generate extract s for the current state using only the extract generators
   * in the collection.
   */
  bool Extract(vtkCollection* collection);

  //@{
  /**
   * Check if any of the extract generators registered with the chosen
   * proxy-manager (or active proxy-manager, is none specified) has their
   * trigger activated given the current state of the application and the values
   * for Time and TimeStep set on the controller.
   */
  bool IsAnyTriggerActivated(vtkSMSessionProxyManager* pxm);
  bool IsAnyTriggerActivated();
  bool IsAnyTriggerActivated(vtkCollection* collection);
  //@}

  /**
   * Same as `IsAnyTriggerActivated` except only check the selected extract
   * generator's trigger definition.
   */
  bool IsTriggerActivated(vtkSMProxy* generator);

  /**
   * Returns a list of extract generators associated with the proxy.
   */
  std::vector<vtkSMProxy*> FindExtractGenerators(vtkSMProxy* proxy) const;

  /**
   * Returns a list of prototype proxies for extract generators that can be
   * attached to the given `proxy`.
   */
  std::vector<vtkSMProxy*> GetSupportedExtractGeneratorPrototypes(vtkSMProxy* proxy) const;

  /**
   * Returns true is the given extract generator (or prototype of the same) can
   * extract the selected set of proxies at the same time.
   */
  bool CanExtract(vtkSMProxy* generator, const std::vector<vtkSMProxy*>& inputs) const;
  bool CanExtract(vtkSMProxy* generator, vtkSMProxy* input) const
  {
    return this->CanExtract(generator, std::vector<vtkSMProxy*>{ input });
  }

  //@{
  /**
   * Creates, initializes and registers a new extract generator of the chosen type.
   */
  vtkSMProxy* CreateExtractGenerator(
    vtkSMProxy* proxy, const char* xmlname, const char* registrationName = nullptr) const;
  //@}

  /**
   * Returns true is the `generator` is an extract generator for the `proxy`.
   */
  static bool IsExtractGenerator(vtkSMProxy* generator, vtkSMProxy* proxy);

  /**
   * Given an extract generator proxy, returns the producer for the generator,
   * if any. May return a vtkSMViewProxy or a vtkSMOutputPort.
   */
  vtkSMProxy* GetInputForGenerator(vtkSMProxy* generator) const;

  //@{
  /**
   * These methods are intended to be use by vtkSMExtractWriterProxy and
   * subclasses to ensure chosen output directories are created before
   * attempting to generate extracts.
   *
   * Return true on success, or false to failed to create writeable directories.
   * Extract writers should not attempt to write any extracts when that happens.
   */
  bool CreateImageExtractsOutputDirectory() const;
  bool CreateDataExtractsOutputDirectory() const;
  bool CreateDirectory(const std::string& dname) const;
  //@}
protected:
  vtkSMExtractsController();
  ~vtkSMExtractsController();

private:
  vtkSMExtractsController(const vtkSMExtractsController&) = delete;
  void operator=(const vtkSMExtractsController&) = delete;

  int TimeStep;
  double Time;
  char* DataExtractsOutputDirectory;
  char* ImageExtractsOutputDirectory;
};

#endif
