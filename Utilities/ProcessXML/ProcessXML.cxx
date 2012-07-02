/*=========================================================================

  Program:   ParaView
  Module:    ProcessXML.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkObject.h"

#include <string>
#include <vector>
#include <vtksys/SystemTools.hxx>
#include <vtksys/Base64.h>
#include <vtksys/RegularExpression.hxx>
#include <vtksys/ios/fstream>
#include <vtksys/ios/sstream>

#include <assert.h>
class Output
{
public:
  Output()
    {
    this->MaxLen = 16000;
    this->CurrentPosition = 0;
    this->UseBase64Encoding = false;
    }
  ~Output()
    {
    }
  Output(const Output&){}
  void operator=(const Output&){}
  vtksys_ios::ostringstream Stream;

  int MaxLen;
  long CurrentPosition;
  int Count;
  std::string Prefix;
  std::string Suffix;
  bool UseBase64Encoding;

  void PrintHeader(const char* title, const char* file)
    {
    this->Stream << endl
      << "// From file " << file << endl
      << "static const char* " << this->Prefix.c_str() << title << this->Suffix.c_str() << this->Count 
      << " =" << endl;
    this->CurrentPosition = this->Stream.tellp();
    }

  void CheckSplit(const char* title, const char* file, int force=0)
    {
    if ( (static_cast<long>(this->Stream.tellp()) - this->CurrentPosition) > this->MaxLen ||
      force )
      {
      this->Count ++;
      this->Stream << ";" << endl;
      this->PrintHeader(title, file);
      }
    }

  int ProcessFile(const char* file, const char* title)
    {
    vtksys_ios::ifstream ifs(file,
      this->UseBase64Encoding? (vtksys_ios::ifstream::in |
        vtksys_ios::ifstream::binary) : vtksys_ios::ifstream::in);
    if ( !ifs )
      {
      cout << "Canot open file: " << file << endl;
      return 0;
      }
    if (this->UseBase64Encoding)
      {
      ifs.seekg(0, vtksys_ios::ios::end);
      size_t length = ifs.tellg();
      ifs.seekg(0, vtksys_ios::ios::beg);
      unsigned char* buffer = new unsigned char[length];
      ifs.read(reinterpret_cast<char*>(buffer), length);
      ifs.close();

      char* encoded_buffer = new char[static_cast<int>(1.5 * length + 8)];
      unsigned long end = vtksysBase64_Encode(buffer, length,
        reinterpret_cast<unsigned char*>(encoded_buffer),
        0);
      encoded_buffer[end] = 0;
      vtksys_ios::istringstream iss(encoded_buffer);

      delete[] buffer;
      delete[] encoded_buffer;
      return this->ProcessFile(iss, file, title);
      }
    else
      {
      return this->ProcessFile(ifs, file, title);
      }
    }

  bool ReadLine(vtksys_ios::istream& ifs, std::string &line)
    {
    if (!ifs)
      {
      return false;
      }

    if (this->UseBase64Encoding)
      {
      char buffer[81];
      buffer[80] = 0;
      ifs.read(buffer, 80);
      size_t gcount = ifs.gcount();
      buffer[gcount] = 0;
      line = buffer;
      return (gcount > 0);
      }
    else
      {
      return vtksys::SystemTools::GetLineFromStream(ifs, line);
      }
    }

  int ProcessFile(vtksys_ios::istream &ifs, const char* file, const char* title)
    {
    int ch;
    int in_ifdef = 0;

    this->Count = 0;
    this->PrintHeader(title, file);
    this->Stream << "\"";

    std::string line;
    std::string::size_type cc;

    vtksys::RegularExpression reIfDef("^[ \r\n\t]*#[ \r\n\t]*if");
    vtksys::RegularExpression reElse("^[ \r\n\t]*#[ \r\n\t]*el(se|if)");
    vtksys::RegularExpression reEndif("^[ \r\n\t]*#[ \r\n\t]*endif");
    int res = 0;

    while (this->ReadLine(ifs, line))
      {
      res ++;
      int regex = 0;
      int ifdef_line = 0;
      if (!this->UseBase64Encoding)
        {
        if ( reIfDef.find(line) )
          {
          in_ifdef ++;
          regex = 1;
          ifdef_line = 1;
          }
        else if ( reElse.find(line) )
          {
          regex = 1;
          }
        else if ( reEndif.find(line) )
          {
          in_ifdef --;
          regex = 1;
          }
        }
      if ( regex )
        {
        assert (this->UseBase64Encoding == false);
        this->Stream << "\\n\"" << endl;
        if ( ifdef_line )
          {
          this->CheckSplit(title, file, 1);
          }
        this->Stream << line.c_str() << endl;
        if ( !ifdef_line )
          {
          this->CheckSplit(title, file);
          }
        this->Stream << "\"";
        }
      else
        {
        for ( cc = 0; cc < line.size(); cc ++ )
          {
          ch = line[cc];
          if ( ch == '\\' )
            {
            this->Stream << "\\\\";
            }
          else if ( ch == '\"' )
            {
            this->Stream << "\\\"";
            }
          else
            {
            this->Stream << (unsigned char)ch;
            }
          }
        this->Stream 
          << (this->UseBase64Encoding? "\"" : "\\n\"") << endl;
        if ( !in_ifdef )
          {
          this->CheckSplit(title, file);
          }
        this->Stream << "\"";
        }
      }
    this->Stream << (this->UseBase64Encoding? "\";" : "\\n\";") << endl;

    if ( !res )
      {
      return 0;
      }
    return this->Count+1;
    }
};

int main(int argc, char* argv[])
{
  if ( argc < 4 )
    {
    cerr << "Usage: " << argv[0]
      << " [-base64] <output-file> <prefix> <suffix> <getmethod> <modules>..." 
      << endl;
    return 1;
    }
  Output ot;

  int argv_offset = 0;
  if (strcmp(argv[1], "-base64") == 0)
    {
    ot.UseBase64Encoding = true;
    argv_offset = 1;
    }

  std::string output = argv[argv_offset + 1];
  std::string output_file_name =
    vtksys::SystemTools::GetFilenameWithoutExtension(
      output);
  ot.Prefix = argv[argv_offset + 2];
  ot.Suffix = argv[argv_offset + 3];
  ot.Stream << "// Loadable modules" << endl
    << "//" << endl
    << "// Generated by " << argv[0] << endl
    << "//" << endl
    << "#ifndef __" << output_file_name.c_str() << "_h" << endl
    << "#define __" << output_file_name.c_str() << "_h" << endl
    << endl
    << "#include <string.h>" << endl
    << endl;


  int cc;
  for ( cc = 5; (cc + argv_offset) < argc; cc ++ )
    {
    std::string fname = argv[argv_offset + cc];
    std::string moduleName =
      vtksys::SystemTools::GetFilenameWithoutLastExtension(fname);
    if (moduleName.size() == 0)
      {
      cerr << "Cannot extract module name from the file: " << argv[argv_offset + cc] << endl;
      return 1;
      }

    cout << "-- Generate module: " << moduleName << endl;

    int num = 0;
    if ( (num = ot.ProcessFile(fname.c_str(), moduleName.c_str())) == 0 )
      {
      cout << "Problem generating header file from XML file: " << fname.c_str() << endl;
      return 1;
      }
    int kk;
    vtksys_ios::ostringstream createstring;
    vtksys_ios::ostringstream lenstr;
    for ( kk = 0; kk < num; kk ++ )
      {
      lenstr << endl 
        << "    + strlen(" << ot.Prefix.c_str() 
        << moduleName.c_str() << ot.Suffix.c_str() << kk << ")";
      createstring << "  strcat(res, " << ot.Prefix.c_str() << moduleName.c_str() << ot.Suffix.c_str()
        << kk << ");" << endl;
      }
    ot.Stream
      << "// Get single string" << endl
      << "char* " << ot.Prefix.c_str() << moduleName.c_str() << argv[argv_offset + 4] 
      << "()" << endl
      << "{" << endl
      << "  size_t len = ( 0"
      << lenstr.str()
      << " );" << endl
      << "  char* res = new char[ len + 1];" << endl
      << "  res[0] = 0;" << endl
      << createstring.str()
      << "  return res;" << endl
      << "}" << endl << endl;
    }

  ot.Stream
    << endl << endl
    << "#endif" << endl;
  FILE* fp = fopen(output.c_str(), "w");
  if ( !fp )
    {
    cout << "Cannot open output file: " << output.c_str() << endl;
    return 1;
    }
  fprintf(fp, "%s", ot.Stream.str().c_str());
  fclose(fp);
  return 0;
}
