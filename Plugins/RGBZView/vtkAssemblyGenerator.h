/*=========================================================================

   Program: ParaView
   Module:  vtkAssemblyGenerator.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
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

#ifndef __vtkAssemblyGenerator_h
#define __vtkAssemblyGenerator_h

#include "vtkObject.h"

class vtkAssemblyGenerator : public vtkObject
{
public:
  static vtkAssemblyGenerator* New();
  vtkTypeMacro(vtkAssemblyGenerator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Adds names of files to be read. The files are read in the order
  // they are added.
  virtual void AddFileName(const char* fname);

  // Description:
  // Remove all file names.
  virtual void RemoveAllFileNames();

  // Description:
  // Returns the number of file names added by AddFileName.
  virtual unsigned int GetNumberOfFileNames();

  // Description:
  // Returns the name of a file with index idx.
  virtual const char* GetFileName(unsigned int idx);

  // Description:
  // Set output directory to write to the assembly descriptor and data
  vtkSetStringMacro(DestinationDirectory);
  vtkGetStringMacro(DestinationDirectory);

  // Description:
  // Execute assembly analysis
  virtual void Write();

//BTX
protected:
  vtkAssemblyGenerator();
  ~vtkAssemblyGenerator();

private:
  vtkAssemblyGenerator(const vtkAssemblyGenerator&); // Not implemented
  void operator=(const vtkAssemblyGenerator&); // Not implemented

  char* DestinationDirectory;
  struct vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif
