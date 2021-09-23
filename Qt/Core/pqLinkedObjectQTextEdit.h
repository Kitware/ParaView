/*=========================================================================

   Program: ParaView
   Module:    pqLinkedObjectQTextEdit.h

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
  std::unique_ptr<pqLinkedObjectInterface> clone() const noexcept override
  {
    return std::unique_ptr<pqLinkedObjectQTextEdit>(new pqLinkedObjectQTextEdit(*this));
  }

  /**
   * Link this object to the \ref other. Note that the link is established if and only if the given
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
