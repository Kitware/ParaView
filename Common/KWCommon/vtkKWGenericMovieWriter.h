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
// .NAME vtkKWGenericMovieWriter - Writes Windows Movie files.
// .SECTION Description
// vtkKWGenericMovieWriter writes Movie files. The data type
// of the file is unsigned char regardless of the input type.

// .SECTION See Also
// vtkMovieReader

#ifndef __vtkKWGenericMovieWriter_h
#define __vtkKWGenericMovieWriter_h

#include "vtkProcessObject.h"

class vtkImageData;
class vtkKWGenericMovieWriterInternal;

class VTK_EXPORT vtkKWGenericMovieWriter : public vtkProcessObject
{
public:
  vtkTypeRevisionMacro(vtkKWGenericMovieWriter,vtkProcessObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the input object from the image pipeline.
  virtual void SetInput(vtkImageData *input);
  virtual vtkImageData *GetInput();

  // Description:
  // Specify file name of avi file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // These methods start writing an Movie file, write a frame to the file
  // and then end the writing process.
  virtual void Start() =0;
  virtual void Write() =0;
  virtual void End() =0;
  
  // Description:
  // Was there an error on the last read performed?
  vtkGetMacro(Error,int);

protected:
  vtkKWGenericMovieWriter();
  ~vtkKWGenericMovieWriter();

  char *FileName;
  int Error;

private:
  vtkKWGenericMovieWriter(const vtkKWGenericMovieWriter&); // Not implemented
  void operator=(const vtkKWGenericMovieWriter&); // Not implemented
};

#endif



