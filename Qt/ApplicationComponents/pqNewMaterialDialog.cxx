/*=========================================================================

   Program: ParaView
   Module:    pqNewMaterialDialog.cxx

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
