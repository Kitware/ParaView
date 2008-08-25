/*=========================================================================

   Program: ParaView
   Module:    pqLineEdit.h

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
#ifndef __pqLineEdit_h 
#define __pqLineEdit_h

#include <QLineEdit>
#include "QtWidgetsExport.h"
/// pqLineEdit is a specialization of QLineEdit which provide a new property
/// 'text2'. When the text on the line widget is set using this 'text2' property
/// (or using setTextAndResetCursor()), the after the set, the cursor position
/// is reset to 0.
class QTWIDGETS_EXPORT pqLineEdit : public QLineEdit
{
  Q_OBJECT
  Q_PROPERTY(QString text2 READ text WRITE setTextAndResetCursor)
  typedef QLineEdit Superclass;
public:
  pqLineEdit(QWidget *parent=0);
  pqLineEdit(const QString &contents, QWidget *parent=0);

  virtual ~pqLineEdit();

public slots:
  void setTextAndResetCursor(const QString& text);

private:
  pqLineEdit(const pqLineEdit&); // Not implemented.
  void operator=(const pqLineEdit&); // Not implemented.
};

#endif


