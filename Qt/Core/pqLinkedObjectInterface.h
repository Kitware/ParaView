// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqLinkedObjectInterface_h
#define pqLinkedObjectInterface_h

#include <pqCoreModule.h>

#include <QMetaObject>
#include <QString>

#include <memory>

/**
 * @brief The interface to a linked Qt text widget. Derived classes should implement
 * the proper behavior for all abstract functions.
 * @details The \ref pqLinkedObjectInterface acts as an interface between two Qt text widgets that
 * needs to be linked: when the associated text is updated the linked one gets updated too.
 */
struct PQCORE_EXPORT pqLinkedObjectInterface
{
  /**
   * Default constructor
   */
  pqLinkedObjectInterface() = default;

  /**
   * Copy constructor needed for the clone method
   */
  explicit pqLinkedObjectInterface(const pqLinkedObjectInterface&) = default;

  /**
   * Default virtual destructor
   */
  virtual ~pqLinkedObjectInterface() noexcept = default;

  /**
   * Pure virtual cloning this widget.
   */
  virtual std::unique_ptr<pqLinkedObjectInterface> clone() const = 0;

  /**
   * Link this object to the other. The expected behavior should be that when this object is
   * updated, the other is too.
   */
  virtual void link(pqLinkedObjectInterface* other) = 0;

  /**
   * Unlinks the objects
   */
  virtual void unlink() = 0;

  /**
   * Used to trigger on and off an object signal
   */
  enum class QtSignalState : bool
  {
    On = true,
    Off = false
  };

  /**
   * Updates the object text. This effectively should override the current content of the widget
   **/
  virtual void setText(const QString& txt) = 0;

  /**
   * Returns the current buffer associated with this widget
   **/
  virtual QString getText() const = 0;

  /**
   * Returns the linked QOBject
   */
  virtual QObject* getLinked() const noexcept = 0;

  /**
   * Returns the linked QObject name
   */
  virtual QString getName() const = 0;

  /**
   * Returns true if this object is connected to another one
   */
  bool isLinked() const noexcept { return this->ConnectedTo != nullptr; }

protected:
  pqLinkedObjectInterface* ConnectedTo = nullptr;
  QMetaObject::Connection Connection;
  bool SettingText = false;
};

#endif // pqLinkedObjectInterface_h
