/*=========================================================================

   Program: ParaView
   Module:    pqNamedWidgets.h

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

#ifndef _pqNamedWidgets_h
#define _pqNamedWidgets_h

#include "pqSMProxy.h"

#include "pqComponentsExport.h"

class QWidget;
class QGridLayout;
class pqPropertyManager;
class QStringList;

/** Links Qt widgets with server manager properties by name.

A Qt widget will be considered a "match" for a property if its name is
any one of the forms:

PropertyName
PropertyName:String
PropertyName:Digit
PropertyName:String:Digit

... where PropertyName is the name of the server manager property in question.

The first form is the most used, when there is a single Qt widget associated
with a property.

The second form is used when more than one Qt widget will be associated
with a single property.  An example would be a property controlled by
a slider widget plus a spin box.  In this case the two widgets might be named
"Foo:Slider" and "Foo:Spin".  The choice of identifier for individual widgets
is not significant, as long as it is unique.

The third and fourth forms are used when Qt widgets will be associated with
multi-element properties.  The digits are used as indices to control with
which property element a given widget will be associated.

If widgets can also represent domains (combo boxes has a fixed list of items,
sliders or spin boxes have a minimum or maximum value), their domains will be
linked as well so if the domain changes, the possible values or
minimums/maximums will automatically updated.
*/
class PQCOMPONENTS_EXPORT pqNamedWidgets
{
public:
  /// populate a grid layout with widgets to represent the properties
  static void createWidgets(QGridLayout* l, vtkSMProxy* pxy, bool summaryOnly = false);

  /// Link a collection of Qt child widgets with server manager properties by name
  static void link(QWidget* parent, pqSMProxy proxy, pqPropertyManager* property_manager,
                         const QStringList* exceptions=NULL);
  /// Remove links between Qt widgets and server manager properties
  static void unlink(QWidget* parent, pqSMProxy proxy, pqPropertyManager* property_manager);

  static void linkObject(QObject* object, pqSMProxy proxy, 
                         const QString& property,
                         pqPropertyManager* property_manager);
  
  static void unlinkObject(QObject* object, pqSMProxy proxy,
                           const QString& property,
                           pqPropertyManager* property_manager);

  /// given an object, find the user property and its associated signal
  /// this is used to find which property of a widget to link with
  /// returns whether it was found
  /// for QCheckBox and QTextEdit, the signal names not derived from the user
  /// property
  static bool propertyInformation(QObject* object, 
    QString& property, QString& signal);

  /// this function does the actual linking, and adds a range domain if one is
  /// needed
  static void linkObject(QObject* o, const QString& property,
                         const QString& signal, pqSMProxy proxy,
                         vtkSMProperty* smProperty, int index,
                         pqPropertyManager* pm);
  
  /// this function does the actual un-linking, and removes a range domain if one
  /// exists
  static void unlinkObject(QObject* o, const QString& property,
                         const QString& signal, pqSMProxy proxy,
                         vtkSMProperty* smProperty, int index,
                         pqPropertyManager* pm);
};

#endif

