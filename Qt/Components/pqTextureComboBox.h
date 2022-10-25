/*=========================================================================

   Program: ParaView
   Module:  pqTextureComboBox.h

   Copyright (c) 2005,2019 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   1712 Route 9, Suite 300
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
#ifndef pqTextureComboBox_h
#define pqTextureComboBox_h

#include "pqComponentsModule.h"
#include "vtkNew.h"

#include <QComboBox>

/**
 * This is a ComboBox that is used on the display tab to select available
 * textures. It can be used with Representations, Sources and Views.
 * It provides the user the optional feature of loading new images as
 * textures directly from the combobox. One can choose to disable this
 * feature by setting canLoadNew to false in the constructor. If omitted
 * them CanLoadNew is true.
 */
class vtkSMProxyGroupDomain;
class vtkSMProxy;
class vtkEventQtSlotConnect;
class PQCOMPONENTS_EXPORT pqTextureComboBox : public QComboBox
{
  Q_OBJECT
  typedef QComboBox Superclass;

public:
  pqTextureComboBox(vtkSMProxyGroupDomain* domain, QWidget* parent = nullptr);
  pqTextureComboBox(vtkSMProxyGroupDomain* domain, bool canLoadNew, QWidget* parent = nullptr);
  ~pqTextureComboBox() override = default;

  /**
   * Update the selected index in the combobox using the provided texture if it
   * is present in the combobox
   */
  void updateFromTexture(vtkSMProxy* texture);

Q_SIGNALS:

  /**
   * Emitted whenever the texture has been changed
   */
  void textureChanged(vtkSMProxy* texture);

protected:
  void loadTexture();
  bool loadTexture(const QString& filename);

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void onCurrentIndexChanged(int index);
  void updateTextures();
  void proxyRegistered(const QString& group, const QString&, vtkSMProxy* proxy);
  void proxyUnRegistered(const QString& group, const QString&, vtkSMProxy* proxy);

private:
  Q_DISABLE_COPY(pqTextureComboBox)

  vtkSMProxyGroupDomain* Domain;
  vtkNew<vtkEventQtSlotConnect> VTKConnector;
  QString GroupName;
  bool CanLoadNew;
};

#endif
