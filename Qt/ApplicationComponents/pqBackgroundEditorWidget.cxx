/*=========================================================================

   Program: ParaView
   Module: pqBackgroundEditorWidget.cxx

   Copyright (c) 2005-2012 Kitware Inc.
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

#include "pqBackgroundEditorWidget.h"
#include "ui_pqBackgroundEditorWidget.h"

#include <QGridLayout>
#include <QPushButton>

#include "pqPropertiesPanel.h"
#include "pqPropertyManager.h"
#include "pqProxy.h"
#include "pqRenderView.h"
#include "pqUndoStack.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMProxy.h"

const char* COLOR_PROPERTY = "Background";
const char* COLOR2_PROPERTY = "Background2";
const char* GRADIENT_BACKGROUND_PROPERTY = "UseGradientBackground";
const char* IMAGE_BACKGROUND_PROPERTY = "UseTexturedBackground";
const char* IMAGE_PROPERTY = "BackgroundTexture";

enum BackgroundType
{
  SINGLE_COLOR_TYPE,
  GRADIENT_TYPE,
  IMAGE_TYPE,
  TYPE_COUNT
};

enum BackgroundTypeEncoding
{
  GRADIENT_TYPE_ENCODING,
  IMAGE_TYPE_ENCODING,
  TYPE_ENCODING_COUNT
};

class pqBackgroundEditorWidget::pqInternal : public Ui::BackgroundEditorWidget
{
public:
  enum BackgroundType PreviousType;

  pqInternal(pqBackgroundEditorWidget* self)
    : PreviousType(SINGLE_COLOR_TYPE)
  {
    this->setupUi(self);
    this->mainLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->mainLayout->setSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
    this->page1Layout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->page1Layout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
    this->page1Layout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
    this->page3Layout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->page3Layout->setSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
  }
};

pqBackgroundEditorWidget::pqBackgroundEditorWidget(
  vtkSMProxy* smProxy, vtkSMPropertyGroup* smGroup, QWidget* parentObject)
  : Superclass(smProxy, smGroup, parentObject)
  , Internal(new pqInternal(this))
{
  Ui::BackgroundEditorWidget& ui = *this->Internal;
  connect(ui.BackgroundType, SIGNAL(currentIndexChanged(int)), this,
    SLOT(currentIndexChangedBackgroundType(int)));
  connect(ui.RestoreDefaultColor, SIGNAL(clicked()), this, SLOT(clickedRestoreDefaultColor()));
  connect(ui.RestoreDefaultColor2, SIGNAL(clicked()), this, SLOT(clickedRestoreDefaultColor2()));

  this->addPropertyLink(ui.Color, COLOR_PROPERTY);
  this->addPropertyLink(ui.Color2, COLOR2_PROPERTY);

  vtkSMProperty* smProperty = smGroup->GetProperty(GRADIENT_BACKGROUND_PROPERTY);
  if (smProperty)
  {
    this->addPropertyLink(
      this, "gradientBackground", SIGNAL(gradientBackgroundChanged()), smProperty);
  }
  else
  {
    ui.BackgroundType->hide();
  }

  smProperty = smGroup->GetProperty(IMAGE_BACKGROUND_PROPERTY);
  if (smProperty)
  {
    this->addPropertyLink(this, "imageBackground", SIGNAL(imageBackgroundChanged()), smProperty);
  }
  else
  {
    ui.BackgroundType->hide();
  }
  currentIndexChangedBackgroundType(this->Internal->PreviousType);
}

pqBackgroundEditorWidget::~pqBackgroundEditorWidget()
{
  delete this->Internal;
  this->Internal = 0;
}

void pqBackgroundEditorWidget::currentIndexChangedBackgroundType(int type)
{
  const int ROWS = 3;
  const int COLS = 2;
  Ui::BackgroundEditorWidget& ui = *this->Internal;
  const char* colorButtonName[TYPE_COUNT] = { "Color", "Color 1", "Color" };
  int currentPage[TYPE_COUNT] = { 0, 0, 1 };
  bool visibleControls[TYPE_COUNT][ROWS] = { { true, false, false }, { true, true, false },
    { false, false, true } };
  QWidget* controls[ROWS][COLS] = { { ui.Color, ui.RestoreDefaultColor },
    { ui.Color2, ui.RestoreDefaultColor2 }, { ui.Image, 0 } };
  for (int i = 0; i < ROWS; ++i)
  {
    for (int j = 0; j < COLS; ++j)
    {
      QWidget* control = controls[i][j];
      if (control)
      {
        control->setVisible(visibleControls[type][i]);
      }
    }
  }
  ui.stackedWidget->setCurrentIndex(currentPage[type]);
  ui.Color->setText(colorButtonName[type]);
  fireGradientAndImageChanged(this->Internal->PreviousType, type);
  this->Internal->PreviousType = BackgroundType(type);
}

bool pqBackgroundEditorWidget::gradientBackground() const
{
  return this->Internal->BackgroundType->currentIndex() == GRADIENT_TYPE;
}

void pqBackgroundEditorWidget::setGradientBackground(bool gradientValue)
{
  enum BackgroundType typeFunction[TYPE_COUNT][2] = { { SINGLE_COLOR_TYPE, GRADIENT_TYPE },
    { SINGLE_COLOR_TYPE, GRADIENT_TYPE },
    { IMAGE_TYPE, GRADIENT_TYPE } }; // gradient has precendece over image
  int newType = typeFunction[this->Internal->PreviousType][gradientValue];
  fireGradientAndImageChanged(this->Internal->PreviousType, newType);
  this->Internal->BackgroundType->setCurrentIndex(newType);
}

bool pqBackgroundEditorWidget::imageBackground() const
{
  return this->Internal->BackgroundType->currentIndex() == IMAGE_TYPE;
}

void pqBackgroundEditorWidget::setImageBackground(bool imageValue)
{
  enum BackgroundType typeFunction[TYPE_COUNT][2] = { { SINGLE_COLOR_TYPE, IMAGE_TYPE },
    { GRADIENT_TYPE, GRADIENT_TYPE }, // gradient has precendece over image
    { SINGLE_COLOR_TYPE, IMAGE_TYPE } };
  enum BackgroundType newType = typeFunction[this->Internal->PreviousType][imageValue];
  fireGradientAndImageChanged(this->Internal->PreviousType, newType);
  this->Internal->BackgroundType->setCurrentIndex(newType);
}

void pqBackgroundEditorWidget::fireGradientAndImageChanged(int oldType, int newType)
{
  bool gradientChanged[TYPE_COUNT][TYPE_COUNT] = { { false, true, false }, { true, false, true },
    { false, true, false } };
  bool imageChanged[TYPE_COUNT][TYPE_COUNT] = { { false, false, true }, { false, false, true },
    { true, true, false } };
  if (gradientChanged[oldType][newType])
  {
    emit gradientBackgroundChanged();
  }
  if (imageChanged[oldType][newType])
  {
    emit imageBackgroundChanged();
  }
}

void pqBackgroundEditorWidget::setView(pqView* _view)
{
  this->Internal->Image->setRenderView(qobject_cast<pqRenderView*>(_view));
}

void pqBackgroundEditorWidget::clickedRestoreDefaultColor()
{
  this->changeColor(COLOR_PROPERTY);
}

void pqBackgroundEditorWidget::clickedRestoreDefaultColor2()
{
  this->changeColor(COLOR2_PROPERTY);
}

void pqBackgroundEditorWidget::changeColor(const char* propertyName)
{
  vtkSMProperty* _property = this->propertyGroup()->GetProperty(propertyName);
  BEGIN_UNDO_SET("Restore Default Color");
  _property->ResetToDefault();
  emit this->changeFinished();
  END_UNDO_SET();
}
