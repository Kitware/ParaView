/*=========================================================================

   Program: ParaView
   Module:    pqHelpWindow.h

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
#ifndef __pqHelpWindow_h 
#define __pqHelpWindow_h

#include <QMainWindow>
#include "QtWidgetsExport.h"

class QHelpEngine;
class QTextBrowser;

/// pqHelpWindow provides a assistant-like window  for showing embedded
/// documentation in any application. It supports showing of Qt compressed help 
/// files (*.qch files generated from *.qhp files). The qch file can be
/// either on disk or an embedded qt-resource.
class QTWIDGETS_EXPORT pqHelpWindow : public QMainWindow
{
  Q_OBJECT
  typedef QMainWindow Superclass;
public:
  pqHelpWindow(
    const QString& window_title, QWidget* parent=0, Qt::WindowFlags flags=0);
  virtual ~pqHelpWindow();

  /// Register a *.qch documentation file.
  /// This can be a file in the qt-resource space as well.
  /// Note these are Qt compressed help files i.e. *.qch files generated
  /// from *.qhp and not the help collection files (*.qhc generated from *.qhcp)
  /// which have the assistant configuration details.
  /// Unlike registering the documentation with the Qt assistant, this is not
  /// remembered across sessions and must be done each time.
  /// On successful loading, returns the namespace name provided by the help
  /// file.
  virtual QString registerDocumentation(const QString& qchfilename);

  /// Requests showing of a particular page. The url must begin with "qthelp:"
  /// scheme when referring to pages from the help files.
  virtual void showPage(const QString& url);

  /// Experimental. I am not sure how to reliably determine the home page.
  virtual void showHomePage(const QString& namespace_name);

signals:
  /// fired to relay warning messages from the help system.
  void helpWarnings(const QString&);

protected:
  QHelpEngine* HelpEngine;
  QTextBrowser* Browser;

private:
  Q_DISABLE_COPY(pqHelpWindow)

  class pqTextBrowser;
};

#endif


