/*=========================================================================

   Program:   ParaQ
   Module:    pqProcessModuleGUIHelper.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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
#include "pqWidgetsExport.h" // needed for PQWIDGETS_EXPORT macro.

class QWidget;
class vtkSMApplication;
class QApplication;
class QString;
/*! \brief This is the GUI helper for ParaQ.
 * This class provides GUI elements to the process module without forcing
 * the process modules to link with the GUI. This class creates the QApplication
 * and the pqMainWindow when the ProcessModule requests the event loop to begin.
 * If one wants to create any other kind of main window (pqMainWindow subclass)
 * then one must subclass this class and provide the correct instance of the
 * GUI Helper to the process module when intializing it.
 * \todo When the GUI Helper receives Progress, it must be conveyed over to the
 * MainWindow so that the progress can be shown.
 */
class PQWIDGETS_EXPORT pqProcessModuleGUIHelper : public vtkProcessModuleGUIHelper
{
public:
  vtkTypeRevisionMacro(pqProcessModuleGUIHelper, vtkProcessModuleGUIHelper);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Get the main window for the application.
  QWidget* GetMainWindow();

  /// Start the GUI event loop.
  virtual int RunGUIStart(int argc, char** argv, int numServerProcs, int myId);

  /// Open a connection dialog GUI. Called when default Connection to server fails.
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

  /// Dangerous option that disables the debug output window, intended for demo purposes only
  void disableOutputWindow();

protected:
  pqProcessModuleGUIHelper(); 
  ~pqProcessModuleGUIHelper(); 
  
private:
  /// InitializeApplication initializes the QApplication and 
  /// the MainWindow.
  virtual int InitializeApplication(int argc, char** argv);

  /// Cleans up the main window.
  virtual void FinalizeApplication();

  /// subclasses can override this method to create their own
  /// subclass of pqMainWindow as the Main Window.
  virtual QWidget* CreateMainWindow() = 0;
  
  class pqImplementation;
  pqImplementation* const Implementation;
  
  pqProcessModuleGUIHelper(pqProcessModuleGUIHelper&); // Not implemented.
  void operator=(const pqProcessModuleGUIHelper&); // Not implemented.
};

#endif

