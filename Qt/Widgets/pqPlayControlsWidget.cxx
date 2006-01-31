/*=========================================================================

   Program:   ParaQ
   Module:    pqPlayControlsWidget.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include "Resources/First.xpm"
#include "Resources/Last.xpm"
#include "Resources/Forward.xpm"
#include "Resources/Back.xpm"
//#include "Resources/Play.xpm"
//#include "Resources/Pause.xpm"

const char *pqPlayControlsWidget::Name[NUM_BUTTONS] = {
  "First",
  "Back",
  "Forward",
  "Last",
//  "Pause",
//  "Play"
};
char **pqPlayControlsWidget::Image[NUM_BUTTONS] = {
  First,
  Back,
  Forward,
  Last,
//  Pause,
//  Play
};

pqPlayControlsWidget::pqPlayControlsWidget(QWidget *parent)
  : QWidget( parent)
{
  // this->Frame = new QFrame( this, "pqPlayControlsWidget" );
  // this->Frame->setEnabled( TRUE );
  // this->Frame->setFrameStyle( QFrame::Box | QFrame::Plain );
  // this->Frame->setFrameShape( QFrame::Box );
  // this->Frame->setFrameShadow( QFrame::Plain );
  // this->Frame->setLineWidth( 1 );
  // this->Frame->setFocusPolicy( QWidget::NoFocus );

  this->Layout = new QHBoxLayout( this );
  this->Layout->setMargin(0);
  this->Layout->setSpacing(0);

  int max_dim = 15;
  for ( int i=0;i<pqPlayControlsWidget::NUM_BUTTONS;i++ )
    {
    this->Pixmap[i] = new QPixmap( 
      const_cast<const char **>( pqPlayControlsWidget::Image[i] ) );
    this->Button[i] = new QToolButton( this ); 
    this->Button[i]->setAutoRaise( true );
    QSizePolicy sizePolicy = this->Button[i]->sizePolicy();
    sizePolicy.setHorizontalPolicy(QSizePolicy::Minimum);
    sizePolicy.setVerticalPolicy(QSizePolicy::Minimum);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    this->Button[i]->setSizePolicy(sizePolicy);
    this->Button[i]->setMaximumSize( QSize( max_dim, max_dim ) );
    // this->Button[i]->setText( tr( "..." ) );
    // this->Button[i]->setFocusPolicy( QWidget::NoFocus );
    this->Button[i]->setIcon( QIcon(*(this->Pixmap[i])) );
    this->Button[i]->setToolTip(pqPlayControlsWidget::Name[i]);
    this->Button[i]->setStatusTip(pqPlayControlsWidget::Name[i]);
    this->Layout->addWidget( this->Button[i] );
    }

  connect( this->Button[FIRST], SIGNAL( clicked() ), 
      SIGNAL( first() ) ); 
  connect( this->Button[LAST], SIGNAL( clicked() ), 
      SIGNAL( last() ) ); 
  connect( this->Button[FORWARD], SIGNAL( clicked() ), 
      SIGNAL( forward() ) ); 
  connect( this->Button[BACK], SIGNAL( clicked() ), 
      SIGNAL( back() ) ); 
/*
  connect( this->Button[PLAY], SIGNAL( clicked() ), 
      SIGNAL( play() ) ); 
  connect( this->Button[PAUSE], SIGNAL( clicked() ), 
      SIGNAL( pause() ) ); 
*/
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
    if ( this->Pixmap[i] )
      {
      delete this->Pixmap[i];
      this->Pixmap[i] = NULL;
      }
    }
  if ( this->Layout )
    {
    delete this->Layout;
    this->Layout = NULL;
    }
}

