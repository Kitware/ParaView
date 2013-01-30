/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVDataEncoder - class used to compress/encode images using threads.
// .SECTION Description
// vtkPVDataEncoder is used to compress and encode images using threads.
// Multiple images can be pushed into the encoder for compression and encoding.
// We use a vtkTypeUInt32 as the key to identify different image pipes. The
// images in each pipe will be processed in parallel threads. The latest
// compressed and encoded image can be accessed using GetLatestOutput().
//
// vtkPVDataEncoder uses a thread-pool to do the compression and encoding in
// parallel.  Note that images may not come out of the vtkPVDataEncoder in the
// same order as they are pushed in, if an image pushed in at N-th location
// takes longer to compress and encode than that pushed in at N+1-th location or
// if it was pushed in before the N-th location was even taken up for encoding
// by the a thread in the thread pool.

#ifndef __vtkPVDataEncoder_h
#define __vtkPVDataEncoder_h

#include "vtkObject.h"
#include "vtkParaViewWebModule.h" // needed for exports
#include "vtkSmartPointer.h" // needed for vtkSmartPointer

class vtkUnsignedCharArray;
class vtkImageData;

class VTKPARAVIEWWEB_EXPORT vtkPVDataEncoder : public vtkObject
{
public:
  static vtkPVDataEncoder* New();
  vtkTypeMacro(vtkPVDataEncoder, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Re-initializes the encoder. This will abort any on going encoding threads
  // and clear internal data-structures.
  void Initialize();

  // Description:
  // Push an image into the encoder. It is not safe to modify the image
  // after this point, including changing the reference counts for it.
  // You may run into thread safety issues. Typically,
  // the caller code will simply release reference to the data and stop using
  // it. vtkPVDataEncoder takes over the reference for the image and will call
  // vtkObject::UnRegister() on it when it's done.
  void PushAndTakeReference(vtkTypeUInt32 key, vtkImageData* &data, int quality);

  // Description:
  // Get access to the most-recent fully encoded result corresponding to the
  // given key, if any. This methods returns true if the \c data obtained is the
  // result from the most recent Push() for the key, if any. If this method
  // returns false, it means that there's some image either being processed on
  // pending processing.
  bool GetLatestOutput(vtkTypeUInt32 key,vtkSmartPointer<vtkUnsignedCharArray>& data);

  // Description:
  // Flushes the encoding pipe and blocks till the most recently pushed image
  // for the particular key has been processed. This call will block. Once this
  // method returns, caller can use GetLatestOutput(key) to access the processed
  // output.
  void Flush(vtkTypeUInt32 key);

//BTX
protected:
  vtkPVDataEncoder();
  ~vtkPVDataEncoder();

private:
  vtkPVDataEncoder(const vtkPVDataEncoder&); // Not implemented
  void operator=(const vtkPVDataEncoder&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
