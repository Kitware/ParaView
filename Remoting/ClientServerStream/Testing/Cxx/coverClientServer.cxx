#include "vtkClientServerStream.h"
#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkStringArray.h"
#include "vtkVariantArray.h"

static double dblIni[] = { 904., 906., 917. };
static const char* strIni[] = { "901", "Turbo", "Targa" };

template <class T>
struct Help
{
  static void Store(vtkClientServerStream& css)
  {
    T v = 123;
    T a[2] = { 12, 3 };
    css << v << vtkClientServerStream::InsertArray(a, 2);
  }
  static bool Check(vtkClientServerStream& css, int& arg)
  {
    T v;
    T a[2];
    if (!css.GetArgument(0, arg++, &v) || v != 123)
    {
      return false;
    }
    if (!css.GetArgument(0, arg++, a, 2) || a[0] != 12 || a[1] != 3)
    {
      return false;
    }
    return true;
  }
};

// Store a bunch of values in a stream.
void do_store(vtkClientServerStream& css)
{
  vtkClientServerID id(123);
  css.Reserve(64);
  css.Reset();
  css << vtkClientServerStream::Reply;
  css << true;
  css << id;
  Help<char>::Store(css);
  Help<short>::Store(css);
  Help<int>::Store(css);
  Help<long>::Store(css);
  Help<signed char>::Store(css);
  Help<unsigned char>::Store(css);
  Help<unsigned short>::Store(css);
  Help<unsigned int>::Store(css);
  Help<unsigned long>::Store(css);
  Help<long long>::Store(css);
  Help<unsigned long long>::Store(css);
#if defined(VTK_TYPE_USE___INT64)
  Help<__int64>::Store(css);
  Help<unsigned __int64>::Store(css);
#endif
  Help<float>::Store(css);
  Help<double>::Store(css);

  vtkVariant varStr("911");
  css << varStr;
  vtkVariant varNum(356.0);
  css << varNum;

  // Test a variant that is itself an array of primitives (doubles).
  vtkNew<vtkDoubleArray> dblArr;
  dblArr->SetArray(dblIni, sizeof(dblIni) / sizeof(dblIni[0]), 1);
  vtkVariant varDblArr(dblArr.GetPointer());
  css << varDblArr;

  // Test a variant that is itself an array of composites (strings).
  vtkNew<vtkStringArray> strArr;
  strArr->SetNumberOfValues(sizeof(strIni) / sizeof(strIni[0]));
  for (vtkIdType i = 0; i < strArr->GetNumberOfValues(); ++i)
  {
    strArr->SetValue(i, strIni[i]);
  }
  vtkVariant varStrArr(strArr.GetPointer());
  css << varStrArr;

  // Test a variant that is itself an array of composites (variants).
  vtkNew<vtkVariantArray> varArr;
  varArr->SetNumberOfValues(sizeof(strIni) / sizeof(strIni[0]));
  for (vtkIdType i = 0; i < varArr->GetNumberOfValues(); ++i)
  {
    vtkVariant tmp(strIni[i]);
    bool numeric;
    double x = tmp.ToDouble(&numeric);
    if (numeric)
    {
      varArr->SetVariantValue(i, x);
    }
    else
    {
      varArr->SetVariantValue(i, strIni[i]);
    }
  }
  vtkVariant varVarArr(varArr.GetPointer());
  css << varVarArr;

  vtkVariant varInvalid;
  css << varInvalid;

  css << "123";
  {
    vtkClientServerStream nested;
    nested << vtkClientServerStream::Reply << "456" << vtkClientServerStream::End;
    css << nested;
  }
  css << vtkClientServerStream::End;
}

// Check stored values in a stream.
bool do_check(vtkClientServerStream& css)
{
  if (css.GetNumberOfMessages() != 1)
  {
    return false;
  }
  if (css.GetCommand(0) != vtkClientServerStream::Reply)
  {
    return false;
  }
  int arg = 0;
  {
    bool b;
    if (!css.GetArgument(0, arg++, &b) || b != true)
    {
      return false;
    }
  }
  {
    vtkClientServerID id;
    if (!css.GetArgument(0, arg++, &id) || id.ID != 123)
    {
      return false;
    }
  }
  {
    if (!Help<char>::Check(css, arg))
    {
      return false;
    }
    if (!Help<short>::Check(css, arg))
    {
      return false;
    }
    if (!Help<int>::Check(css, arg))
    {
      return false;
    }
    if (!Help<long>::Check(css, arg))
    {
      return false;
    }
    if (!Help<signed char>::Check(css, arg))
    {
      return false;
    }
    if (!Help<unsigned char>::Check(css, arg))
    {
      return false;
    }
    if (!Help<unsigned short>::Check(css, arg))
    {
      return false;
    }
    if (!Help<unsigned int>::Check(css, arg))
    {
      return false;
    }
    if (!Help<unsigned long>::Check(css, arg))
    {
      return false;
    }
    if (!Help<long long>::Check(css, arg))
    {
      return false;
    }
    if (!Help<unsigned long long>::Check(css, arg))
    {
      return false;
    }
#if defined(VTK_TYPE_USE___INT64)
    if (!Help<__int64>::Check(css, arg))
    {
      return false;
    }
    if (!Help<unsigned __int64>::Check(css, arg))
    {
      return false;
    }
#endif
    if (!Help<float>::Check(css, arg))
    {
      return false;
    }
    if (!Help<double>::Check(css, arg))
    {
      return false;
    }
  }
  {
    vtkVariant varOut;
    if (!css.GetArgument(0, arg, &varOut) || varOut.ToString() != "911")
    {
      cout << "Variant was \"" << varOut.ToString().c_str() << "\" instead of 911\n";
      return false;
    }
    if (!css.GetArgument(0, arg, &varOut) || varOut.ToDouble() != 356.0)
    {
      cout << "Variant was \"" << varOut.ToString().c_str() << "\" instead of 356.0\n";
      return false;
    }
    vtkDoubleArray* dblArr;
    if (!css.GetArgument(0, arg, &varOut) ||
      !(dblArr = vtkDoubleArray::SafeDownCast(varOut.ToArray())) ||
      dblArr->GetMaxId() != (sizeof(dblIni) / sizeof(dblIni[0]) - 1) ||
      dblArr->GetValue(0) != dblIni[0] || dblArr->GetValue(1) != dblIni[1] ||
      dblArr->GetValue(2) != dblIni[2])
    {
      cout << "Variant was \"" << varOut.ToString().c_str()
           << "\" instead of (904.0, 906.0, 917.0)\n";
      return false;
    }
    vtkStringArray* strArr;
    if (!css.GetArgument(0, arg, &varOut) ||
      !(strArr = vtkStringArray::SafeDownCast(varOut.ToArray())) ||
      strArr->GetMaxId() != (sizeof(strIni) / sizeof(strIni[0]) - 1) ||
      strArr->GetValue(0) != strIni[0] || strArr->GetValue(1) != strIni[1] ||
      strArr->GetValue(2) != strIni[2])
    {
      cout << "Variant was \"" << varOut.ToString().c_str()
           << "\" instead of (\"901\", \"Turbo\", \"Targa\")\n";
      return false;
    }
    vtkVariantArray* varArr;
    if (!css.GetArgument(0, arg, &varOut) ||
      !(varArr = vtkVariantArray::SafeDownCast(varOut.ToArray())) ||
      varArr->GetMaxId() != (sizeof(strIni) / sizeof(strIni[0]) - 1) ||
      varArr->GetValue(0) != vtkVariant(strIni[0]).ToDouble() || varArr->GetValue(1) != strIni[1] ||
      varArr->GetValue(2) != strIni[2])
    {
      cout << "Variant was \"" << varOut.ToString().c_str()
           << "\" instead of (901.0, \"Turbo\", \"Targa\")\n";
      return false;
    }
    if (!css.GetArgument(0, arg, &varOut) || varOut.IsValid())
    {
      cout << "Variant that was supposed to be invalid is valid\n";
      return false;
    }
  }
  {
    const char* s;
    if (!css.GetArgument(0, arg++, &s) || strcmp(s, "123") != 0)
    {
      return false;
    }
  }
  {
    const char* s;
    vtkClientServerStream nested;
    if (!css.GetArgument(0, arg++, &nested))
    {
      return false;
    }
    if (!nested.GetArgument(0, 0, &s) || strcmp(s, "456") != 0)
    {
      return false;
    }
  }
  return true;
}

bool do_test()
{
  // Construct a stream and store values.
  vtkClientServerStream css1;
  do_store(css1);

  // Cover stream print code.
  cout << "-----------------------------------------------------------\n";
  css1.Print(cout);
  cout << "-----------------------------------------------------------\n";

  // Copy the stream in various ways.
  vtkClientServerStream css2(css1);
  vtkClientServerStream css3;
  css3.Copy(&css2);
  vtkClientServerStream css4;
  if (!css4.StreamFromString(css3.StreamToString()))
  {
    cerr << "FAILED: StreamToString or StreamFromString failed." << endl;
    return false;
  }
  vtkClientServerStream css5;
  {
    const unsigned char* data;
    size_t length;
    if (!css4.GetData(&data, &length))
    {
      cerr << "FAILED: GetData failed." << endl;
      return false;
    }
    if (!css5.SetData(data, length))
    {
      cerr << "FAILED: SetData failed." << endl;
      return false;
    }
  }

  if (!do_check(css1))
  {
    cerr << "FAILED: Stream contents could not correctly be retrieved." << endl;
    return false;
  }
  if (!do_check(css2))
  {
    cerr << "FAILED: Copy constructor did not copy stream properly." << endl;
    return false;
  }
  if (!do_check(css3))
  {
    cerr << "FAILED: Copy method did not copy stream properly." << endl;
    return false;
  }
  if (!do_check(css4))
  {
    cerr << "FAILED: String(To/From)Stream did not copy stream properly." << endl;
    return false;
  }
  if (!do_check(css5))
  {
    cerr << "FAILED: (Get/Set)Data did not copy stream properly." << endl;
    return false;
  }
  return true;
}

int coverClientServer(int, char* [])
{
  return do_test() ? 0 : 1;
}
