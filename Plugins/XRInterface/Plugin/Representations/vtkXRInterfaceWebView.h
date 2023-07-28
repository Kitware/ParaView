// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXRInterfaceWebView
 * @brief   Simple web browser for VTK
 *
 * This class brings a very basic web browser for use in XR
 */

#include <QUrl> // for ivar and method sig
#include <QWidget>

class vtkXRInterfaceWebView : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  vtkXRInterfaceWebView(QWidget* p = nullptr)
    : Superclass(p)
  {
    this->constructor();
  }
  ~vtkXRInterfaceWebView();

  void loadURL(std::string const& url);
  void loadURL(QUrl url);

  void SetInputText(std::string const& val);
  void SendTab();

private:
  void constructor();

  class pqInternals;
  pqInternals* Internals;
};
