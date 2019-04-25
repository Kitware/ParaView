/*=========================================================================

   Program: ParaView
   Module:    pqNewMaterialDialog.h

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
#ifndef pqNewMaterialDialog_h
#define pqNewMaterialDialog_h

#include "pqApplicationComponentsModule.h"
#include "pqDialog.h"

class vtkOSPRayMaterialLibrary;

class PQAPPLICATIONCOMPONENTS_EXPORT pqNewMaterialDialog : public pqDialog
{
  Q_OBJECT
  typedef pqDialog Superclass;

public:
  pqNewMaterialDialog(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
  ~pqNewMaterialDialog() override;

  void setMaterialLibrary(vtkOSPRayMaterialLibrary* lib);

  const QString& name() { return this->Name; }
  const QString& type() { return this->Type; }

public slots:
  void accept() override;

protected:
  vtkOSPRayMaterialLibrary* MaterialLibrary;

  QString Name;
  QString Type;

private:
  Q_DISABLE_COPY(pqNewMaterialDialog)
  class pqInternals;
  pqInternals* Internals;
  friend class pqInternals;
};

#endif
