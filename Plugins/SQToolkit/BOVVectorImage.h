/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef BOVVectorImage_h
#define BOVVectorImage_h

#include "BOVScalarImage.h"

#include <iostream>
using std::ostream;
using std::endl;

#include <vector>
using std::vector;

#include <string>
using std::string;

/// Handle to the files comprising a multi-component vector.
class BOVVectorImage
{
public:
  BOVVectorImage(){};
  ~BOVVectorImage();

  void Clear();

  void SetComponentFile(
        int i,
        MPI_Comm comm,
        MPI_Info hints,
        const char *fileName);

  MPI_File GetComponentFile(int i) const
    {
    return this->ComponentFiles[i]->GetFile();
    }

  void SetNumberOfComponents(int nComps);
  int GetNumberOfComponents() const { return this->ComponentFiles.size(); }

  void SetName(const char *name){ this->Name=name; }
  const char *GetName() const { return this->Name.c_str(); }

private:
  string Name;
  vector<BOVScalarImage *> ComponentFiles;

private:
  friend ostream &operator<<(ostream &os, const BOVVectorImage &si);
  const BOVVectorImage &operator=(const BOVVectorImage &other);
  BOVVectorImage(const BOVVectorImage &other);
};

ostream &operator<<(ostream &os, const BOVVectorImage &vi);

#endif
