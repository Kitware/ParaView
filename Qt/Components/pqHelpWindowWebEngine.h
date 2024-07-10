// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqHelpWindowWebEngine_h
#define pqHelpWindowWebEngine_h

/**
 *============================================================================
 * This is an internal header used by pqHelpWindow.
 * This header gets included when PARAVIEW_USE_QTWEBENGINE is ON.
 *============================================================================
 */

#include <QBuffer>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QTimer>
#include <QWebEngineHistory>
#include <QWebEngineProfile>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlSchemeHandler>
#include <QWebEngineView>

namespace
{

class pqUrlSchemeHandler : public QWebEngineUrlSchemeHandler
{
  typedef QWebEngineUrlSchemeHandler Superclass;

public:
  pqUrlSchemeHandler(QHelpEngineCore* engine)
    : Engine(engine)
  {
  }
  ~pqUrlSchemeHandler() = default;

  void requestStarted(QWebEngineUrlRequestJob* request) override
  {
    QMap<QString, QString> extension_type_map;
    extension_type_map["jpg"] = "image/jpeg";
    extension_type_map["jpeg"] = "image/jpeg";
    extension_type_map["png"] = "image/png";
    extension_type_map["gif"] = "image/gif";
    extension_type_map["tiff"] = "image/tiff";
    extension_type_map["htm"] = "text/html";
    extension_type_map["html"] = "text/html";
    extension_type_map["css"] = "text/css";
    extension_type_map["xml"] = "text/xml";

    QUrl url = request->requestUrl();
    QString extension = QFileInfo(url.path()).suffix().toLower();
    QString content_type = extension_type_map.value(extension, "text/plain");

    QByteArray array = this->Engine->fileData(url);
    QBuffer* buffer = new QBuffer;
    buffer->setData(array);
    buffer->open(QIODevice::ReadOnly);
    connect(buffer, &QIODevice::aboutToClose, buffer, &QObject::deleteLater);
    request->reply(content_type.toUtf8(), buffer);
  }

private:
  QHelpEngineCore* Engine;
};
//----------------------------------------------------------------------------------
/**
 * Extend QWebView to support the interface expected in pqBrowserTemplate.
 */
class pqWebView : public QWebEngineView
{
  typedef QWebEngineView Superclass;

public:
  pqWebView(QWidget* parentObject)
    : Superclass(parentObject)
  {
  }
  ~pqWebView() = default;

  static pqWebView* newInstance(QHelpEngine* engine, pqHelpWindow* self)
  {
    pqWebView* instance = new pqWebView(self);
    QWebEngineProfile* profile = QWebEngineProfile::defaultProfile();
    profile->installUrlSchemeHandler("qthelp", new pqUrlSchemeHandler(engine));
    self->connect(instance, &pqWebView::loadFinished, self, &pqHelpWindow::updateHistoryButtons);
    return instance;
  }

  QUrl url() { return this->history()->currentItem().url(); }

  QUrl goBackward()
  {
    this->history()->back();
    return this->history()->currentItem().url();
  }

  QUrl goForward()
  {
    this->history()->forward();
    return this->history()->currentItem().url();
  }

  bool canGoBackward() { return this->history()->canGoBack(); }

  bool canGoForward() { return this->history()->canGoForward(); }

private:
  Q_DISABLE_COPY(pqWebView)
};

} // end of namespace
#endif
