// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqNewMaterialDialog.h"
#include "ui_pqNewMaterialDialog.h"

#include "vtkOSPRayMaterialLibrary.h"

//-----------------------------------------------------------------------------
class pqNewMaterialDialog::pqInternals
{
public:
  Ui::NewMaterial Ui;

  pqInternals(pqNewMaterialDialog* self) { this->Ui.setupUi(self); }

  ~pqInternals() = default;
};

//-----------------------------------------------------------------------------
pqNewMaterialDialog::pqNewMaterialDialog(QWidget* parentObject, Qt::WindowFlags flags)
  : Superclass(parentObject, flags)
  , Internals(new pqNewMaterialDialog::pqInternals(this))
{
  // set the material type list
  QStringList typesList;

  for (auto& type : vtkOSPRayMaterialLibrary::GetParametersDictionary())
  {
    typesList << type.first.c_str();
  }

  this->Internals->Ui.MaterialType->insertItems(0, typesList);

  QObject::connect(
    this->Internals->Ui.ButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  QObject::connect(
    this->Internals->Ui.ButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

//-----------------------------------------------------------------------------
void pqNewMaterialDialog::accept()
{
  if (!this->MaterialLibrary)
  {
    this->done(Rejected);
    return;
  }

  this->Name = this->Internals->Ui.MaterialName->text().trimmed();

  if (this->Name.isEmpty())
  {
    this->done(Rejected);
    return;
  }

  auto currentNames = this->MaterialLibrary->GetMaterialNames();

  if (currentNames.find(this->Name.toStdString()) != currentNames.end())
  {
    this->done(Rejected);
    return;
  }

  this->Type = this->Internals->Ui.MaterialType->currentText();

  this->done(Accepted);
}

//-----------------------------------------------------------------------------
pqNewMaterialDialog::~pqNewMaterialDialog()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqNewMaterialDialog::setMaterialLibrary(vtkOSPRayMaterialLibrary* lib)
{
  this->MaterialLibrary = lib;
}
