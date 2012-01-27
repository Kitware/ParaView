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
#include <QPointer>
#include "QtWidgetsExport.h"

class QHelpEngine;
class QHelpEngineCore;
class QUrl;
class QWebView;

/// pqHelpWindow provides a assistant-like window  for showing help provided by
/// a QHelpEngine.
class QTWIDGETS_EXPORT pqHelpWindow : public QMainWindow
{
  Q_OBJECT
  typedef QMainWindow Superclass;
public:
  pqHelpWindow(QHelpEngine* engine,
    QWidget* parent=0, Qt::WindowFlags flags=0);
  virtual ~pqHelpWindow();

public slots:
  /// Requests showing of a particular page. The url must begin with "qthelp:"
  /// scheme when referring to pages from the help files.
  virtual void showPage(const QString& url);
  virtual void showPage(const QUrl& url);

  /// Tires to locate a file name index.html in the given namespace and then
  /// shows that page.
  virtual void showHomePage(const QString& namespace_name);

signals:
  /// fired to relay warning messages from the help system.
  void helpWarnings(const QString&);

protected slots:
  void search();

protected:
  QHelpEngine* HelpEngine;
  QWebView* Browser;

private:
  Q_DISABLE_COPY(pqHelpWindow)

  class pqNetworkAccessManager;
  friend class pqNetworkAccessManager;
};


#include <QtNetwork/QNetworkReply>
#include <QBuffer>
/// Internal class used to add support to QWebView to load files from
/// QHelpEngine.
class QTWIDGETS_EXPORT pqHelpWindowNetworkReply : public QNetworkReply
{
  Q_OBJECT;
  typedef QNetworkReply Superclass;
public:
  pqHelpWindowNetworkReply(const QUrl& url, QHelpEngineCore* helpEngine);

  virtual void abort() {}
  virtual qint64 bytesAvailable() const
    { return this->Buffer.bytesAvailable(); }
  virtual bool isSequential() const
    { return this->Buffer.isSequential(); }

private slots:
  /// reads the raw data fires signals to notify data is available.
  void process();

protected:
  virtual qint64 readData(char *data, qint64 maxSize)
    {
    return this->Buffer.read(data, maxSize);
    }

  QPointer<QHelpEngineCore> HelpEngine;
  QBuffer Buffer;

private:
  Q_DISABLE_COPY(pqHelpWindowNetworkReply)
};

#endif
