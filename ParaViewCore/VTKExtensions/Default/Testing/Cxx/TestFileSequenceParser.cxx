/*=========================================================================

  Program:   ParaView
  Module:    TestFileSequenceParser.cxx

  Copyright (c) Menno Deij - van Rijswijk, MARIN, The Netherlands
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkFileSequenceParser.h>
#include <vtkNew.h>

bool check_group(vtkFileSequenceParser* parser, const char* fname, const char* seqname)
{
  if (!parser->ParseFileSequence(fname))
  {
    cout << "ERROR: group not detected for '" << fname << "'" << endl;
    return false;
  }
  if (strcmp(parser->GetSequenceName(), seqname) != 0)
  {
    cout << "ERROR: sequence name mismatch for '" << fname << "' " << endl
         << "  expected : '" << seqname << "'" << endl
         << "      got  : '" << parser->GetSequenceName() << "'" << endl;
    return false;
  }
  return true;
}

bool check_no_group(vtkFileSequenceParser* parser, const char* fname)
{
  if (parser->ParseFileSequence(fname))
  {
    cout << "ERROR: group detected erroneously for '" << fname << "'" << endl;
    return false;
  }
  return true;
}

int TestFileSequenceParser(int, char* argv[])
{
  (void)argv;
  vtkNew<vtkFileSequenceParser> seqParser;

  check_group(seqParser.Get(), "foo.1.csv", "foo...csv");
  check_group(seqParser.Get(), "foo1.csv", "foo..csv");
  check_group(seqParser.Get(), "alpha99beta88gamma0001.csv", "alpha99beta88gamma..csv");
  check_group(seqParser.Get(), "foo.csv.1", "foo.csv");
  check_group(seqParser.Get(), "foo.csv.10.0", "foo.csv.10");
  check_group(seqParser.Get(), "spcta.10", "spcta");
  check_group(seqParser.Get(), "spcta1.10", "spcta1");
  check_group(seqParser.Get(), "Project_01_solution.cgns", "Project_.._solution.cgns");
  check_group(seqParser.Get(), "prefix-021-suffix.ext", "prefix-..-suffix.ext");
  check_group(seqParser.Get(), "prefix021suffix.ext", "prefix..suffix.ext");
  check_group(seqParser.Get(), "plt0001000", "plt..");

  check_no_group(seqParser.Get(), "foo.3dm");
  check_no_group(seqParser.Get(), "foo.2dm");

  return EXIT_SUCCESS;
}
