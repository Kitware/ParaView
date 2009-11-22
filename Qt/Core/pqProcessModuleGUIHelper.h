/*=========================================================================

   Program: ParaView
   Module:    pqProcessModuleGUIHelper.h

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
#ifndef __pqProcessModuleGUIHelper_h
#define __pqProcessModuleGUIHelper_h

#include "vtkProcessModuleGUIHelper.h"
#include "pqCoreExport.h" // needed for PQCORE_EXPORT macro.

class pqApplicationCore;
class pqTestUtility;
class QApplication;
class QString;
class QWidget;
class vtkSMApplication;

/*! \brief This is the GUI helper for ParaView.
 * This class provides GUI elements to the process module without forcing
 * the process modules to link with the GUI. This class creates the main window
 * when the ProcessModule requests the event loop to begin.
 * \todo When the GUI Helper receives Progress, it must be conveyed over to the
 * MainWindow so that the progress can be shown.
 * @deprecated vtkProcessModuleGUIHelper and subclasses will soon be removed.
 * Switch to using new style application initialization.
 */
class PQCORE_EXPORT pqProcessModuleGUIHelper : public vtkProcessModuleGUIHelper
{
public:
  vtkTypeRevisionMacro(pqProcessModuleGUIHelper, vtkProcessModuleGUIHelper);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Get the main window for the application.
  QWidget* GetMainWindow();

  /// Start the GUI event loop.
  virtual int RunGUIStart(int argc, char** argv, int numServerProcs, int myId);

  /// Open a connection dialog GUI. Called when default Connection to
  /// server fails.
  virtual int OpenConnectionDialog(int *) { return 0; }

  /// Handle progress events.
  virtual void SendPrepareProgress();
  /// Handle progress events.
  virtual void SendCleanupPendingProgress();
  /// Handle progress events.
  virtual void SetLocalProgress(const char*, int);

  /// Exit the application
  virtual void ExitApplication();

  /// Compares the contents of the window with the given reference image,
  /// returns true iff they "match" within some tolerance
  virtual  bool compareView(const QString& ReferenceImage, double Threshold,
    ostream& Output, const QString& TempDirectory);


  // Description:
  // Get the application singleton.
  QApplication* GetApplication();

  // return the test utility
  virtual pqTestUtility* TestUtility();

  /// Dangerous option that disables the debug output window, intended for
  /// demo purposes only
  void disableOutputWindow();

  /// Show the output window.
  virtual void showOutputWindow();

  /// show the main window
  virtual void showWindow();

  /// hide the main window
  virtual void hideWindow();

protected:
  /// InitializeSMApplication initializes the vtkSMApplication.
  virtual void InitializeSMApplication();

  /// InitializeApplication initializes the QApplication and
  /// the MainWindow.
  virtual int InitializeApplication(int argc, char** argv);

  ///
  /// The three methods constitute the RunGUIStart operations:
  ///   pre-app, app, and post-app
  /// They are provided as a convenience for users who want to be able to
  /// re-implement pqProcessModuleGUIHelper, for example to create a
  /// custom paraview client, supply their own application and event handling,
  /// but re-use the default process module startup code.
  ///
  /// preAppExec does everything up to appExec(), for example initializes the
  /// vtkSMApplication, shows the window, etc.
  /// (see pqProcessModuleGUIHelper.cxx)
  virtual int preAppExec(int argc, char** argv, int numServerProcs, int myId);

  /// appExec executes the QApplication::exec()
  virtual int appExec();

  /// postAppExec does everything after appExec(), for example cleans up the
  /// main window (see pqProcessModuleGUIHelper.cxx)
  virtual int postAppExec();

  /// Cleans up the main window.
  virtual void FinalizeApplication();

  /// Returns the number of errors registered in the OutputWindow
  virtual int ErrorCount();

  VTK_LEGACY(pqProcessModuleGUIHelper());
  ~pqProcessModuleGUIHelper();

private:
  /// subclasses can override this method to create their own
  /// subclass of pqMainWindow as the Main Window.
  virtual QWidget* CreateMainWindow() = 0;

  /// Subclasses can override this to create a different subclass of
  /// pqApplicationCore.
  virtual pqApplicationCore* CreateApplicationCore();

  class pqImplementation;
  pqImplementation* const Implementation;

  pqProcessModuleGUIHelper(pqProcessModuleGUIHelper&); // Not implemented.
  void operator=(const pqProcessModuleGUIHelper&); // Not implemented.
};

#endif
