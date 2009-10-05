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

#include "pqClientProcessModuleGUIHelper.h"

/*!
 * ProcessModuleGUIHelper extends pqProcessModuleGUIHelper
 * so that we can create the type of MainWindow needed for pqClient.
 *
 */
class ProcessModuleGUIHelper : public pqClientProcessModuleGUIHelper
{
public:
  static ProcessModuleGUIHelper* New();
  vtkTypeRevisionMacro(ProcessModuleGUIHelper, pqProcessModuleGUIHelper);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Start the GUI event loop.
  virtual int RunGUIStart(int argc, char** argv, int numServerProcs, int myId);

  /// postAppExec does everything after the appExec
  virtual int postAppExec() { return pqClientProcessModuleGUIHelper::postAppExec(); }


protected:
  ProcessModuleGUIHelper();
  ~ProcessModuleGUIHelper();

  /// override to implement some custom functionality for adaptive application
  virtual int preAppExec(int argc, char** argv, int numServerProcs, int myId);

  /// subclasses can override this method to create their own
  /// subclass of pqMainWindow as the Main Window.
  virtual QWidget* CreateMainWindow();

private slots:
  void setMessage(const QString &);
    
private:
  ProcessModuleGUIHelper(const ProcessModuleGUIHelper&); // Not implemented.
  void operator=(const ProcessModuleGUIHelper&); // Not implemented.
};

#endif


