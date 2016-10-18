/*=========================================================================

   Program: ParaView
   Module:    pqTextureComboBox.h

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

========================================================================*/
#ifndef pqTextureComboBox_h
#define pqTextureComboBox_h

#include "pqComponentsModule.h"
#include <QComboBox>

class pqDataRepresentation;
class pqRenderView;

class vtkSMProxy;

/**
* This is a ComboBox that is used on the display tab to select available
* textures. It checks whether current representation has texture coordinates,
* if not, the widget will be disabled automatically.
* It also provides the user with an option to load new images as textures.
*/
class PQCOMPONENTS_EXPORT pqTextureComboBox : public QComboBox
{
  Q_OBJECT
  typedef QComboBox Superclass;

public:
  pqTextureComboBox(QWidget* parent = 0);
  virtual ~pqTextureComboBox();

public slots:
  /**
  * Set the representation. We need the representation, since we need to
  * update the enable state of the widget depending on whether texture
  * coordinates are available.
  */
  void setRepresentation(pqDataRepresentation* repr);

  void setRenderView(pqRenderView* rview);

  /**
  * Forces a reload of the widget. Generally one does not need to call this
  * method explicity.
  */
  void reload();

protected slots:
  /**
  * Update the enable state of the widget.
  */
  virtual void updateEnableState();

  /**
  * Called when user activates an item.
  */
  void onActivated(int);

  void updateFromProperty();

  void updateTextures();

  void proxyRegistered(const QString& groupname);
  void proxyUnRegistered(const QString& group, const QString&, vtkSMProxy* proxy);

protected:
  /**
  * Get the texture proxy associated with the given data.
  */
  vtkSMProxy* getTextureProxy(const QVariant& data) const;

  /**
  * Prompts the user to load a texture file.
  */
  void loadTexture();
  bool loadTexture(const QString& filename);

private:
  Q_DISABLE_COPY(pqTextureComboBox)

  class pqInternal;
  pqInternal* Internal;

  bool InOnActivate;
};

#endif
