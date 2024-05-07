// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqKeyEventFilter_h
#define pqKeyEventFilter_h

#include "pqCoreModule.h"

#include <QObject>

#include <QList>
#include <QMap>
#include <QSharedPointer>

class QKeyEvent;

/**
 * \brief: A class to handle QKeyEvent in an eventFilter and send high level signals.
 *
 * \details: pqKeyEventFilter is a configurable class that reimplements eventFilter()
 * and emit high level signals to communicate about key events.
 *
 * This is specially useful when a container widget wants to know about key events
 * regardless of which of its child receive the event.
 */
class PQCORE_EXPORT pqKeyEventFilter : public QObject
{
  Q_OBJECT;
  typedef QObject Superclass;

public:
  pqKeyEventFilter(QObject* parent);
  ~pqKeyEventFilter() override;

  /**
   * An enum for the key events categories.
   */
  enum KeyCategory
  {
    Accept,
    Reject,
    TextInput,
    Motion,
    Focus
  };

  /**
   * install event filter on monitored object.
   */
  void filter(QObject* monitored);

  ///@{
  /**
   * Filter monitored object but still forward event that fall
   * under the given type.
   */
  void forwardTypes(QObject* monitored, QList<int> types);
  void forwardType(QObject* monitored, int type);
  ///@}

Q_SIGNALS:
  /**
   * Emited on text key pressed, without modifier.
   * Text keys:
   *   - numbers
   *   - letters
   *   - space
   *   - backspace
   *
   *  As defined by QChar::isLetterOrNumber and QChar::isSpace
   */
  void textChanged(int key);

  /**
   * Emited on enter or return pressed
   * modified is true if Shift is pressed.
   */
  void accepted(bool modified);

  /**
   * Emited on Escape.
   */
  void rejected();

  /**
   * Emited on a motion key was pressed.
   * Motion keys:
   *  the four arrows, home, end, page down and page up.
   */
  void motion(int key);

  /**
   * Emited on Tab press.
   * reverse is true on Shift + Tab.
   */
  void focusChanged();

protected:
  /**
   * Reimplemented to handled QKeyEvent.
   */
  bool eventFilter(QObject* obj, QEvent* event) override;

private:
  bool isAcceptType(int key);
  bool isRejectType(int key);
  bool isTextUpdateType(QChar key);
  bool isMotionType(int key);
  bool isFocusType(int key);

  bool shouldHandle(QObject* obj, int type);

  QMap<QObject*, QList<int>> ForwardTypes;
  QList<QObject*> Monitored;

  QSharedPointer<QKeyEvent> LastEvent;
};

#endif
