/*=========================================================================

   Program: ParaView
   Module:    pqCustomViewButtonDialog.h

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
#ifndef pqCustomViewDialog_h
#define pqCustomViewDialog_h
// .NAME pqCustomViewDialog - Dialog for configuring custom view buttons.
//
// .SECTION Description
// Provides the machinery for associating the current camera configuration
// to a custom view button, and importing or exporting all of the custom view
// button configurations.
//
// .SECTION See Also
// pqCameraDialog
//
// .SECTION Thanks
// This class was contributed by SciberQuest Inc.

#include <QDialog>
#include <QLineEdit>
#include <QList>
#include <QString>
#include <QStringList>

class pqCustomViewButtonDialogUI;
class vtkSMCameraConfigurationReader;

class pqCustomViewButtonDialog : public QDialog
{
  Q_OBJECT

public:
  // Description:
  // Create and initialize the dialog.
  pqCustomViewButtonDialog(QWidget* parent, Qt::WindowFlags f, QStringList& toolTips,
    QStringList& configurations, QString& currentConfig);

  ~pqCustomViewButtonDialog();

  // Description:
  // Constant variable that contains the default name for the tool tips.
  const static QString DEFAULT_TOOLTIP;

  // Description:
  // Set/get a list of tool tips, one for each button.
  void setToolTips(QStringList& toolTips);
  QStringList getToolTips();

  // Description:
  // Set/get a list of camera configurations, one for each buttton.
  void setConfigurations(QStringList& configs);
  QStringList getConfigurations();

  // Descrition:
  // Set/get the current camera configuration.
  void setCurrentConfiguration(QString& config);
  QString getCurrentConfiguration();

private slots:
  void importConfigurations();
  void exportConfigurations();
  void clearAll();

  void assignCurrentView(int id);
  void assignCurrentView0() { this->assignCurrentView(0); }
  void assignCurrentView1() { this->assignCurrentView(1); }
  void assignCurrentView2() { this->assignCurrentView(2); }
  void assignCurrentView3() { this->assignCurrentView(3); }

private:
  pqCustomViewButtonDialog() {}

  int NButtons;

  QList<QLineEdit*> ToolTips;
  QStringList Configurations;
  QString CurrentConfiguration;

  pqCustomViewButtonDialogUI* ui;
};
#endif
