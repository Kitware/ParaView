// -*- c++ -*-
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

#include <vtkToolkits.h>
#include "MainWindow.h"
#include <QApplication>

#ifdef VTK_USE_MPI
# include <mpi.h>
#endif

#include <pqEventPlayer.h>
#include <pqEventPlayerXML.h>

#include <QApplication>
#include <vtkIOStream.h>

#include <algorithm>

/// Stores a threshold for test-case image comparison
double g_image_threshold = 10.0;
/// Stores a temp directory to receive test-case output images
QString g_test_directory = ".";

/// Defines storage for command-line arguments
typedef vtkstd::vector<vtkstd::string> arguments_t;

/// Displays command-line usage information
void printUsage()
{
  cerr << "Usage: pqclient [options]\n\n";
  cerr << "Options:\n\n";
  cerr << "--help                         Print usage information and exit\n";
  cerr << "--compare-view <image>         Compare the viewport to a reference image, and exit\n";
  cerr << "--test-directory <path>        Set the temporary directory where test-case output will be stored\n";
  cerr << "--image-threshold <threshold>  Set the threshold beyond which viewport-image comparisons fail\n";
  cerr << "--run-test <test>              Run a recorded test case\n";
  cerr << "--version                      Print version information and exit\n";
}

/// Parses command-line arguments for the --version flag, returning unused arguments
const arguments_t handleVersion(arguments_t& Arguments, bool& Quit, bool& /*Error*/)
{
  if(vtkstd::count(Arguments.begin(), Arguments.end(), "--version"))
    {
    Arguments.erase(vtkstd::remove(Arguments.begin(), Arguments.end(), "--version"), Arguments.end());
    cout << "ParaQ Qt Client version " << VERSION << "\n";
    Quit = true;
    }
    
  return Arguments;
}

/// Parses command-line arguments for the --help flag, returning unused arguments
const arguments_t handleHelp(arguments_t& Arguments, bool& Quit, bool& /*Error*/)
{
  if(vtkstd::count(Arguments.begin(), Arguments.end(), "--help"))
    {
    Arguments.erase(vtkstd::remove(Arguments.begin(), Arguments.end(), "--help"), Arguments.end());
    printUsage();
    Quit = true;
    }
    
  return Arguments;
}

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

/// Parses command-line arguments for the --temp-directory flag, returning unused arguments
const arguments_t handleTestDirectory(const arguments_t& Arguments, bool& /*Quit*/, bool& /*Error*/)
{
  arguments_t unused;
  
  for(arguments_t::const_iterator argument = Arguments.begin(); argument != Arguments.end(); ++argument)
    {
    if(*argument == "--test-directory" && ++argument != Arguments.end())
      {
      g_test_directory = argument->c_str();
      continue;
      }

    unused.push_back(*argument);
    }

  return unused;
}

/// Parses command-line arguments for the --image-threshold flag, returning unused arguments
const arguments_t handleImageThreshold(const arguments_t& Arguments, bool& /*Quit*/, bool& /*Error*/)
{
  arguments_t unused;
  
  for(arguments_t::const_iterator argument = Arguments.begin(); argument != Arguments.end(); ++argument)
    {
    if(*argument == "--image-threshold" && ++argument != Arguments.end())
      {
      g_image_threshold = atof(argument->c_str());
      continue;
      }

    unused.push_back(*argument);
    }

  return unused;
}

/// Parses command-line arguments for the --compare-view flag, returning unused arguments
const arguments_t handleCompareView(const arguments_t& Arguments, MainWindow& MainWindow, bool& Quit, bool& Error)
{
  arguments_t unused;
  
  for(arguments_t::const_iterator argument = Arguments.begin(); argument != Arguments.end(); ++argument)
    {
    if(*argument == "--compare-view" && ++argument != Arguments.end())
      {
      Quit = true;
      Error = !MainWindow.compareView(argument->c_str(), g_image_threshold, cout, g_test_directory);
      continue;
      }

    unused.push_back(*argument);
    }

  return unused;
}

/// Parses command-line arguments for the --exit flag, returning unused arguments
const arguments_t handleExit(arguments_t& Arguments, bool& Quit, bool& /*Error*/)
{
  if(vtkstd::count(Arguments.begin(), Arguments.end(), "--exit"))
    {
    Arguments.erase(vtkstd::remove(Arguments.begin(), Arguments.end(), "--exit"), Arguments.end());
    Quit = true;
    }
    
  return Arguments;
}

/// Handles unknown command-line arguments
void handleUnusedArguments(const arguments_t& Arguments, bool& Quit, bool& Error)
{
   for(arguments_t::const_iterator argument = Arguments.begin(); argument != Arguments.end(); ++argument)
      {
      cerr << "Unknown argument: " << *argument << "\n";
      }
      
    if(Arguments.size())
      {
      Quit = true;
      Error = true;
      }
}

int main(int argc, char* argv[])
{
#ifdef VTK_USE_MPI
  int myId =0;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD,&myId);
#endif

  // Create the main window
  QApplication application(argc, argv);

  bool quit = false;
  bool error = false;

  arguments_t arguments(argv+1, argv+argc);
  
  // Handle command-line arguments that cause an immediate exit
  arguments = handleVersion(arguments, quit, error);
  arguments = handleHelp(arguments, quit, error);
    
  if(quit)
    return error ? 1 : 0;  

/** \todo Figure-out how to export Qt's resource symbols from a DLL, so we can use them here */
#if !(defined(WIN32) && defined(PARAQ_BUILD_SHARED_LIBS))
  Q_INIT_RESOURCE(pqWidgets);
#endif
  
  MainWindow main_window;
  main_window.resize(800, 600);
  main_window.show();

  // Run test cases
  arguments = handleTestCases(arguments, main_window, quit, error);
  // Set the temp directory for test case image comparison
  arguments = handleTestDirectory(arguments, quit, error);
  // Set the image threshold value
  arguments = handleImageThreshold(arguments, quit, error);
  // Perform image comparisons
  arguments = handleCompareView(arguments, main_window, quit, error);
  // Exit the application if requested
  arguments = handleExit(arguments, quit, error);
  // Check for unused (i.e. unknown) arguments
  handleUnusedArguments(arguments, quit, error);

  if(quit)
    return error ? 1 : 0;

  // Main GUI event loop
  int ret = application.exec();

#ifdef VTK_USE_MPI
  MPI_Finalize();
#endif

  return ret;
}

