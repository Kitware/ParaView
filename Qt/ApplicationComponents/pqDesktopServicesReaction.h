// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqDesktopServicesReaction_h
#define pqDesktopServicesReaction_h

#include "pqReaction.h"

#include <QUrl>

/**
 * @ingroup Reactions
 * pqDesktopServicesReaction can be used to open a file (or URL) using
 * QDesktopServices. e.g. if your application wants to launch a PDF viewer to
 * open the application's User Guide, you can hookup a menu QAction to the
 * pqDesktopServicesReaction. e.g.
 * @code
 * QAction* action = ...
 * new pqDesktopServicesReaction(QUrl("file:///..../doc/UsersGuide.pdf"), action);
 * @endcode
 *
 * The URL is set as status tip on the parent action.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqDesktopServicesReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqDesktopServicesReaction(const QUrl& url, QAction* parent);
  ~pqDesktopServicesReaction() override;

  /**
   * Attempt to open a file (local or on the Web) using QDesktopServices.
   * Returns false if failed to open for some reason, other returns true.
   */
  static bool openUrl(const QUrl& url);

protected:
  void onTriggered() override { pqDesktopServicesReaction::openUrl(this->URL); }

private:
  Q_DISABLE_COPY(pqDesktopServicesReaction)
  QUrl URL;
};

#endif
