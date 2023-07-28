// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSelectionLinkDialog.h"
#include "ui_pqSelectionLinkDialog.h"

class pqSelectionLinkDialog::pqInternal : public Ui::SelectionLinkDialog
{
};

//-----------------------------------------------------------------------------
pqSelectionLinkDialog::pqSelectionLinkDialog(QWidget* _parent, Qt::WindowFlags f)
  : Superclass(_parent, f)
{
  this->Internal = new pqInternal();
  this->Internal->setupUi(this);
}

//-----------------------------------------------------------------------------
pqSelectionLinkDialog::~pqSelectionLinkDialog()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
bool pqSelectionLinkDialog::convertToIndices() const
{
  return (this->Internal->convertToIndicesRadioButton->isChecked());
}
