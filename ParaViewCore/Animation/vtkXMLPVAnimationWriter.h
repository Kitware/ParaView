/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLPVAnimationWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLPVAnimationWriter
 * @brief   Data writer for ParaView
 *
 * vtkXMLPVAnimationWriter is used to save all parts of a current
 * source to a file with pieces spread across other server processes.
*/

#ifndef vtkXMLPVAnimationWriter_h
#define vtkXMLPVAnimationWriter_h

#include "vtkPVAnimationModule.h" //needed for exports
#include "vtkXMLPVDWriter.h"

class vtkXMLPVAnimationWriterInternals;

class VTKPVANIMATION_EXPORT vtkXMLPVAnimationWriter : public vtkXMLPVDWriter
{
public:
  static vtkXMLPVAnimationWriter* New();
  vtkTypeMacro(vtkXMLPVAnimationWriter, vtkXMLPVDWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Add/Remove representations.
   */
  void AddRepresentation(vtkAlgorithm*, const char* groupname);
  void RemoveAllRepresentations();
  //@}

  /**
   * Start a new animation with the current set of inputs.
   */
  void Start();

  /**
   * Write the current time step.
   */
  void WriteTime(double time);

  /**
   * Finish an animation by writing the collection file.
   */
  void Finish();

protected:
  vtkXMLPVAnimationWriter();
  ~vtkXMLPVAnimationWriter() override;

  // Replace vtkXMLWriter's writing driver method.
  int WriteInternal() override;

  // Status safety check for method call ordering.
  int StartCalled;
  int FinishCalled;

  // Internal implementation details.
  vtkXMLPVAnimationWriterInternals* Internal;

  char** FileNamesCreated;
  int NumberOfFileNamesCreated;
  void AddFileName(const char* fileName);
  void DeleteFileNames();
  void DeleteFiles();

  void AddInputInternal(const char* group);

private:
  vtkXMLPVAnimationWriter(const vtkXMLPVAnimationWriter&) = delete;
  void operator=(const vtkXMLPVAnimationWriter&) = delete;
};

#endif
