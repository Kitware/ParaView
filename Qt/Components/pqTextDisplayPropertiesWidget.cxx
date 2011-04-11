/*=========================================================================

   Program: ParaView
   Module:    pqTextDisplayPropertiesWidget.cxx

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
#include "pqTextDisplayPropertiesWidget.h"
#include "ui_pqTextDisplayPropertiesWidget.h"

#include "vtkSMProxy.h"
#include "vtkTextRepresentation.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"

#include <QPointer>

#include "pqApplicationCore.h"
#include "pqPropertyLinks.h"
#include "pqRepresentation.h"
#include "pqSignalAdaptors.h"
#include "pqStandardColorLinkAdaptor.h"
#include "pqTextRepresentation.h"
#include "pqUndoStack.h"

class pqTextDisplayPropertiesWidget::pqInternal : 
  public Ui::pqTextDisplayPropertiesWidget
{
public:
  pqInternal();
  ~pqInternal();

  QPointer<pqRepresentation> Display;
  pqPropertyLinks Links;

  pqSignalAdaptorColor *ColorAdaptor;
  pqSignalAdaptorComboBox *FontFamilyAdaptor;
  pqSignalAdaptorComboBox *TextAlignAdaptor;
};

//----------------------------------------------------------------------------
pqTextDisplayPropertiesWidget::pqInternal::pqInternal() : 
Ui::pqTextDisplayPropertiesWidget()
{
  this->ColorAdaptor = 0;
  this->FontFamilyAdaptor = 0;
  this->TextAlignAdaptor = 0;
}

//----------------------------------------------------------------------------
pqTextDisplayPropertiesWidget::pqInternal::~pqInternal() 
{
  delete this->ColorAdaptor;
  delete this->FontFamilyAdaptor;
  delete this->TextAlignAdaptor;
}

//-----------------------------------------------------------------------------
pqTextDisplayPropertiesWidget::pqTextDisplayPropertiesWidget(pqRepresentation* display, QWidget* p)
  : pqDisplayPanel(display, p)
{
  this->Internal = new pqInternal();
  this->Internal->setupUi(this);
  QObject::connect(&this->Internal->Links, SIGNAL(qtWidgetChanged()),
    this, SLOT(updateAllViews()));
  
  this->Internal->ColorAdaptor = new pqSignalAdaptorColor(
      this->Internal->buttonColor, "chosenColor", 
      SIGNAL(chosenColorChanged(const QColor&)), false);

  this->Internal->FontFamilyAdaptor = new pqSignalAdaptorComboBox(
      this->Internal->comboFontFamily);

  this->Internal->TextAlignAdaptor = new pqSignalAdaptorComboBox(
      this->Internal->comboTextAlign);

  this->setDisplay(display);

  this->Internal->buttonColor->setUndoLabel("Change Color");
  QObject::connect(this->Internal->buttonColor,
    SIGNAL(beginUndo(const QString&)),
    this, SLOT(beginUndoSet(const QString&)));
  QObject::connect(this->Internal->buttonColor,
    SIGNAL(endUndo()), this, SLOT(endUndoSet()));
}

//-----------------------------------------------------------------------------
pqTextDisplayPropertiesWidget::~pqTextDisplayPropertiesWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqTextDisplayPropertiesWidget::beginUndoSet(const QString& str)
{
  BEGIN_UNDO_SET(str);
}

//-----------------------------------------------------------------------------
void pqTextDisplayPropertiesWidget::endUndoSet()
{
  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqTextDisplayPropertiesWidget::setDisplay(pqRepresentation* display)
{
  if (this->Internal->Display == display)
    {
    return;
    }

  this->setEnabled(false);
  this->Internal->Links.removeAllPropertyLinks();
  if (this->Internal->Display)
    {
    QObject::disconnect(this->Internal->Display, 0, this, 0);
    }

  this->Internal->Display = qobject_cast<pqTextRepresentation*>(display);
  if (!this->Internal->Display)
    {
    return;
    }

  this->setEnabled(true);
  vtkSMProxy* proxy = display->getProxy();
  this->Internal->Links.addPropertyLink(
    this->Internal->Visibility, "checked", SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("Visibility"));

  this->Internal->Links.addPropertyLink(
    this->Internal->Interactivity, "checked", SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("Interactivity"));
  this->Internal->Links.addPropertyLink(
    this->Internal->Position1X, "value", SIGNAL(editingFinished()),
    proxy, proxy->GetProperty("Position"), 0);
  this->Internal->Links.addPropertyLink(
    this->Internal->Position1Y, "value", SIGNAL(editingFinished()),
    proxy, proxy->GetProperty("Position"), 1);

  this->Internal->Links.addPropertyLink(
    this->Internal->toolButtonBold, "checked", SIGNAL(toggled(bool)),
    proxy, proxy->GetProperty("Bold"), 1);
  this->Internal->Links.addPropertyLink(
    this->Internal->toolButtonItalic, "checked", SIGNAL(toggled(bool)),
    proxy, proxy->GetProperty("Italic"), 1);
  this->Internal->Links.addPropertyLink(
    this->Internal->toolButtonShadow, "checked", SIGNAL(toggled(bool)),
    proxy, proxy->GetProperty("Shadow"), 1);

  this->Internal->Links.addPropertyLink(this->Internal->ColorAdaptor, 
      "color", SIGNAL(colorChanged(const QVariant&)),
      proxy, proxy->GetProperty("Color"));
  // This manages the global property linking.
  new pqStandardColorLinkAdaptor(this->Internal->buttonColor,
    proxy, "Color");
  this->Internal->Links.addPropertyLink(this->Internal->FontFamilyAdaptor,
      "currentText", SIGNAL(currentTextChanged(const QString&)),
      proxy, proxy->GetProperty("FontFamily"));
  this->Internal->Links.addPropertyLink(this->Internal->TextAlignAdaptor,
      "currentText", SIGNAL(currentTextChanged(const QString&)),
      proxy, proxy->GetProperty("Justification"));

  this->Internal->Links.addPropertyLink(
    this->Internal->spinBoxSize, "value", SIGNAL(editingFinished()),
    proxy, proxy->GetProperty("FontSize"), 1);

  this->Internal->Links.addPropertyLink(
      this->Internal->spinBoxOpacity, "value", SIGNAL(editingFinished()),
      proxy, proxy->GetProperty("Opacity"));

  QObject::connect(this->Internal->groupBoxLocation, SIGNAL(clicked(bool)),
    this, SLOT(onTextLocationChanged(bool)));
  QObject::connect(this->Internal->toolButtonLL, SIGNAL(clicked(bool)),
    this, SLOT(onTextLocationChanged(bool)));
  QObject::connect(this->Internal->toolButtonLC, SIGNAL(clicked(bool)),
    this, SLOT(onTextLocationChanged(bool)));
  QObject::connect(this->Internal->toolButtonLR, SIGNAL(clicked(bool)),
    this, SLOT(onTextLocationChanged(bool)));
  QObject::connect(this->Internal->toolButtonUL, SIGNAL(clicked(bool)),
    this, SLOT(onTextLocationChanged(bool)));
  QObject::connect(this->Internal->toolButtonUC, SIGNAL(clicked(bool)),
    this, SLOT(onTextLocationChanged(bool)));
  QObject::connect(this->Internal->toolButtonUR, SIGNAL(clicked(bool)),
    this, SLOT(onTextLocationChanged(bool)));

}

//-----------------------------------------------------------------------------
void pqTextDisplayPropertiesWidget::onTextLocationChanged(bool checked)
{
  int winLocation = vtkTextRepresentation::AnyLocation;
  if (checked)
    {
    if(this->Internal->toolButtonLL->isChecked())
      {
      winLocation = vtkTextRepresentation::LowerLeftCorner;
      }
    else if(this->Internal->toolButtonLC->isChecked())
      {
      winLocation = vtkTextRepresentation::LowerCenter;
      }
    else if(this->Internal->toolButtonLR->isChecked())
      {
      winLocation = vtkTextRepresentation::LowerRightCorner;
      }
    else if(this->Internal->toolButtonUL->isChecked())
      {
      winLocation = vtkTextRepresentation::UpperLeftCorner;
      }
    else if(this->Internal->toolButtonUC->isChecked())
      {
      winLocation = vtkTextRepresentation::UpperCenter;
      }
    else if(this->Internal->toolButtonUR->isChecked())
      {
      winLocation = vtkTextRepresentation::UpperRightCorner;
      }
    }

  vtkSMProxy* proxy = this->Internal->Display->getProxy();
  vtkSMIntVectorProperty* pLocation =
    vtkSMIntVectorProperty::SafeDownCast(
    proxy->GetProperty("WindowLocation"));
  if(!pLocation)
    {
    return;
    }
  pLocation->SetElement(0, winLocation);
  proxy->UpdateVTKObjects();

  //Reset the text position according to the PositionX and PositionY box
  if(winLocation == vtkTextRepresentation::AnyLocation)
    {
    proxy->UpdatePropertyInformation();
    vtkSMDoubleVectorProperty* pPosition =
      vtkSMDoubleVectorProperty::SafeDownCast(
      proxy->GetProperty("PositionInfo"));
   
    if(pPosition)
      {
      double *textPos = pPosition->GetElements();
      this->Internal->Position1X->setValue(textPos[0]);
      this->Internal->Position1Y->setValue(textPos[1]);
      }
    }

  this->Internal->Display->renderViewEventually();

}
