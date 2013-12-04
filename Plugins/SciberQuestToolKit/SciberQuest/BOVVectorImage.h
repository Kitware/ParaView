/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef BOVVectorImage_h
#define BOVVectorImage_h

#include "BOVScalarImage.h"

#include <iostream> // for ostream
#include <vector> // for vector
#include <string> // for string

#ifdef SQTK_WITHOUT_MPI
typedef void * MPI_Comm;
typedef void * MPI_Info;
typedef void * MPI_File;
#else
#include "SQMPICHWarningSupression.h" // for suppressing MPI warnings
#include <mpi.h> // for MPI_Comm, MPI_Info, and MPI_File
#endif

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
        const char *fileName,
        int mode);

  MPI_File GetComponentFile(int i) const
    {
    return this->ComponentFiles[i]->GetFile();
    }

  void SetNumberOfComponents(int nComps);
  int GetNumberOfComponents() const { return (int)this->ComponentFiles.size(); }

  void SetName(const char *name){ this->Name=name; }
  const char *GetName() const { return this->Name.c_str(); }

private:
  std::string Name;
  std::vector<BOVScalarImage *> ComponentFiles;

private:
  friend std::ostream &operator<<(std::ostream &os, const BOVVectorImage &si);
  const BOVVectorImage &operator=(const BOVVectorImage &other);
  BOVVectorImage(const BOVVectorImage &other);
};

std::ostream &operator<<(std::ostream &os, const BOVVectorImage &vi);

#endif

// VTK-HeaderTest-Exclude: BOVVectorImage.h
