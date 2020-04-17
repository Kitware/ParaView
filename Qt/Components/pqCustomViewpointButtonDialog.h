/*=========================================================================

   Program: ParaView
   Module:    pqCustomViewpointButtonDialog.h

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
#ifndef pqCustomViewpointDialog_h
#define pqCustomViewpointDialog_h

#include "pqComponentsModule.h"

#include <QDialog>
#include <QLineEdit>
#include <QList>
#include <QPointer>
#include <QPushButton>
#include <QString>
#include <QStringList>

class pqCustomViewpointButtonDialogUI;
class vtkSMCameraConfigurationReader;

/*
 * @class pqCustomViewpointDialog
 * @brief Dialog for configuring custom view buttons.
 *
 * Provides the machinery for associating the current camera configuration
 * to a custom view button, and importing or exporting all of the custom view
 * button configurations.
 *
 * @section thanks Thanks
 * This class was contributed by SciberQuest Inc.
 *
 * @sa pqCameraDialog
 */
class PQCOMPONENTS_EXPORT pqCustomViewpointButtonDialog : public QDialog
{
  Q_OBJECT

public:
  /**
   * Create and initialize the dialog.
   */
  pqCustomViewpointButtonDialog(QWidget* parent, Qt::WindowFlags f, QStringList& toolTips,
    QStringList& configurations, QString& currentConfig);

  ~pqCustomViewpointButtonDialog() override;

  /**
   * Constant variable that contains the default name for the tool tips.
   */
  const static QString DEFAULT_TOOLTIP;

  /**
   * Constant variable that defines the minimum number of items.
   */
  const static int MINIMUM_NUMBER_OF_ITEMS;

  /**
   * Constant variable that defines the maximum number of items.
   */
  const static int MAXIMUM_NUMBER_OF_ITEMS;

  /**
   * Set the list of tool tips and configurations. This is the preferred way of
   * settings these as it supports changing the number of items.
   */
  void setToolTipsAndConfigurations(const QStringList& toolTips, const QStringList& configs);

  //@{
  /**
   * Set/get a list of tool tips, one for each button. The number of items in
   * the `toolTips` list must match the current number of tooltips being shown.
   * Use `setToolTipsAndConfigurations` to change the number of items.
   */
  void setToolTips(const QStringList& toolTips);
  QStringList getToolTips();
  //@}

  //@{
  /**
   * Set/get a list of camera configurations, one for each button. The number of
   * items in `configs` must match the current number of configs.
   * Use `setToolTipsAndConfigurations` to change the number of items.
   */
  void setConfigurations(const QStringList& configs);
  QStringList getConfigurations();
  //@}

  //@{
  /**
   * Set/get the current camera configuration.
   */
  void setCurrentConfiguration(const QString& config);
  QString getCurrentConfiguration();
  //@}

private Q_SLOTS:
  void appendRow();
  void importConfigurations();
  void exportConfigurations();
  void clearAll();

  void assignCurrentViewpoint();
  void deleteRow();

private:
  pqCustomViewpointButtonDialog() {}
  QStringList Configurations;
  QString CurrentConfiguration;
  pqCustomViewpointButtonDialogUI* ui;

  friend class pqCustomViewpointButtonDialogUI;
};
#endif
