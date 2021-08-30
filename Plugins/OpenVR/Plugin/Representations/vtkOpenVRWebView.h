
/*=========================================================================

  Program:   ParaView

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenVRWebView
 * @brief   Simple web browser for VTK
 *
 * This class brings a very basic web browser for use in VR
 */

#include <QUrl> // for ivar and method sig
#include <QWidget>

class vtkOpenVRWebView : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  vtkOpenVRWebView(QWidget* p = nullptr)
    : Superclass(p)
  {
    this->constructor();
  }
  ~vtkOpenVRWebView();

  void loadURL(std::string const& url);
  void loadURL(QUrl url);

  void SetInputText(std::string const& val);
  void SendTab();

private:
  void constructor();

  class pqInternals;
  pqInternals* Internals;
};
