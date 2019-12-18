
/*=========================================================================

  Program:   ParaView
  Module:    vtkInitializationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkInitializationHelper
 * @brief   help class for python modules
 *
 * This class is used by the python modules when they are loaded from
 * python (as opposed to pvpython). It simply initializes the server
 * manager so that it can be used.
*/

#ifndef vtkInitializationHelper_h
#define vtkInitializationHelper_h

#include "vtkObject.h"
#include "vtkRemotingApplicationModule.h" // needed for exports
#include <string>                         // needed for std::string
class vtkPVOptions;

class VTKREMOTINGAPPLICATION_EXPORT vtkInitializationHelper : public vtkObject
{
public:
  vtkTypeMacro(vtkInitializationHelper, vtkObject);
  void PrintSelf(ostream&, vtkIndent) override;

  /**
   * Initializes the server manager. Do not use the server manager
   * before calling this.
   */
  static void Initialize(const char* executable, int type);

  /**
   * Initializes the server manager. Do not use the server manager
   * before calling this. In this variant, one passes in a vtkPVOptions
   * instance.
   *
   * @note `--no-mpi` and `--mpi` options are handled specially, by this call.
   * If you want to pass those to vtkProcessModule so it doesn't (or does)
   * initialize MPI, set the corresponding ivars on the `options` object passed
   * in.
   */
  static void Initialize(const char* executable, int type, vtkPVOptions* options);

  /**
   * Alternative API to initialize the server manager. This takes in  the
   * command line arguments and the vtkPVOptions instance to use to process the
   * command line options.
   */
  static void Initialize(int argc, char** argv, int type, vtkPVOptions* options);

  /**
   * Finalizes the server manager. Do not use the server manager
   * after calling this.
   */
  static void Finalize();

  //@{
  /**
   * Initialization for standalone executables linking against a PV
   * library. This is needed to insure that linker does not remove object
   * factories' auto init during static linking. It also cleans up after
   * protobuf.
   */
  static void StandaloneInitialize();
  static void StandaloneFinalize();
  //@}

  //@{
  /**
   * During initialization, vtkInitializationHelper reads "settings" files for
   * configuring vtkSMSettings. To disable this processing of the settings file
   * for an application (e.g. in Catalyst), turn this off. On by default.
   */
  static void SetLoadSettingsFilesDuringInitialization(bool);
  static bool GetLoadSettingsFilesDuringInitialization();
  //@}

  //@{
  /**
   * Sets the organization producing this application. This is
   * "ParaView" by default, but can be different for branded applications.
   */
  static void SetOrganizationName(const std::string& organizationName);
  static const std::string& GetOrganizationName();
  //@}

  //@{
  /**
   * Sets the name of the application. This is "ParaView" by default, but
   * can be different for branded applications.
   */
  static void SetApplicationName(const std::string& appName);
  static const std::string& GetApplicationName();
  //@}

  /**
   * Get directory for user settings file. The last character is always the
   * file path separator appropriate for the system.
   */
  static std::string GetUserSettingsDirectory();

  /**
   * Get file path for the user settings file.
   */
  static std::string GetUserSettingsFilePath();

protected:
  vtkInitializationHelper(){};
  ~vtkInitializationHelper() override{};

  /**
   * Load user and site settings
   */
  static void LoadSettings();

private:
  vtkInitializationHelper(const vtkInitializationHelper&) = delete;
  void operator=(const vtkInitializationHelper&) = delete;

  static bool LoadSettingsFilesDuringInitialization;

  static bool SaveUserSettingsFileDuringFinalization;

  static std::string OrganizationName;
  static std::string ApplicationName;
};

#endif
