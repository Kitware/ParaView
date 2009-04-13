#ifndef PrintUtils
#define PrintUtils

#include "vtkIOStream.h"
#include <vtkstd/vector>
using vtkstd::vector;
#include <vtkstd/string>
using vtkstd::string;

ostream &operator<<(ostream &os, vector<string> v);
ostream &operator<<(ostream &os, vector<double> v);
ostream &operator<<(ostream &os, vector<int> v);
ostream &PrintD3(ostream &os, const double *v3);
ostream &PrintD6(ostream &os, const double *v6);

#endif
