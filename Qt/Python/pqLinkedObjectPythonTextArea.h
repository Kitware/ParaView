/*=========================================================================

   Program: ParaView
   Module:    pqLinkedObjectPythonTextArea.h

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
  std::unique_ptr<pqLinkedObjectInterface> clone() const noexcept override
  {
    return std::unique_ptr<pqLinkedObjectPythonTextArea>(new pqLinkedObjectPythonTextArea(*this));
  }

  /**
   * Link this object to the \ref other.
   * Note that the link is established if and only if the given pqLinkedObjectInterface is no empty.
   */
  void link(pqLinkedObjectInterface* other) override;

  pqPythonTextArea& TextArea;
};

#endif // pqLinkedObjectPythonTextArea_h
