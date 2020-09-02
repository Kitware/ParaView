/*=========================================================================

  Program:   ParaView
  Module:    vtkFileSequenceParser.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFileSequenceParser
 * @brief   Parses out the base file name of a file
 * sequence and also the specific index of the given file.
 *
 * Given a file name (without path). I will
 * extract the base portion of the file name that is common to all the files
 * in the sequence. It will also provide the current sequence index of the
 * provided file name.
 * by several vtkPVUpdateSuppressor objects.
*/

#ifndef vtkFileSequenceParser_h
#define vtkFileSequenceParser_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsCoreModule.h" //needed for exports

#include <string>

namespace vtksys
{
class RegularExpression;
}

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkFileSequenceParser : public vtkObject
{
public:
  static vtkFileSequenceParser* New();
  vtkTypeMacro(vtkFileSequenceParser, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Extract base file name sequence from the file.
   * Returns true if a sequence is detected and
   * sets SequenceName and SequenceIndex.
   */
  bool ParseFileSequence(const char* file);

  /**
   * Getter to use to get the sequence name after
   * calling ParseFileSequence
   */
  vtkGetStringMacro(SequenceName);

  /**
   * Getter to use to get the sequence index after
   * calling ParseFileSequence
   */
  vtkGetMacro(SequenceIndex, int);

  /**
   * Getter to use to get the sequence index string
   * after calling ParseFileSequence. It can be usefull
   * when sequence index is duplicated.
   */
  vtkGetMacro(SequenceIndexString, std::string);

protected:
  vtkFileSequenceParser();
  ~vtkFileSequenceParser() override;

  vtksys::RegularExpression* reg_ex;
  vtksys::RegularExpression* reg_ex2;
  vtksys::RegularExpression* reg_ex3;
  vtksys::RegularExpression* reg_ex4;
  vtksys::RegularExpression* reg_ex5;
  vtksys::RegularExpression* reg_ex_last;

  // Used internal so char * allocations are done automatically.
  vtkSetStringMacro(SequenceName);

  int SequenceIndex;
  char* SequenceName;
  std::string SequenceIndexString;

private:
  vtkFileSequenceParser(const vtkFileSequenceParser&) = delete;
  void operator=(const vtkFileSequenceParser&) = delete;
};

#endif
