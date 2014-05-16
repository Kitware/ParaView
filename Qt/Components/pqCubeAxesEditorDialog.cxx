/*=========================================================================

   Program: ParaView
   Module:    pqCubeAxesEditorDialog.cxx

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
#include "pqCubeAxesEditorDialog.h"
#include "ui_pqCubeAxesEditorDialog.h"

// Server Manager Includes.
#include "vtkSmartPointer.h"
#include "vtkSMProxy.h"

// Qt Includes.
#include <QDoubleValidator>
#include <QPointer>

// ParaView Includes.
#include "pqApplicationCore.h"
#include "pqNamedWidgets.h"
#include "pqPropertyManager.h"
#include "pqUndoStack.h"

#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"

class pqCubeAxesEditorDialog::pqInternal : public Ui::CubeAxesEditorDialog
{
public:
  vtkSmartPointer<vtkSMProxy> Representation;
  pqPropertyManager* PropertyManager;
  QPointer<pqColorPaletteLinkHelper> PaletteLinkHelper;
  pqInternal()
    {
    this->PropertyManager = 0;
    }
  ~pqInternal()
    {
    delete this->PropertyManager;
    this->PropertyManager = 0;
    delete this->PaletteLinkHelper;
    }
};

//-----------------------------------------------------------------------------
pqCubeAxesEditorDialog::pqCubeAxesEditorDialog(
  QWidget *_parent/*=0*/, Qt::WindowFlags f/*=0*/):
  Superclass(_parent, f)
{
  this->Internal = new pqInternal();
  this->Internal->setupUi(this);
  QObject::connect(this->Internal->Ok, SIGNAL(clicked()),
    this, SLOT(accept()), Qt::QueuedConnection);
  QObject::connect(this->Internal->Cancel, SIGNAL(clicked()),
    this, SLOT(reject()), Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------
pqCubeAxesEditorDialog::~pqCubeAxesEditorDialog()
{
  delete this->Internal;
}



#define PV_LINEEDIT_REGISTER(ui, propName, index)\
{\
  this->Internal->PropertyManager->registerLink(\
    this->Internal->ui, "text",SIGNAL(textChanged(const QString &)),\
    repr, repr->GetProperty(propName), index);\
}

#define PV_GROUPBOX_REGISTER(ui, propName, index)\
{\
  this->Internal->PropertyManager->registerLink(\
    this->Internal->ui, "checked",SIGNAL(toggled(bool)),\
    repr, repr->GetProperty(propName), index);\
}
//-----------------------------------------------------------------------------
void pqCubeAxesEditorDialog::setRepresentationProxy(vtkSMProxy* repr)
{
  if (this->Internal->Representation == repr)
    {
    return;
    }
  delete this->Internal->PropertyManager;
  this->Internal->PropertyManager = new pqPropertyManager(this);
  this->Internal->Representation = repr;
  delete this->Internal->PaletteLinkHelper;

  if (repr)
    {
    // set up links between the property manager and the widgets.
    pqNamedWidgets::link(this, repr, this->Internal->PropertyManager);
    this->Internal->PropertyManager->registerLink(
      this->Internal->Color,
      "chosenColorRgbF",  SIGNAL(chosenColorChanged(const QColor&)),
      repr, repr->GetProperty("CubeAxesColor"));
    new pqColorPaletteLinkHelper(
      this->Internal->Color, repr, "CubeAxesColor");

    //fill the ui elements with the correct object bounds
    double pvBounds[6];
    int axesOriginSelected = 0;
    vtkSMPVRepresentationProxy *pvProxy = vtkSMPVRepresentationProxy::SafeDownCast(repr);
    if ( pvProxy )
      {
      //link the ui elements to the vtkSMCubeAxesRepresentationProxy
      PV_LINEEDIT_REGISTER(CubeAxesXCustomBoundsMin, "CustomBounds", 0);
      PV_LINEEDIT_REGISTER(CubeAxesXCustomBoundsMax, "CustomBounds", 1);
      PV_LINEEDIT_REGISTER(CubeAxesYCustomBoundsMin, "CustomBounds", 2);
      PV_LINEEDIT_REGISTER(CubeAxesYCustomBoundsMax, "CustomBounds", 3);
      PV_LINEEDIT_REGISTER(CubeAxesZCustomBoundsMin, "CustomBounds", 4);
      PV_LINEEDIT_REGISTER(CubeAxesZCustomBoundsMax, "CustomBounds", 5);

      //link the activation of the group boxes to vtkSMCubeAxesRepresentationProxy
      PV_GROUPBOX_REGISTER(CubeAxesXCustomBounds, "CustomBoundsActive", 0);
      PV_GROUPBOX_REGISTER(CubeAxesYCustomBounds, "CustomBoundsActive", 1);
      PV_GROUPBOX_REGISTER(CubeAxesZCustomBounds, "CustomBoundsActive", 2);

      //link the ui elements to the vtkSMCubeAxesRepresentationProxy
      PV_LINEEDIT_REGISTER(CubeAxesXCustomRangeMin, "CustomRange", 0);
      PV_LINEEDIT_REGISTER(CubeAxesXCustomRangeMax, "CustomRange", 1);
      PV_LINEEDIT_REGISTER(CubeAxesYCustomRangeMin, "CustomRange", 2);
      PV_LINEEDIT_REGISTER(CubeAxesYCustomRangeMax, "CustomRange", 3);
      PV_LINEEDIT_REGISTER(CubeAxesZCustomRangeMin, "CustomRange", 4);
      PV_LINEEDIT_REGISTER(CubeAxesZCustomRangeMax, "CustomRange", 5);

      //link the activation of the group boxes to vtkSMCubeAxesRepresentationProxy
      PV_GROUPBOX_REGISTER(CubeAxesXCustomRange, "CustomRangeActive", 0);
      PV_GROUPBOX_REGISTER(CubeAxesYCustomRange, "CustomRangeActive", 1);
      PV_GROUPBOX_REGISTER(CubeAxesZCustomRange, "CustomRangeActive", 2);

      //link the activation of the group boxes to vtkSMCubeAxesRepresentationProxy
      PV_GROUPBOX_REGISTER(UseOriginalBoundsRangeForX, "OriginalBoundsRangeActive", 0);
      PV_GROUPBOX_REGISTER(UseOriginalBoundsRangeForY, "OriginalBoundsRangeActive", 1);
      PV_GROUPBOX_REGISTER(UseOriginalBoundsRangeForZ, "OriginalBoundsRangeActive", 2);

      //link the ui elements to the vtkSMCubeAxesRepresentationProxy
      PV_LINEEDIT_REGISTER(AxesOriginX, "AxesOrigin", 0);
      PV_LINEEDIT_REGISTER(AxesOriginY, "AxesOrigin", 1);
      PV_LINEEDIT_REGISTER(AxesOriginZ, "AxesOrigin", 2);

      // Link Axis origin checkbox
      PV_GROUPBOX_REGISTER(UseAxesOrigin, "UseAxesOrigin", 0);
      QObject::connect( this->Internal->UseAxesOrigin, SIGNAL(toggled(bool)),
                        this, SLOT(onUseAxesOriginChange(bool)));

      //now they are linked, set them to objects bounds.
      vtkSMPropertyHelper(repr,"DataBounds").UpdateValueFromServer();
      vtkSMPropertyHelper(repr,"DataBounds").Get(pvBounds,6);
      this->setupCustomAxes( pvBounds[0],pvBounds[1],
        !this->Internal->CubeAxesXCustomBounds->isChecked(),
        this->Internal->CubeAxesXCustomBoundsMin,
        this->Internal->CubeAxesXCustomBoundsMax);

      this->setupCustomAxes( pvBounds[2], pvBounds[3],
        !this->Internal->CubeAxesYCustomBounds->isChecked(),
        this->Internal->CubeAxesYCustomBoundsMin,
        this->Internal->CubeAxesYCustomBoundsMax);

      this->setupCustomAxes( pvBounds[4], pvBounds[5],
        !this->Internal->CubeAxesZCustomBounds->isChecked(),
        this->Internal->CubeAxesZCustomBoundsMin,
        this->Internal->CubeAxesZCustomBoundsMax);

      this->setupCustomAxes( pvBounds[0],pvBounds[1],
        !this->Internal->CubeAxesXCustomRange->isChecked(),
        this->Internal->CubeAxesXCustomRangeMin,
        this->Internal->CubeAxesXCustomRangeMax);

      this->setupCustomAxes( pvBounds[2], pvBounds[3],
        !this->Internal->CubeAxesYCustomRange->isChecked(),
        this->Internal->CubeAxesYCustomRangeMin,
        this->Internal->CubeAxesYCustomRangeMax);

      this->setupCustomAxes( pvBounds[4], pvBounds[5],
        !this->Internal->CubeAxesZCustomRange->isChecked(),
        this->Internal->CubeAxesZCustomRangeMin,
        this->Internal->CubeAxesZCustomRangeMax);

      // Update UI base on Axis Origin usage
      vtkSMPropertyHelper(repr,"AxesOrigin").Get(pvBounds,3);
      this->setupCustomAxes( pvBounds[0], pvBounds[1], true,
                             this->Internal->AxesOriginX, this->Internal->AxesOriginY);
      this->setupCustomAxes( pvBounds[2], pvBounds[2], true,
                             this->Internal->AxesOriginZ, this->Internal->AxesOriginZ);
      vtkSMPropertyHelper(repr,"UseAxesOrigin").Get(&axesOriginSelected, 1);
      this->onUseAxesOriginChange( (axesOriginSelected == 1) ? true : false );

      // Link sticky axes option
      PV_GROUPBOX_REGISTER(StickyAxes, "StickyAxes", 0);
      QObject::connect( this->Internal->StickyAxes, SIGNAL(toggled(bool)),
                        this, SLOT( onStickyAxesChange(bool)));
      int stickyAxesSelected = 0;
      vtkSMPropertyHelper(repr,"StickyAxes").Get(&stickyAxesSelected, 1);
      this->onStickyAxesChange( (stickyAxesSelected == 1) ? true : false );

      PV_GROUPBOX_REGISTER(CenterStickyAxes, "CenterStickyAxes", 0);
      }
    }
}

#undef PV_LINEEDIT_REGISTER
#undef PV_GROUPBOX_REGISTER

//-----------------------------------------------------------------------------
void pqCubeAxesEditorDialog::setupCustomAxes( const double &min, const double &max,
    const bool &enabled, QLineEdit *minWidget, QLineEdit *maxWidget)
{
  //setup the validator for this axes]
  if (minWidget->validator() == NULL )
    {
    minWidget->setValidator(new QDoubleValidator(minWidget));
    }
  if (maxWidget->validator() == NULL )
    {
    maxWidget->setValidator(new QDoubleValidator(maxWidget));
    }

  //setup initial values
  if ( enabled )
    {
    minWidget->setText( QString::number(min) );
    maxWidget->setText( QString::number(max) );
    }
}


//-----------------------------------------------------------------------------
void pqCubeAxesEditorDialog::done(int res)
{
  if (res == QDialog::Accepted && this->Internal->PropertyManager->isModified())
    {
    BEGIN_UNDO_SET("Cube Axes Parameters");
    this->Internal->PropertyManager->accept();
    END_UNDO_SET();
    pqApplicationCore::instance()->render();
    }
  this->Superclass::done(res);
}

//-----------------------------------------------------------------------------
void pqCubeAxesEditorDialog::onUseAxesOriginChange(bool enableAxesOrigin)
{
  this->Internal->AxesOriginX->setEnabled(enableAxesOrigin);
  this->Internal->AxesOriginY->setEnabled(enableAxesOrigin);
  this->Internal->AxesOriginZ->setEnabled(enableAxesOrigin);
  if(enableAxesOrigin)
    {
    this->Internal->CubeAxesXGridLines->setChecked(false);
    this->Internal->CubeAxesYGridLines->setChecked(false);
    this->Internal->CubeAxesZGridLines->setChecked(false);
    this->Internal->CubeAxesXGridLines->setEnabled(false);
    this->Internal->CubeAxesYGridLines->setEnabled(false);
    this->Internal->CubeAxesZGridLines->setEnabled(false);
    }
  else
    {
    this->Internal->CubeAxesXGridLines->setEnabled(true);
    this->Internal->CubeAxesYGridLines->setEnabled(true);
    this->Internal->CubeAxesZGridLines->setEnabled(true);
    }
}

//-----------------------------------------------------------------------------
void pqCubeAxesEditorDialog::onStickyAxesChange(bool enableStickyAxes)
{
  this->Internal->CenterStickyAxes->setEnabled(enableStickyAxes);
}
