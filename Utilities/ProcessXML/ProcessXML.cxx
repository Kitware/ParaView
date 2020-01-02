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

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <vtksys/Base64.h>
#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

#include <assert.h>
#include <cstring>

class Output
{
public:
  Output()
  {
    this->MaxLen = 16000;
    this->CurrentPosition = 0;
    this->UseBase64Encoding = false;
  }
  ~Output() {}
  Output(const Output&) {}
  void operator=(const Output&) {}
  std::ostringstream Stream;

  int MaxLen;
  long CurrentPosition;
  int Count;
  std::string Prefix;
  std::string Suffix;
  bool UseBase64Encoding;

  void PrintHeader(const char* title, const char* file)
  {
    this->Stream << std::endl
                 << "// From file " << file << std::endl
                 << "static const char* const " << this->Prefix << title << this->Suffix
                 << this->Count << " =" << std::endl;
    this->CurrentPosition = this->Stream.tellp();
  }

  void CheckSplit(const char* title, const char* file, int force = 0)
  {
    if ((static_cast<long>(this->Stream.tellp()) - this->CurrentPosition) > this->MaxLen || force)
    {
      this->Count++;
      this->Stream << ";" << std::endl;
      this->PrintHeader(title, file);
    }
  }

  int ProcessFile(const char* file, const char* title)
  {
    std::ifstream ifs(file,
      this->UseBase64Encoding ? (std::ifstream::in | std::ifstream::binary) : std::ifstream::in);
    if (!ifs)
    {
      std::cerr << "Cannot open file: " << file << std::endl;
      return 0;
    }
    if (this->UseBase64Encoding)
    {
      ifs.seekg(0, std::ios::end);
      size_t length = ifs.tellg();
      ifs.seekg(0, std::ios::beg);
      unsigned char* buffer = new unsigned char[length];
      ifs.read(reinterpret_cast<char*>(buffer), length);
      ifs.close();

      char* encoded_buffer = new char[static_cast<int>(1.5 * length + 8)];
      size_t end = vtksysBase64_Encode(buffer, static_cast<unsigned long>(length),
        reinterpret_cast<unsigned char*>(encoded_buffer), 0);
      encoded_buffer[end] = 0;
      std::istringstream iss(encoded_buffer);

      delete[] buffer;
      delete[] encoded_buffer;
      return this->ProcessFile(iss, file, title);
    }
    else
    {
      return this->ProcessFile(ifs, file, title);
    }
  }

  bool ReadLine(std::istream& ifs, std::string& line)
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

  int ProcessFile(std::istream& ifs, const char* file, const char* title)
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
      res++;
      int regex = 0;
      int ifdef_line = 0;
      if (!this->UseBase64Encoding)
      {
        if (reIfDef.find(line))
        {
          in_ifdef++;
          regex = 1;
          ifdef_line = 1;
        }
        else if (reElse.find(line))
        {
          regex = 1;
        }
        else if (reEndif.find(line))
        {
          in_ifdef--;
          regex = 1;
        }
      }
      if (regex)
      {
        assert(this->UseBase64Encoding == false);
        this->Stream << "\\n\"" << std::endl;
        if (ifdef_line)
        {
          this->CheckSplit(title, file, 1);
        }
        this->Stream << line << std::endl;
        if (!ifdef_line)
        {
          this->CheckSplit(title, file);
        }
        this->Stream << "\"";
      }
      else
      {
        for (cc = 0; cc < line.size(); cc++)
        {
          ch = line[cc];
          if (ch == '\\')
          {
            this->Stream << "\\\\";
          }
          else if (ch == '\"')
          {
            this->Stream << "\\\"";
          }
          else
          {
            this->Stream << (unsigned char)ch;
          }
        }
        this->Stream << (this->UseBase64Encoding ? "\"" : "\\n\"") << std::endl;
        if (!in_ifdef)
        {
          this->CheckSplit(title, file);
        }
        this->Stream << "\"";
      }
    }
    this->Stream << (this->UseBase64Encoding ? "\";" : "\\n\";") << std::endl;

    if (!res)
    {
      return 0;
    }
    return this->Count + 1;
  }
};

static bool read_option_file(std::vector<std::string>& strings, const char* fname)
{
  std::ifstream fp(fname);
  if (!fp)
  {
    return false;
  }
  for (std::string line; std::getline(fp, line);)
  {
    strings.push_back(line);
  }
  return true;
}
static std::vector<std::string> parse_expand_args(int argc, char* argv[])
{
  std::vector<std::string> strings;
  for (int cc = 0; cc < argc; ++cc)
  {
    if (cc > 0 && argv[cc][0] == '@')
    {
      /* if read_option_file fails, add "@file" to the args */
      /* (this mimics the way that gcc expands @file arguments) */
      if (!read_option_file(strings, &argv[cc][1]))
      {
        strings.push_back(argv[cc]);
      }
    }
    else
    {
      strings.push_back(argv[cc]);
    }
  }

  return strings;
}

int main(int argc, char* argv[])
{
  auto args = parse_expand_args(argc, argv);
  if (args.size() < 4)
  {
    std::cerr << "Usage: " << argv[0]
              << " [-base64] <output-file> <prefix> <suffix> <getmethod> <modules>..." << std::endl;
    return 1;
  }
  Output ot;

  int argv_offset = 0;
  if (args[1] == "-base64")
  {
    ot.UseBase64Encoding = true;
    argv_offset = 1;
  }

  std::string output = args[argv_offset + 1];
  std::string output_file_name = vtksys::SystemTools::GetFilenameWithoutExtension(output);
  ot.Prefix = args[argv_offset + 2];
  ot.Suffix = args[argv_offset + 3];
  ot.Stream << "// Loadable modules" << std::endl
            << "//" << std::endl
            << "// Generated by " << argv[0] << std::endl
            << "//" << std::endl
            << "#ifndef " << output_file_name << "_h" << std::endl
            << "#define " << output_file_name << "_h" << std::endl
            << std::endl
            << "#include <string.h>" << std::endl
            << "#include <cassert>" << std::endl
            << "#include <algorithm>" << std::endl
            << std::endl;

  size_t cc;
  for (cc = 5; (cc + argv_offset) < args.size(); cc++)
  {
    std::string fname = args[argv_offset + cc];
    std::string moduleName = vtksys::SystemTools::GetFilenameWithoutLastExtension(fname);
    if (moduleName.size() == 0)
    {
      std::cerr << "Cannot extract module name from the file: " << args[argv_offset + cc].c_str()
                << std::endl;
      return 1;
    }

    int num = 0;
    if ((num = ot.ProcessFile(fname.c_str(), moduleName.c_str())) == 0)
    {
      std::cerr << "Problem generating header file from XML file: " << fname << std::endl;
      return 1;
    }
    int kk;
    std::ostringstream createstring;
    std::ostringstream lenstr;
    std::ostringstream prelenstr;
    for (kk = 0; kk < num; kk++)
    {
      prelenstr << std::endl
                << "  const size_t len" << kk << " = strlen(" << ot.Prefix << moduleName
                << ot.Suffix << kk << ");";
      lenstr << std::endl << "    + len" << kk;
      createstring << "  std::copy(" << ot.Prefix << moduleName << ot.Suffix << kk << ", "
                   << ot.Prefix << moduleName << ot.Suffix << kk << " + len" << kk
                   << ", res + offset); offset += len" << kk << ";" << std::endl;
    }
    ot.Stream << "// Get single string" << std::endl
              << "char* " << ot.Prefix << moduleName << args[argv_offset + 4].c_str() << "()"
              << std::endl
              << "{" << std::endl
              << prelenstr.str() << std::endl
              << "  size_t len = ( 0" << lenstr.str() << " );" << std::endl
              << "  char* res = new char[ len + 1];" << std::endl
              << "  size_t offset = 0;" << std::endl
              << createstring.str() << "  assert(offset == len);" << std::endl
              << "  res[offset] = 0;" << std::endl
              << "  return res;" << std::endl
              << "}" << std::endl
              << std::endl;
  }

  ot.Stream << std::endl << std::endl << "#endif" << std::endl;
  FILE* fp = fopen(output.c_str(), "w");
  if (!fp)
  {
    std::cerr << "Cannot open output file: " << output << std::endl;
    return 1;
  }
  fprintf(fp, "%s", ot.Stream.str().c_str());
  fclose(fp);
  return 0;
}
