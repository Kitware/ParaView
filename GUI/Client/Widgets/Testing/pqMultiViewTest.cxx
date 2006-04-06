/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

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

#include <QApplication>
#include <QTextEdit>
#include <QFileDialog>
#include "pqMultiView.h"
#include "pqMultiViewFrame.h"
#include "pqMultiViewManager.h"

#include "pqRecordEventsDialog.h"
#include "pqEventPlayer.h"
#include "pqEventPlayerXML.h"
#include "vtkIOStream.h"
#include "vtkstd/algorithm"

typedef vtkstd::vector<vtkstd::string> arguments_t;

/// Parses command-line arguments for the --run-test flag, returning unused arguments
const arguments_t handleTestCases(const arguments_t& Arguments, QObject& RootObject, bool& Quit, bool& Error)
{
  arguments_t unused;
  
  for(arguments_t::const_iterator argument = Arguments.begin(); argument != Arguments.end(); ++argument)
    {
    if(*argument == "--run-test" && ++argument != Arguments.end())
      {
      pqEventPlayer player(RootObject);
      player.addDefaultWidgetEventPlayers();

      pqEventPlayerXML xml_player;
      if(!xml_player.playXML(player, argument->c_str()))
        {
        Quit = true;
        Error = true;
        }
      
      continue;
      }

    unused.push_back(*argument);
    }

  return unused;
}

const arguments_t handleTestRecording(const arguments_t& Arguments, QWidget& RootObject)
{
  arguments_t unused;
  
  for(arguments_t::const_iterator argument = Arguments.begin(); argument != Arguments.end(); ++argument)
    {
    if(*argument == "--record-test")
      {
      QString file = QFileDialog::getSaveFileName();
      pqRecordEventsDialog* dialog = new pqRecordEventsDialog(file, &RootObject);
      dialog->show();
      continue;
      }

    unused.push_back(*argument);
    }

  return unused;
}

/// Parses command-line arguments for the --exit flag, returning unused arguments
const arguments_t handleExit(arguments_t& Arguments, bool& Quit, bool&)
{
  if(vtkstd::count(Arguments.begin(), Arguments.end(), "--exit"))
    {
    Arguments.erase(vtkstd::remove(Arguments.begin(), Arguments.end(), "--exit"), Arguments.end());
    Quit = true;
    }
    
  return Arguments;
}


int main(int argc, char** argv)
{
  arguments_t arguments(argv+1, argv+argc);
  bool quit = false;
  bool error = false;

  QApplication app(argc, argv);
  Q_INIT_RESOURCE(pqMultiViewTest);

  pqMultiView mv;
  mv.resize(400, 300);
  mv.show();
  
  QTextEdit* te1 = new QTextEdit;
  QTextEdit* te2 = new QTextEdit;
  pqMultiView::Index idx;
  delete mv.replaceView(idx, te1);
  idx = mv.splitView(idx, Qt::Horizontal);
  delete mv.replaceView(idx, te2);

  QCoreApplication::processEvents();

  pqMultiViewManager mv2;
  mv2.resize(600, 400);
  mv2.setObjectName("multiviewtest");
  mv2.show();
  
  QCoreApplication::processEvents();
  
  arguments = handleTestRecording(arguments, mv2);
  arguments = handleTestCases(arguments, mv2, quit, error);
  arguments = handleExit(arguments, quit, error);
  
  if(quit)
    return error ? 1 : 0;

  return app.exec();
}

