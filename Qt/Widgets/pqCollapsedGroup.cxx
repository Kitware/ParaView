/*=========================================================================

   Program: ParaView
   Module:    pqCollapsedGroup.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#include "pqCollapsedGroup.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

/* XPM */
static char * expand_xpm[] = {
"9 9 33 1",
"  c None",
". c #FFFFFF",
"+ c #8E997D",
"@ c #FCFCFB",
"# c #FDFDFB",
"$ c #000000",
"% c #FCFCFA",
"& c #F7F6F3",
"* c #F7F7F5",
"= c #F7F7F4",
"- c #F6F6F4",
"; c #F1F0EB",
"> c #E5E1DA",
", c #F5F5F1",
"' c #DFDBD2",
") c #F2F2EE",
"! c #F0F0EC",
"~ c #EDEDE7",
"{ c #EAE9E3",
"] c #E3E0D9",
"^ c #DBD6CC",
"/ c #E4E1D9",
"( c #DCD8CF",
"_ c #D8D3C9",
": c #D6D1C6",
"< c #D2CCC0",
"[ c #CFC8BB",
"} c #D2CCBF",
"| c #C6BEAE",
"1 c #C2B8A8",
"2 c #C1B8A7",
"3 c #C0B7A6",
"4 c #C3BAAA",
".+++++++.",
"+.......+",
"+@##$@%&+",
"+**=$-;>+",
"+,$$$$$'+",
"+)!~${]^+",
"+/(_$:<[+",
"+}|12234+",
".+++++++."};

/* XPM */
static char * collapse_xpm[] = {
"9 9 35 1",
"  c None",
". c #FFFFFF",
"+ c #8E997D",
"@ c #FCFCFB",
"# c #FDFDFB",
"$ c #FCFCFA",
"% c #F7F6F3",
"& c #F7F7F5",
"* c #F7F7F4",
"= c #F6F6F4",
"- c #F1F0EB",
"; c #E5E1DA",
"> c #F5F5F1",
", c #000000",
"' c #DFDBD2",
") c #F2F2EE",
"! c #F0F0EC",
"~ c #EDEDE7",
"{ c #ECEBE6",
"] c #EAE9E3",
"^ c #E3E0D9",
"/ c #DBD6CC",
"( c #E4E1D9",
"_ c #DCD8CF",
": c #D8D3C9",
"< c #D7D2C7",
"[ c #D6D1C6",
"} c #D2CCC0",
"| c #CFC8BB",
"1 c #D2CCBF",
"2 c #C6BEAE",
"3 c #C2B8A8",
"4 c #C1B8A7",
"5 c #C0B7A6",
"6 c #C3BAAA",
".+++++++.",
"+.......+",
"+@###@$%+",
"+&&**=-;+",
"+>,,,,,'+",
"+)!~{]^/+",
"+(_:<[}|+",
"+1234456+",
".+++++++."};

///////////////////////////////////////////////////////////////////////////////
// pqCollapsedGroup::pqImplementation

class pqCollapsedGroup::pqImplementation
{
public:
  pqImplementation(const QString& Name, QWidget* parent_widget) :
    Expanded(true),
    Widget(0),
    Button(Name),
    HideIcon(QPixmap(collapse_xpm)),
    ShowIcon(QPixmap(expand_xpm))
  {
  this->Button.setObjectName("expandCollapse");
  
  this->Button.setIcon(
    this->Expanded ? this->HideIcon : this->ShowIcon);
    
  this->Button.setToolTip(
    this->Expanded ? tr("Collapse Group") : tr("Expand Group"));
  
  this->HLayout.setMargin(0);
  this->HLayout.setSpacing(0);
  this->HLayout.addSpacing(15);
  
  this->VLayout.setMargin(0);
  this->VLayout.setSpacing(0);
  this->VLayout.addWidget(&this->Button);
  this->VLayout.addLayout(&this->HLayout);
  parent_widget->setLayout(&this->VLayout);
  }

  bool Expanded;
  QVBoxLayout VLayout;
  QHBoxLayout HLayout;
  QPushButton Button;
  QWidget* Widget;
  QIcon HideIcon;
  QIcon ShowIcon;
};

pqCollapsedGroup::pqCollapsedGroup(QWidget* parent_widget) :
  QWidget(parent_widget),
  Implementation(new pqImplementation("", this))
{
  this->connect(
    &this->Implementation->Button,
    SIGNAL(clicked()),
    this,
    SLOT(toggle()));
}

pqCollapsedGroup::pqCollapsedGroup(const QString& group_title, QWidget* parent_widget) :
  QWidget(parent_widget),
  Implementation(new pqImplementation(group_title, this))
{
  this->connect(
    &this->Implementation->Button,
    SIGNAL(clicked()),
    this,
    SLOT(toggle()));
}

pqCollapsedGroup::~pqCollapsedGroup()
{
  delete this->Implementation;
}

void pqCollapsedGroup::setWidget(QWidget* child_widget)
{
  if(this->Implementation->Widget)
    {
    this->Implementation->HLayout.removeWidget(
      this->Implementation->Widget);
    }
  
  this->Implementation->Widget = child_widget;
  
  if(this->Implementation->Widget)
    {
    this->Implementation->Widget->setParent(this);
    this->Implementation->HLayout.addWidget(this->Implementation->Widget);
    }
}

const bool pqCollapsedGroup::isExpanded()
{
  return this->Implementation->Expanded;
}

void pqCollapsedGroup::expand()
{
  if(!this->Implementation->Expanded)
    this->toggle();
}

void pqCollapsedGroup::collapse()
{
  if(this->Implementation->Expanded)
    this->toggle();
}

void pqCollapsedGroup::toggle()
{
  this->Implementation->Expanded = 
    !this->Implementation->Expanded;
    
  this->Implementation->Button.setIcon(
    this->Implementation->Expanded ?
      this->Implementation->HideIcon :
      this->Implementation->ShowIcon);

  this->Implementation->Button.setToolTip(
    this->Implementation->Expanded ? tr("Collapse Group") : tr("Expand Group"));
    
  if(this->Implementation->Widget)
    {
    this->Implementation->Widget->setVisible(
      this->Implementation->Expanded);
    }
}
