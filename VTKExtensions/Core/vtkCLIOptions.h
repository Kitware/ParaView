/*=========================================================================

  Program:   ParaView
  Module:    vtkCLIOptions.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkCLIOptions
 * @brief command line options manager
 *
 * vtkCLIOptions handles command line arguments processing in ParaView. To use,
 * create a `vtkCLIOptions` instance, populate it with available arguments and
 * then call `Parse`. Various modules in ParaView offer ways of populating
 * vtkCLIOptions instance with supported command line arguments. For example:
 *
 * @code
 *
 * vtkNew<vtkCLIOptions> options;
 * vtkProcessModuleConfiguration::GetInstance()->PopulateOptions(options);
 * vtkRemotingCoreConfiguration::GetInstance()->PopulateOptions(options);
 *
 * @endcode
 *
 * Applications can add custom configuration classes that expose new command
 * line options and configuration parameters.
 *
 * ParaView uses CLI11 for the processing on command line arguments. The CLI11
 * App can be accessed using `GetCLI11App`. vtkCLIOptions automatically creates
 * a `CLI::App` on instantiation. One can also pass it an existing app using
 * `vtkCLIOptions::New(app)`.
 */

#ifndef vtkCLIOptions_h
#define vtkCLIOptions_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsCoreModule.h" // needed for export macro

#include <memory>              // for std::unique_ptr
#include <string>              // for std::string
#include <vector>              // for std::vector
#include <vtk_cli11_forward.h> // for CLI::App

// forward declare CLI::App
namespace CLI
{
class App;
}

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkCLIOptions : public vtkObject
{
public:
  static vtkCLIOptions* New();
  vtkTypeMacro(vtkCLIOptions, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Use this overload of `New` to use an existing CLI::App instance. The app
   * will not be initialized i.e. the formatter, `AllowExtras` and
   * `StopOnUnrecognizedArgument` state etc. will be left unchanged.
   */
  static vtkCLIOptions* New(const std::shared_ptr<CLI::App>& app);

  /**
   * Returns the `CLI::App` reference that can be used to populate command line
   * options.
   */
  CLI::App* GetCLI11App() const;

  /**
   * Returns the help string.
   */
  const char* GetHelp() const;

  ///@{
  /**
   * Get/Set the application name to use. If none specified, one is set using the
   * arguments passed to `Parse`.
   */
  void SetName(const char* name);
  const char* GetName() const;
  ///@}

  ///@{
  /**
   * Get/Set the application description to use.
   */
  void SetDescription(const char* desc);
  const char* GetDescription() const;
  ///@}

  ///@{
  /**
   * Indicate whether extra arguments should be allowed/ignored. Unparsed
   * arguments can be obtained using `GetExtraArguments`.
   *
   * Forwards to `CLI::App::allow_extras`.
   *
   * By default, this is set to true.
   */
  void SetAllowExtras(bool val);
  bool GetAllowExtras() const;
  ///@}

  ///@{
  /**
   * Indicate whether the parsing should stop as soon as the first unrecognized
   * option is detected. This implies SetAllowExtras(true).
   *
   * Forwards to `CLI::App::prefix_command`.
   *
   * By default, this is set to true.
   */
  void SetStopOnUnrecognizedArgument(bool val);
  bool GetStopOnUnrecognizedArgument() const;
  ///@}

  ///@{
  /**
   * When set to true (default), warnings messages are generated with
   * incorrectly named short arguments are automatically converted to long
   * arguments.
   * @sa SetConvertInvalidShortNamesToLong
   */
  vtkSetMacro(GenerateWarnings, bool);
  vtkGetMacro(GenerateWarnings, bool);
  vtkBooleanMacro(GenerateWarnings, bool);
  ///@}

  ///@{
  /**
   * When set to true (default), attempts to convert legacy forms of argument specifications
   * in `Parse`. Currently, the following cases are supported:
   *
   * * Invalid short flags/options for the from `-name` are converted to `--name`.
   * * Equals (`=`) used as separator between short option name and its value is
   *   replaced by a space (` `).
   *
   * If `GenerateWarnings` is true, a warning is raised when any option is convert thusly.
   *
   * Note, these conversions are only performed till the first positional
   * separator, `--`, is detected. All arguments after
   * the first positional separator are left unchanged.
   */
  vtkSetMacro(HandleLegacyArgumentFormats, bool);
  vtkGetMacro(HandleLegacyArgumentFormats, bool);
  vtkBooleanMacro(HandleLegacyArgumentFormats, bool);
  ///@}

  /**
   * Parse command line arguments. Returns true on success else false.
   *
   * If HandleLegacyArgumentFormats is true, `-option` are converted to
   * `--option` until the first positional separator is encountered. Likewise,
   * `-l=foo` is split as `-l foo`.
   *
   * If GenerateWarnings is true, a warning message is generated for each such
   * converted option.
   */
  bool Parse(int& argc, char**& argv);

  ///@{
  /**
   * Returns unparsed / extra arguments left over from the most reset `Parse`
   * call.
   */
  const std::vector<std::string> GetExtraArguments() const { return this->ExtraArguments; }
  ///@}

  /**
   * Returns error message from the most recent call to `Parse`, if any.
   */
  const std::string& GetLastErrorMessage() const { return this->LastErrorMessage; }

  /**
   * Returns true of help was requested in the most recent call to `Parse`.
   */
  vtkGetMacro(HelpRequested, bool);

  /**
   * Returns usage string.
   */
  std::string GetUsage() const;

protected:
  vtkCLIOptions();
  ~vtkCLIOptions();

private:
  vtkCLIOptions(const vtkCLIOptions&) = delete;
  void operator=(const vtkCLIOptions&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
  bool HandleLegacyArgumentFormats = true;
  bool GenerateWarnings = true;
  std::vector<std::string> ExtraArguments;
  std::string LastErrorMessage;
  bool HelpRequested = false;
  mutable std::string Help;
  mutable std::string Description;
};

#endif
