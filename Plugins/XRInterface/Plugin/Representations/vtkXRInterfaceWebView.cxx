// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXRInterfaceWebView.h"
#include "ui_vtkXRInterfaceWebView.h"

#include <QKeyEvent>
#include <QStringList>
#include <QWebEngineSettings>
#include <functional>
#include <sstream>

class vtkXRInterfaceWebView::pqInternals : public Ui::vtkXRInterfaceWebView
{
};

void vtkXRInterfaceWebView::constructor()
{
  this->setWindowTitle("vtkXRInterfaceWebView");
  this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
  QWidget* t_widget = new QWidget(this);
  this->Internals = new pqInternals();
  this->Internals->setupUi(t_widget);

  this->Internals->webView->settings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
  this->Internals->webView->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);

  connect(this->Internals->webView, &QWebEngineView::urlChanged,
    [this](const QUrl& url) { this->Internals->urlLabel->setText(url.toDisplayString()); });

  connect(this->Internals->backButton, &QPushButton::clicked,
    [this]() { this->Internals->webView->back(); });

  connect(this->Internals->tabButton, &QPushButton::clicked, [this]() { this->SendTab(); });
}

vtkXRInterfaceWebView::~vtkXRInterfaceWebView()
{
  delete this->Internals;
}

void vtkXRInterfaceWebView::SendTab()
{
  // Manually trigger focus.
  this->Internals->webView->setFocus();
  auto focusProxy = this->Internals->webView->focusProxy();

  QKeyEvent newKey(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, QString(QChar('\t')));
  QCoreApplication::sendEvent(focusProxy, &newKey);
  QKeyEvent releaseKey(QEvent::KeyRelease, Qt::Key_Tab, Qt::NoModifier, QString(QChar('\t')));
  QCoreApplication::sendEvent(focusProxy, &releaseKey);
}

void vtkXRInterfaceWebView::SetInputText(std::string const& val)
{
  if (!val.length())
  {
    return;
  }

  // Manually trigger focus.
  this->Internals->webView->setFocus();

  // Check that focus is set in QWebEngineView and all internal classes.
  bool ret = this->Internals->webView->hasFocus();
  auto focusProxy = this->Internals->webView->focusProxy();
  ret = focusProxy->hasFocus();

  for (auto k : val)
  {
    QKeyEvent newKey(QEvent::KeyPress, k, Qt::NoModifier, QString(QChar(k)));
    ret = QCoreApplication::sendEvent(focusProxy, &newKey);
    QKeyEvent releaseKey(QEvent::KeyRelease, k, Qt::NoModifier, QString(QChar(k)));
    ret = QCoreApplication::sendEvent(focusProxy, &releaseKey);
  }
  QKeyEvent newKey(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier, QString(QChar('\n')));
  ret = QCoreApplication::sendEvent(focusProxy, &newKey);
  QKeyEvent releaseKey(QEvent::KeyRelease, Qt::Key_Enter, Qt::NoModifier, QString(QChar('\n')));
  ret = QCoreApplication::sendEvent(focusProxy, &releaseKey);
}

void vtkXRInterfaceWebView::loadURL(std::string const& url)
{
  this->loadURL(QUrl::fromUserInput(url.c_str()));
}

void vtkXRInterfaceWebView::loadURL(QUrl url)
{
  this->Internals->webView->load(url);
}
