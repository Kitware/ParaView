/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqPointSpriteTextureComboBox.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME pqPointSpriteTextureComboBox
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>

#include "pqPointSpriteTextureComboBox.h"
#include "vtkPointSpriteProperty.h"

pqPointSpriteTextureComboBox::pqPointSpriteTextureComboBox(QWidget* parentObject)
  : Superclass(parentObject)
{
  this->RenderMode = vtkPointSpriteProperty::TexturedSprite;
  this->CachedTextureIndex = -1;
}

pqPointSpriteTextureComboBox::~pqPointSpriteTextureComboBox()
{

}

void pqPointSpriteTextureComboBox::updateEnableState()
{
  if (this->RenderMode == vtkPointSpriteProperty::TexturedSprite)
    {
    this->setEnabled(true);
    this->setToolTip("Select/Load texture to apply on sprites.");
    }
  else
    {
    if (this->isEnabled())
      {
      this->CachedTextureIndex = this->currentIndex();
      }
    this->setEnabled(false);
    this->setToolTip(
        "Textures are only used in the TexturedSprite render mode.");
    }
}

// Description:
// Set the point sprite render mode (texture, fast sphere, exact sphere, simple point)
// this in turns updates the enable state and active the texture
void pqPointSpriteTextureComboBox::setRenderMode(int mode)
{
  this->RenderMode = mode;
  updateEnableState();
  updateTexture();
}

void pqPointSpriteTextureComboBox::updateTexture()
{
  if (this->isEnabled())
    {
    if (this->CachedTextureIndex != -1)
      {
      this->onActivated(this->CachedTextureIndex);
      }
    else
      {
      this->onActivated(this->currentIndex());
      }
    }
  else
    {
    this->onActivated(0);
    }
}

