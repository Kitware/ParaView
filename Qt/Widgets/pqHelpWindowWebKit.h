/*=========================================================================

   Program: ParaView
   Module:  pqHelpWindowWebKit.h

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
#ifndef pqHelpWindowWebKit_h
#define pqHelpWindowWebKit_h

/**
*============================================================================
* This is an internal header used by pqHelpWindow.
* This header gets included when PARAVIEW_USE_QTWEBKIT is ON.
*============================================================================
*/

#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QTimer>
#include <QWebPage>
#include <QWebView>
#include <QtNetwork/QNetworkReply>

namespace
{
// ****************************************************************************
//            CLASS pqHelpWindowNetworkReply
// ****************************************************************************
/**
* Internal class used to add support to QWebView to load files from
* QHelpEngine.
*/
class pqHelpWindowNetworkReply : public QNetworkReply
{
  typedef QNetworkReply Superclass;

public:
  pqHelpWindowNetworkReply(const QUrl& url, QHelpEngineCore* helpEngine);

  virtual void abort() {}
  virtual qint64 bytesAvailable() const
  {
    return (this->RawData.size() - this->Offset) + this->Superclass::bytesAvailable();
  }
  virtual bool isSequential() const { return true; }
protected:
  virtual qint64 readData(char* data, qint64 maxSize);

  QByteArray RawData;
  qint64 Offset;

private:
  Q_DISABLE_COPY(pqHelpWindowNetworkReply)
};

//-----------------------------------------------------------------------------
pqHelpWindowNetworkReply::pqHelpWindowNetworkReply(const QUrl& my_url, QHelpEngineCore* engine)
  : Superclass(engine)
  , Offset(0)
{
  Q_ASSERT(engine);

  this->RawData = engine->fileData(my_url);

  QString content_type = "text/plain";
  QString extension = QFileInfo(my_url.path()).suffix().toLower();
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

  if (extension_type_map.contains(extension))
  {
    content_type = extension_type_map[extension];
  }

  this->setHeader(QNetworkRequest::ContentLengthHeader, QVariant(this->RawData.size()));
  this->setHeader(QNetworkRequest::ContentTypeHeader, content_type);
  this->open(QIODevice::ReadOnly | QIODevice::Unbuffered);
  this->setUrl(my_url);
  QTimer::singleShot(0, this, SIGNAL(readyRead()));
  QTimer::singleShot(0, this, SLOT(finished()));
}

//-----------------------------------------------------------------------------
qint64 pqHelpWindowNetworkReply::readData(char* data, qint64 maxSize)
{
  if (this->Offset <= this->RawData.size())
  {
    qint64 end = qMin(this->Offset + maxSize, static_cast<qint64>(this->RawData.size()));
    qint64 delta = end - this->Offset;
    memcpy(data, this->RawData.constData() + this->Offset, delta);
    this->Offset += delta;
    return delta;
  }
  return -1;
}

// ****************************************************************************
//    CLASS pqNetworkAccessManager
// ****************************************************************************
//-----------------------------------------------------------------------------
class pqNetworkAccessManager : public QNetworkAccessManager
{
  typedef QNetworkAccessManager Superclass;
  QPointer<QHelpEngineCore> Engine;

public:
  pqNetworkAccessManager(
    QHelpEngineCore* helpEngine, QNetworkAccessManager* manager, QObject* parentObject)
    : Superclass(parentObject)
    , Engine(helpEngine)
  {
    Q_ASSERT(manager != NULL && helpEngine != NULL);

    this->setCache(manager->cache());
    this->setCookieJar(manager->cookieJar());
    this->setProxy(manager->proxy());
    this->setProxyFactory(manager->proxyFactory());
  }

protected:
  virtual QNetworkReply* createRequest(
    Operation operation, const QNetworkRequest& request, QIODevice* device)
  {
    if (request.url().scheme() == "qthelp" && operation == GetOperation)
    {
      return new pqHelpWindowNetworkReply(request.url(), this->Engine);
    }
    else
    {
      return this->Superclass::createRequest(operation, request, device);
    }
  }

private:
  Q_DISABLE_COPY(pqNetworkAccessManager)
};

//----------------------------------------------------------------------------------
/**
* Extend QWebView to support the interface expected in pqBrowserTemplate.
*/
class pqWebView : public QWebView
{
  typedef QWebView Superclass;

public:
  pqWebView(QWidget* parentObject)
    : Superclass(parentObject)
  {
  }
  ~pqWebView() {}

  static pqWebView* newInstance(QHelpEngine* engine, QWidget* parentObject)
  {
    pqWebView* instance = new pqWebView(parentObject);
    QNetworkAccessManager* oldManager = instance->page()->networkAccessManager();
    pqNetworkAccessManager* newManager =
      new pqNetworkAccessManager(engine, oldManager, parentObject);
    instance->page()->setNetworkAccessManager(newManager);
    instance->page()->setForwardUnsupportedContent(false);
    return instance;
  }

private:
  Q_DISABLE_COPY(pqWebView)
};

} // end of namespace
#endif
