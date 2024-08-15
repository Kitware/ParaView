// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqVRAvatarEvents_h
#define pqVRAvatarEvents_h

#include <QDialog>

class pqVRAvatarEvents : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  pqVRAvatarEvents(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags{});
  virtual ~pqVRAvatarEvents();

  enum AvatarEventType
  {
    Head = 0,
    LeftHand,
    RightHand
  };

  /*
   * Get/Set event name for a given event type
   */
  void getEventName(AvatarEventType type /*in*/, QString& eventName /*out*/);
  void setEventName(AvatarEventType type /*in*/, QString& eventName /*in*/);

  /*
   * Get/Set whether navigation sharing is enabled
   */
  bool getNavigationSharing();
  void setNavigationSharing(bool enabled);

  // overridden public slots
  void accept() Q_DECL_OVERRIDE;
  int exec() Q_DECL_OVERRIDE;

private:
  Q_DISABLE_COPY(pqVRAvatarEvents)

  class pqInternals;
  pqInternals* Internals;
};

#endif
