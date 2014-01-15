/*=========================================================================

   Program: ParaView
   Module:    pqPropertyGroupWidget.h

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
#ifndef __pqPropertyGroupWidget_h
#define __pqPropertyGroupWidget_h

#include "pqPropertyWidget.h"

class QCheckBox;
class QDoubleSpinBox;
class QGroupBox;
class QLineEdit;
class QSpinBox;
class QWidget;
class pqColorChooserButton;
class vtkSMProxy;
class vtkSMPropertyGroup;



class PQCOMPONENTS_EXPORT pqPropertyGroupWidget : public pqPropertyWidget
{
  Q_OBJECT
    typedef pqPropertyWidget Superclass;
 public:
  pqPropertyGroupWidget(vtkSMProxy *proxy,
                        vtkSMPropertyGroup* smGroup, QWidget *parent=0);
  vtkSMPropertyGroup* getPropertyGroup () const
  {
    return this->PropertyGroup;
  }

 protected:
  using pqPropertyWidget::addPropertyLink;
  void addPropertyLink(QCheckBox* button, const char* propertyName);
  void addPropertyLink(QGroupBox* groupBox, const char* propertyName);
  void addPropertyLink(QDoubleSpinBox* spinBox, const char* propertyName);
  void addPropertyLink(QSpinBox* spinBox, const char* propertyName);
  void addPropertyLink(pqColorChooserButton* color, const char* propertyName);

 private:
  void addCheckedPropertyLink(QWidget* button, const char* propertyName);
  void addDoubleValuePropertyLink(QWidget* widget, const char* propertyName);
  void addIntValuePropertyLink(QWidget* widget, const char* propertyName);

 private:
  vtkSMPropertyGroup* PropertyGroup;
};

#endif
