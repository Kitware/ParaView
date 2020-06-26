/*=========================================================================

   Program: ParaView
   Module:    pqDialog.h

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
#ifndef pqDialog_h
#define pqDialog_h

#include "pqComponentsModule.h"
#include <QDialog>

/**
* This is a QDialog subclass that is aware of the undo-redo
* sub-system. Many dialogs show information about server manager
* objects. The user can change the information and then when he/she
* hits "Accept" or "Apply", we change the underlying server manager
* object(s). For this change to be undoable, it is essential that
* the undo set generation is commenced before doing any
* changes to the server manager. This manages the start/end
* of building the undo stack for us. For any such dialogs, instead of
* using a QDialog, one should simply use this.
* One can use the accepted(), finished() signals safely to perform
* any changes to the server manager. However, is should not
* override the accept() or done() methods, instead one should override
* acceptInternal() or doneInternal().
*/
class PQCOMPONENTS_EXPORT pqDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  pqDialog(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags{});
  ~pqDialog() override;

  /**
  * Set the label used for undo command.
  */
  void setUndoLabel(const QString& label) { this->UndoLabel = label; }
Q_SIGNALS:
  /**
  * Fired when dialog begins undo-able changes.
  * Should be connected to undo-redo stack builder.
  */
  void beginUndo(const QString&);

  /**
  * Fired when dialog is done with undo-able changes.
  * Should be connected to the undo-redo stack builder.
  */
  void endUndo();

public:
  void accept() override;

  void done(int r) override;

protected:
  /**
  * Subclassess should override this instead of accept();
  */
  virtual void acceptInternal() {}

  /**
  * Subclassess should override this instead of done().
  */
  virtual void doneInternal(int /*r*/) {}

  QString UndoLabel;
};

#endif
