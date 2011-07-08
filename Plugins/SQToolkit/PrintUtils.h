/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef __PrintUtils_h
#define __PrintUtils_h

//BTX
#include <iostream>
using std::ostream;
#include <map>
using std::map;
#include <vector>
using std::vector;
#include <string>
using std::string;

#include "vtkAMRBox.h"

ostream &operator<<(ostream &os, const map<string,int> &m);
ostream &operator<<(ostream &os, const vector<vtkAMRBox> &v);
ostream &operator<<(ostream &os, const vector<string> &v);
ostream &operator<<(ostream &os, const vector<double> &v);
ostream &operator<<(ostream &os, const vector<float> &v);
ostream &operator<<(ostream &os, const vector<int> &v);

//ETX
#endif
