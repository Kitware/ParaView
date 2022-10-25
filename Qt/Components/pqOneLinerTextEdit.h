/*=========================================================================

   Program: ParaView
   Module:    pqOneLinerTextEdit.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#ifndef pqOneLinerTextEdit_h
#define pqOneLinerTextEdit_h

#include "pqComponentsModule.h"
#include "pqTextEdit.h"

/**
 * pqOneLinerTextEdit is a specialization of pqTextEdit to handle one-liner expressions.
 *
 * This widget differs from pqTextEdit with the following:
 *  * New lines characters are disabled from input event and cleaned from pasted text.
 *  * Text is wrapped to fit the widget width.
 *  * Widget height is adapted to the content, so the whole text is displayed.
 */
class PQCOMPONENTS_EXPORT pqOneLinerTextEdit : public pqTextEdit
{
  Q_OBJECT
  typedef pqTextEdit Superclass;

public:
  pqOneLinerTextEdit(QWidget* parent = nullptr);
  ~pqOneLinerTextEdit() override = default;

protected:
  /**
   * Reimplemented to disable "Enter" and "Return" key
   */
  void keyPressEvent(QKeyEvent* e) override;

  /**
   * Reimplemented to update widget height depending on
   * text wrapping.
   */
  void resizeEvent(QResizeEvent* e) override;

  /**
   * Reimplemented to avoid carriage return in pasted text.
   * Also remove redundant spaces.
   * see QString::simplified()
   */
  void insertFromMimeData(const QMimeData* source) override;

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Adjust widget height to the number of displayed lines.
   */
  void adjustToText();

private:
  Q_DISABLE_COPY(pqOneLinerTextEdit)
};

#endif
