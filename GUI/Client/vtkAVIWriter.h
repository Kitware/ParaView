/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
// .NAME vtkAVIWriter - Writes Windows AVI files.
// .SECTION Description
// vtkAVIWriter writes AVI files. The data type
// of the file is unsigned char regardless of the input type.

// .SECTION See Also
// vtkAVIReader

#ifndef __vtkAVIWriter_h
#define __vtkAVIWriter_h

#include "vtkKWGenericMovieWriter.h"

class vtkAVIWriterInternal;

class VTK_EXPORT vtkAVIWriter : public vtkKWGenericMovieWriter
{
public:
  static vtkAVIWriter *New();
  vtkTypeRevisionMacro(vtkAVIWriter,vtkKWGenericMovieWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // These methods start writing an AVI file, write a frame to the file
  // and then end the writing process.
  void Start();
  void Write();
  void End();
  
protected:
  vtkAVIWriter();
  ~vtkAVIWriter();
  
  vtkAVIWriterInternal *Internals;

  int Rate;
  int Time;
private:
  vtkAVIWriter(const vtkAVIWriter&); // Not implemented
  void operator=(const vtkAVIWriter&); // Not implemented
};

#endif



