/*=========================================================================

   Program: ParaView
   Module:    pqLinkedObjectInterface.h

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
  virtual std::unique_ptr<pqLinkedObjectInterface> clone() const noexcept = 0;

  /**
   * Link this object to the \ref other. The expected behavior should be that when this object is
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
