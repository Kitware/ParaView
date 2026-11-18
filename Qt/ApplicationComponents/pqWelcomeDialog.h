// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqWelcomeDialog_h
#define pqWelcomeDialog_h

#include "pqApplicationComponentsModule.h"
#include "pqServerConfiguration.h" // Needed for QList
#include <QDialog>

class pqServerResource;
class pqServer;

namespace Ui
{
class pqWelcomeDialog;
}

/**
 * This class provides a welcome dialog screen that you see in many applications.
 * The intent is to provide an on-ramp with a lower learning curve than the blank
 * ParaView screen.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqWelcomeDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  explicit pqWelcomeDialog(QWidget* parent = nullptr);
  ~pqWelcomeDialog() override;

Q_SIGNALS:
  void serverConnected(const pqServerConfiguration& serverConfig);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  virtual void onGettingStartedGuideClicked();
  virtual void onExampleVisualizationsClicked();
  virtual void onDocumentationClicked();

private Q_SLOTS:
  void connectServer(int serverIndex);
  void onDoNotShowAgainButton();

private: // NOLINT(readability-redundant-access-specifiers)
  void openResource(const pqServerResource& resource);
  bool openResource(pqServer* server, const pqServerResource& resource);

  Ui::pqWelcomeDialog* ui;

  QList<pqServerConfiguration> Configurations;
};

#endif // pqWelcomeDialog_h
