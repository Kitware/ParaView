// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
