/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.

*/
#ifndef __PrintUtils_h
#define __PrintUtils_h

//BTX
#include <iostream> // for ostream
#include <map> // for map
#include <vector> // for vector
#include <string> // for string

#include "vtkAMRBox.h"

std::ostream &operator<<(std::ostream &os, const std::map<std::string,int> &m);
std::ostream &operator<<(std::ostream &os, const std::vector<vtkAMRBox> &v);
std::ostream &operator<<(std::ostream &os, const std::vector<std::string> &v);
std::ostream &operator<<(std::ostream &os, const std::vector<double> &v);
std::ostream &operator<<(std::ostream &os, const std::vector<float> &v);
std::ostream &operator<<(std::ostream &os, const std::vector<int> &v);

//ETX
#endif

// VTK-HeaderTest-Exclude: PrintUtils.h
