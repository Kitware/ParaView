/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
