// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqLinkedObjectQTextEdit_h
#define pqLinkedObjectQTextEdit_h

#include "pqLinkedObjectInterface.h"

class QTextEdit;

/**
 * \ref pqLinkedObjectQTextEdit implements
 * the \ref pqLinkedObjectInterface for a QTextEdit
 */
struct PQCORE_EXPORT pqLinkedObjectQTextEdit : public pqLinkedObjectInterface
{
  /**
   * Default constructor is deleted
   */
  pqLinkedObjectQTextEdit() = delete;

  /**
   * Constructor that takes the QTextEdit as parameter
   */
  explicit pqLinkedObjectQTextEdit(QTextEdit& textEdit) noexcept
    : TextEdit(textEdit)
  {
  }

  /**
   * Copy constructor. Do link the other object to this one if it exists
   */
  explicit pqLinkedObjectQTextEdit(const pqLinkedObjectQTextEdit& other) noexcept
    : pqLinkedObjectInterface(other)
    , TextEdit(other.TextEdit)
  {
    if (other.isLinked())
    {
      this->link(this->ConnectedTo);
    }
  }

  /**
   * Destructor that unlink this object on deletion.
   */
  ~pqLinkedObjectQTextEdit() noexcept override { this->unlink(); }

  /**
   * Constructs a new instance of \ref pqLinkedObjectQTextEdit using the default copy constructor of
   * this class
   */
  std::unique_ptr<pqLinkedObjectInterface> clone() const override
  {
    return std::unique_ptr<pqLinkedObjectQTextEdit>(new pqLinkedObjectQTextEdit(*this));
  }

  /**
   * Link this object to the other. Note that the link is established if and only if the given
   * pqLinkedObjectInterface is not nullptr.
   */
  void link(pqLinkedObjectInterface* other) override;

  /**
   * Unlink this object with the other.
   */
  void unlink() override;

  /**
   * See \ref pqLinkedObjectInterface::setText
   */
  void setText(const QString& txt) override;

  /**
   * See \ref pqLinkedObjectInterface::getText
   */
  QString getText() const override;

  /**
   * See \ref pqLinkedObjectInterface::getLinked
   */
  QObject* getLinked() const noexcept override;

  /**
   * See \ref pqLinkedObjectInterface::getName
   */
  QString getName() const override;

  QTextEdit& TextEdit;
};

#endif // pqLinkedObjectQTextEdit_h
