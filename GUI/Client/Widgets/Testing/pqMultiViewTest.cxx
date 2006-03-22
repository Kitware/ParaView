

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

  pqMultiView* mv = new pqMultiView;
  mv->resize(400, 300);
  mv->show();
  
  QTextEdit* te1 = new QTextEdit;
  QTextEdit* te2 = new QTextEdit;
  pqMultiView::Index idx;
  delete mv->replaceView(idx, te1);
  idx = mv->splitView(idx, Qt::Horizontal);
  delete mv->replaceView(idx, te2);

  QCoreApplication::processEvents();

  pqMultiViewManager* mv2 = new pqMultiViewManager();
  mv2->resize(600, 400);
  mv2->setObjectName("multiviewtest");
  mv2->show();
  
  QCoreApplication::processEvents();
  
  arguments = handleTestRecording(arguments, *mv2);
  arguments = handleTestCases(arguments, *mv2, quit, error);
  arguments = handleExit(arguments, quit, error);
  
  if(quit)
    return error ? 1 : 0;

  return app.exec();
}

