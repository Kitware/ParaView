// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqLinkedObjectPythonTextArea_h
#define pqLinkedObjectPythonTextArea_h

#include "pqPythonModule.h"

#include "pqLinkedObjectQTextEdit.h"

class pqPythonTextArea;

/**
 * pqLinkedObjectPythonTextArea implements the pqLinkedObjectInterface for a QTextEdit using the
 * existing pqLinkedObjectQTextEdit as a parent.
 */
struct PQPYTHON_EXPORT pqLinkedObjectPythonTextArea : public pqLinkedObjectQTextEdit
{
  pqLinkedObjectPythonTextArea() = delete;

  /**
   * Constructor that takes a reference to the \ref pqPythonTextArea as an argument and stores it in
   * the structure.
   */
  explicit pqLinkedObjectPythonTextArea(pqPythonTextArea& textArea) noexcept;

  /**
   * Copy constructor
   */
  explicit pqLinkedObjectPythonTextArea(const pqLinkedObjectPythonTextArea& other) noexcept
    : pqLinkedObjectQTextEdit(other)
    , TextArea(other.TextArea)
  {
    if (other.isLinked())
    {
      this->link(this->ConnectedTo);
    }
  }

  /**
   * Default destructor
   */
  ~pqLinkedObjectPythonTextArea() noexcept override = default;

  /**
   * Constructs a new instance of \ref pqLinkedObjectPythonTextArea using the default copy
   * constructor of this class
   */
  std::unique_ptr<pqLinkedObjectInterface> clone() const override
  {
    return std::unique_ptr<pqLinkedObjectPythonTextArea>(new pqLinkedObjectPythonTextArea(*this));
  }

  /**
   * Link this object to the other.
   * Note that the link is established if and only if the given pqLinkedObjectInterface is no empty.
   */
  void link(pqLinkedObjectInterface* other) override;

  pqPythonTextArea& TextArea;
};

#endif // pqLinkedObjectPythonTextArea_h
