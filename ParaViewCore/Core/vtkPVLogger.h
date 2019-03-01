/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLogger.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVLogger
 * @brief defines various logging categories for ParaView
 *
 * ParaView code generates informative log entires under several categories.
 * This class provides ability to convert a category to a verbosity level
 * to use to log a message.
 *
 * All code in ParaView that generates informative log messages should use one
 * of the categories defined here when logging. For example, to log a message
 * about rendering, one uses `GetRenderingVerbosity` or the convenience macro
 * `PARAVIEW_LOG_RENDERING_VERBOSITY()` as follows:
 *
 * @code{cpp}
 * vtkVLogF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "message about rendering");
 * @endcode
 *
 * Developers can elevate verbosity level for any category using the APIs
 * provided on vtkPVLogger e.g. SetRenderingVerbosity.
 *
 * At runtime, users can elevate verbosity level for any category by setting the
 * corresponding environment variable to level requested e.g. to make all
 * rendering log message show up as INFO, and thus show up on the terminal by
 * default, set the environment variable `PARAVIEW_LOG_RENDERING_VERBOSITY` to
 * `INFO` or `0`.
 *
 * When not changed using the APIs or environment variables, all categories
 * default to vtkLogger::VERBOSITY_TRACE. To change the default used, use
 * `vtkPVLogger::SetDefaultVerbosity`.
 */

#ifndef vtkPVLogger_h
#define vtkPVLogger_h

#include "vtkLogger.h"
#include "vtkPVCoreModule.h" // needed for export macro

class VTKPVCORE_EXPORT vtkPVLogger : public vtkLogger
{
public:
  vtkTypeMacro(vtkPVLogger, vtkLogger);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Use this verbosity level when logging messages that provide information
   * about pipeline execution.
   *
   * Default level is `vtkLogger::VERBOSITY_TRACE` unless overridden by calling
   * `SetPipelineVerbosity` or by setting the environment variable
   * `PARAVIEW_LOG_PIPELINE_VERBOSITY` to the expected verbosity level.
   */
  static Verbosity GetPipelineVerbosity();
  static void SetPipelineVerbosity(Verbosity value);
  //@}

  //@{
  /**
   * Use this verbosity level to for log message relating to ParaView's plugin
   * system.
   *
   * Default level is `vtkLogger::VERBOSITY_TRACE` unless overridden by calling
   * `SetPluginVerbosity` or by setting the environment variable
   * `PARAVIEW_LOG_PLUGIN_VERBOSITY` to the expected verbosity level.
   */
  static Verbosity GetPluginVerbosity();
  static void SetPluginVerbosity(Verbosity value);
  //@}

  //@{
  /**
   * Verbosity level for log messages related to data-movement e.g. moving data
   * between processes for rendering.
   *
   * Default level is `vtkLogger::VERBOSITY_TRACE` unless overridden by calling
   * `SetDataMovementVerbosity` or by setting the environment variable
   * `PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY` to the expected verbosity level.
   */
  static Verbosity GetDataMovementVerbosity();
  static void SetDataMovementVerbosity(Verbosity value);
  //@}

  //@{
  /**
   * Verbosity level for log messages related to rendering.
   *
   * Default level is `vtkLogger::VERBOSITY_TRACE` unless overridden by calling
   * `SetRenderingVerbosity` or by setting the environment variable
   * `PARAVIEW_LOG_RENDERING_VERBOSITY` to the expected verbosity level.
   */
  static Verbosity GetRenderingVerbosity();
  static void SetRenderingVerbosity(Verbosity value);
  //@}

  //@{
  /**
   * Verbosity level for log messages related to the application, gui, and
   * similar components.
   *
   * Default level is `vtkLogger::VERBOSITY_TRACE` unless overridden by calling
   * `SetRenderingVerbosity` or by setting the environment variable
   * `PARAVIEW_LOG_APPLICATION_VERBOSITY` to the expected verbosity level.
   */
  static Verbosity GetApplicationVerbosity();
  static void SetApplicationVerbosity(Verbosity value);
  //@}

  //@{
  /**
   * Change default verbosity to use for all ParaView categories defined here if
   * no overrides are specified. This is intended to be used by ParaView-based
   * applications to change the level at which ParaView messages are logged in
   * bulk.
   *
   * Default level is `vtkLogger::VERBOSITY_TRACE`.
   */
  static Verbosity GetDefaultVerbosity();
  static void SetDefaultVerbosity(Verbosity value);
  //@}
protected:
  vtkPVLogger();
  ~vtkPVLogger() override;

private:
  vtkPVLogger(const vtkPVLogger&) = delete;
  void operator=(const vtkPVLogger&) = delete;
};

/**
 * Macro to use for verbosity when logging pipeline messages. Same as calling
 * vtkPVLogger::GetPipelineVerbosity() e.g.
 *
 * @code{cpp}
 *  vtkVLogF(PARAVIEW_LOG_PIPELINE_VERBOSITY(), "pipeline updated");
 * @endcode
 */
#define PARAVIEW_LOG_PIPELINE_VERBOSITY() vtkPVLogger::GetPipelineVerbosity()

/**
 * Macro to use for verbosity when logging plugin messages. Same as calling
 * vtkPVLogger::GetPluginVerbosity() e.g.
 *
 * @code{cpp}
 *  vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(), "plugin loaded `%s`", name);
 * @endcode
 */
#define PARAVIEW_LOG_PLUGIN_VERBOSITY() vtkPVLogger::GetPluginVerbosity()

/**
 * Macro to use for verbosity when logging data-movement messages. Same as calling
 * vtkPVLogger::GetDataMovementVerbosity() e.g.
 *
 * @code{cpp}
 *  vtkVLogF(PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY(), "moving data");
 * @endcode
 */
#define PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY() vtkPVLogger::GetDataMovementVerbosity()

/**
 * Macro to use for verbosity when logging rendering messages. Same as calling
 * vtkPVLogger::GetRenderingVerbosity() e.g.
 *
 * @code{cpp}
 *  vtkVLogF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "still render");
 * @endcode
 */
#define PARAVIEW_LOG_RENDERING_VERBOSITY() vtkPVLogger::GetRenderingVerbosity()

/**
 * Macro to use for verbosity when logging application messages. Same as calling
 * vtkPVLogger::GetApplicationVerbosity() e.g.
 *
 * @code{cpp}
 *  vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "reading config file %s", filename);
 * @endcode
 */
#define PARAVIEW_LOG_APPLICATION_VERBOSITY() vtkPVLogger::GetApplicationVerbosity()

#endif
