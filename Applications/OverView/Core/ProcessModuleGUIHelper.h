/*=========================================================================

   Program: ParaView
   Module:    ProcessModuleGUIHelper.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#ifndef __pqClientProcessModudeGUIHelper_h
#define __pqClientProcessModudeGUIHelper_h

#include "OverViewCoreExport.h"

#include "pqProcessModuleGUIHelper.h"
#include <QPointer>
#include <QSplashScreen>

/*!
 * ProcessModuleGUIHelper extends pqProcessModuleGUIHelper
 * so that we can create the type of MainWindow needed for pqClient.
 *
 */
class OVERVIEW_CORE_EXPORT ProcessModuleGUIHelper : public pqProcessModuleGUIHelper
{
public:
  static ProcessModuleGUIHelper* New();
  vtkTypeMacro(ProcessModuleGUIHelper, pqProcessModuleGUIHelper);
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetWindowType(const QString& window_type);
  void SetConfiguredPlugins(const QStringList& configured_plugins);

  QWidget* GetUserInterface();

  /// Compares the contents of the window with the given reference image, returns true iff they "match" within some tolerance
  virtual bool compareView(const QString& ReferenceImage, double Threshold, ostream& Output, const QString& TempDirectory);

protected:
  ProcessModuleGUIHelper();
  ~ProcessModuleGUIHelper();

  /// subclasses can override this method to create their own
  /// subclass of pqMainWindow as the Main Window.
  virtual QWidget* CreateMainWindow();

  QPointer<QSplashScreen> Splash;
  QString WindowType;
  QPointer<QWidget> UserInterface;
  QStringList ConfiguredPlugins;

private:
  ProcessModuleGUIHelper(const ProcessModuleGUIHelper&); // Not implemented.
  void operator=(const ProcessModuleGUIHelper&); // Not implemented.
};

#endif


