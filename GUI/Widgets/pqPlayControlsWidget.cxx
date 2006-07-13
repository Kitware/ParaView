/*=========================================================================

   Program: ParaView
   Module:    pqPlayControlsWidget.cxx

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

#include "pqPlayControlsWidget.h"

#include <QFrame>
#include <QIcon>
#include <QLayout>
#include <QToolButton>


const char *pqPlayControlsWidget::Name[NUM_BUTTONS] = {
  "First",
  "Back",
  "Play",
  "Pause",
  "Forward",
  "Last"
};
const char *pqPlayControlsWidget::Image[NUM_BUTTONS] = {
  ":pqWidgets/pqVcrFirst22.png",
  ":pqWidgets/pqVcrBack22.png",
  ":pqWidgets/pqVcrPlay22.png",
  ":pqWidgets/pqVcrPause22.png",
  ":pqWidgets/pqVcrForward22.png",
  ":pqWidgets/pqVcrLast22.png"  
};

pqPlayControlsWidget::pqPlayControlsWidget(QWidget *_parent)
  : QWidget( _parent)
{

  this->Layout = new QHBoxLayout( this );
  this->Layout->setMargin(0);
  this->Layout->setSpacing(0);

  for ( int i=0;i<pqPlayControlsWidget::NUM_BUTTONS;i++ )
    {
    this->Button[i] = new QToolButton( this ); 
    this->Button[i]->setAutoRaise( true );
    this->Button[i]->setIconSize( QSize(22,22) );
    this->Button[i]->setIcon( QIcon(pqPlayControlsWidget::Image[i]) );
    this->Button[i]->setToolTip(pqPlayControlsWidget::Name[i]);
    this->Button[i]->setStatusTip(pqPlayControlsWidget::Name[i]);
    this->Button[i]->setObjectName(pqPlayControlsWidget::Name[i]);
    this->Layout->addWidget( this->Button[i] );
    }

  connect( this->Button[FIRST], SIGNAL( clicked() ), 
      SIGNAL( first() ) ); 
  connect( this->Button[LAST], SIGNAL( clicked() ), 
      SIGNAL( last() ) );
  connect( this->Button[PLAY], SIGNAL( clicked() ), 
      SIGNAL( play() ) ); 
  connect( this->Button[PAUSE], SIGNAL( clicked() ), 
      SIGNAL( pause() ) );  
  connect( this->Button[FORWARD], SIGNAL( clicked() ), 
      SIGNAL( forward() ) ); 
  connect( this->Button[BACK], SIGNAL( clicked() ), 
      SIGNAL( back() ) ); 

}

pqPlayControlsWidget::~pqPlayControlsWidget()
{
  for ( int i=0;i<pqPlayControlsWidget::NUM_BUTTONS;i++ )
    {
    if ( this->Button[i] )
      {
      delete this->Button[i];
      this->Button[i] = NULL;
      }
    }
  if ( this->Layout )
    {
    delete this->Layout;
    this->Layout = NULL;
    }
}

