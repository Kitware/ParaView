/*=========================================================================

   Program: ParaView
   Module:    pqTextEdit.h

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
#ifndef pqTextEdit_h
#define pqTextEdit_h

#include "pqWidgetsModule.h"
#include <QTextEdit>

class pqTextEditPrivate;

/**
* pqTextEdit is a specialization of QTextEdit which provide a
* editingFinished() signal and a textChangedAndEditingFinished().
* Unlike editingFinished() which gets fired whenever the widget looses
* focus irrespective of if the text if actually was edited,
* textChangedAndEditingFinished() is fired only when the text was changed
* as well.
*
* Important Notes:
* The editingFinished() signals and the textChangedAndEditingFinished()
* are *NOT* sent when using the setText, setPlainText and setHtml methods.
*
* Also the textChangedAndEditingFinished() is not truly emitted only when
* the text has changed and the edition is finished. For example, removing
* a letter and adding it back will cause the signal to be fired even though
* the text is the same as before.
*/
class PQWIDGETS_EXPORT pqTextEdit : public QTextEdit
{
  Q_OBJECT
  typedef QTextEdit Superclass;

public:
  pqTextEdit(QWidget* parent = 0);
  pqTextEdit(const QString& contents, QWidget* parent = 0);

  ~pqTextEdit() override;

Q_SIGNALS:
  /**
  * Unlike editingFinished() which gets fired whenever the widget looses
  * focus irrespective of if the text actually was edited,
  * textChangedAndEditingFinished() is fired only when the text was changed
  * as well.
  */
  void textChangedAndEditingFinished();

  /**
  * Just like the QLineEdit::editingFinished(), this signal is fired
  * every time the widget loses focus.
  */
  void editingFinished();

private Q_SLOTS:
  void onEditingFinished();
  void onTextEdited();

protected:
  void keyPressEvent(QKeyEvent* e) override;
  void focusOutEvent(QFocusEvent* e) override;

  QScopedPointer<pqTextEditPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(pqTextEdit);
  Q_DISABLE_COPY(pqTextEdit)
};

#endif
