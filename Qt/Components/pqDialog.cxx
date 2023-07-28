// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqDialog.h"

//-----------------------------------------------------------------------------
pqDialog::pqDialog(QWidget* _parent /*=0*/, Qt::WindowFlags f /*=0*/)
  : QDialog(_parent, f)
{
  this->UndoLabel = "Dialog";

  // hide the Context Help item (it's a "?" in the Title Bar for Windows, a menu item for Linux)
  this->setWindowFlags(this->windowFlags().setFlag(Qt::WindowContextHelpButtonHint, false));
}

//-----------------------------------------------------------------------------
pqDialog::~pqDialog() = default;

//-----------------------------------------------------------------------------
void pqDialog::accept()
{
  Q_EMIT this->beginUndo(this->UndoLabel);
  this->acceptInternal();
  this->Superclass::accept();
  Q_EMIT this->endUndo();
}

//-----------------------------------------------------------------------------
void pqDialog::done(int r)
{
  Q_EMIT this->beginUndo(this->UndoLabel);
  this->doneInternal(r);
  this->Superclass::done(r);
  Q_EMIT this->endUndo();
}
