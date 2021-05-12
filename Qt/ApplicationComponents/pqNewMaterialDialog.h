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

/**
 * @brief pqNewMaterialDialog is a dialog window that is used to create a new
 * material in the material editor widget.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqNewMaterialDialog : public pqDialog
{
  Q_OBJECT
  typedef pqDialog Superclass;

public:
  pqNewMaterialDialog(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
  ~pqNewMaterialDialog() override;

  /**
   * Set the OSPRay material library used to check if the material name is
   * available in OSPRay.
   */
  void setMaterialLibrary(vtkOSPRayMaterialLibrary* lib);
  /**
   * Return the name of the material
   */
  const QString& name() { return this->Name; }
  /**
   * Return the type of the material
   */
  const QString& type() { return this->Type; }

public slots:
  /**
   * Store the name and type of the material after accept.
   * This slot is connected in pqMaterialEditor to add a new material to
   * the library.
   */
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
