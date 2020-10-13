/*=========================================================================

   Program: ParaView
   Module:  pqUsageLoggingBehavior.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#ifndef pqUsageLoggingBehavior_h
#define pqUsageLoggingBehavior_h

#include "pqApplicationComponentsModule.h" // for exports
#include <QObject>

class QJsonObject;

/**
* @class    pqUsageLoggingBehavior
* @ingroup  Behaviors
* @brief    behavior to log usage on application startup.
*
* pqUsageLoggingBehavior is used to log usage of the application. On
* instantiation, this looks for a file named `usage_logger.json` at standard
* locations. If found, the file can be used to specify options that result in
* making an HTTP POST request to a web-service with parameters providing
* information about the application usage.
*
* @section ConfigurationFile Configuration File Specification
*
* The configuration file is a simple JSON as follows:
*
* @code{js}
* {
*   "version": 1.0,
*   "url": "http://....",
*   "params": {
*     "user": "$username$",
*     "platform": "$platform$",
*     "date": "$date$",
*     "product": "$appname$",
*     "version": "$appversion$"
*   }
* }
* @endcode
*
* `version`, and `url` are required. Optional `params` may be used to specify a
* collection of key-value parameters to post when making a request to the
* web-service. `$...$` values are replaced at runtime for supported keywords as
* follows:
*
* * `$username$`   : username
* * `$platform$`   : OS information (see `QSysInfo::prettyProductName()`)
* * `$date$`       : current date as 'MM/dd/yy'
* * `$appname$`    : application name (see `QApplication::applicationName()`)
* * `$appversion$` : appversion version (same as `PARAVIEW_VERSION_FULL`)
*
* @section StandardLocations Standard Locations
*
* On Windows, the config file may be placed in the same directory as the
* executable and ParaView dlls.
*
* On MacOS, the config file may be placed in the same directory as the ParaView
* `.dylib` files or in the `Resources` directory in the app bundle.
*
* On Linux, the config file may be placed in the same directory as the ParaView
* `.so` files or the executable.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqUsageLoggingBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqUsageLoggingBehavior(QObject* parent = 0);
  ~pqUsageLoggingBehavior() override;

  //@{
  /**
   * Get/set the name of the configuration file. This must be set before
   * instantiating pqUsageLoggingBehavior. Default is `usage_logger.json`.
   */
  static void setConfigFileName(const QString& fname);
  static const QString& configFileName() { return pqUsageLoggingBehavior::ConfigFileName; }
  //@}
private:
  Q_DISABLE_COPY(pqUsageLoggingBehavior);

  /**
   * Returns the configuration file.
   */
  QString configurationFile() const;

  void logUsage(const QJsonObject& config);
  static QString substitute(const QString&);
  static QString ConfigFileName;
};

#endif
