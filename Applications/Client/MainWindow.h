/*=========================================================================

   Program: ParaView
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#ifndef _MainWindow_h
#define _MainWindow_h

#include <QMainWindow>
#include <vtkIOStream.h>

class pqPlotViewModule;
class pqGenericViewModule;
class pqPipelineSource;

/// Provides the main window for the ParaView application
class MainWindow :
  public QMainWindow
{
  Q_OBJECT

public:
  MainWindow();
  ~MainWindow();

  bool compareView(const QString& ReferenceImage, double Threshold, ostream& Output, const QString& TempDirectory);
  
private slots:
  void onVCRPlay();
  void onVCRPause();
  void onVCRPlayDone();

  void onUndoLabel(const QString&);
  void onRedoLabel(const QString&);

  void onCameraUndoLabel(const QString&);
  void onCameraRedoLabel(const QString&);

  void onPreAccept();
  void onPostAccept();

  void onHelpAbout();
  void onHelpHelp();

  void onPlotAdded(pqPlotViewModule*);
  void onPlotRemoved(pqPlotViewModule*);

  void onSelectionShortcut();
  void onSelectionShortcutFinished();

  void assistantError(const QString& err);

  void setActiveView(pqGenericViewModule*);
  void setActiveSource(pqPipelineSource*);
private:
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif // !_MainWindow_h

