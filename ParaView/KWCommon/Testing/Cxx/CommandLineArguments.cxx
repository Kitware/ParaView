#include "vtkKWArguments.h"

void* random_ptr = (void*)0x123;

int argument(const char* arg, const char* value, void* call_data)
{
  cout << "Got argument: \"" << arg << "\" value: \"" << (value?value:"(null)") << "\"" << endl;
  if ( call_data != random_ptr )
    {
    cerr << "Problem processing call_data" << endl;
    return 0;
    }
  return 1;
}

int unknown_argument(const char* argument, void* call_data)
{
  cout << "Got unknown argument: \"" << argument << "\"" << endl;
  if ( call_data != random_ptr )
    {
    cerr << "Problem processing call_data" << endl;
    return 0;
    }
  return 1;
}

vtkKWArguments::CallbackStructure callbacks[] = {
    { "-A", vtkKWArguments::NO_ARGUMENT, argument, random_ptr },
    { "-B", vtkKWArguments::SPACE_ARGUMENT, argument, random_ptr },
    { "-C", vtkKWArguments::EQUAL_ARGUMENT, argument, random_ptr },
    { "-D", vtkKWArguments::CONCAT_ARGUMENT, argument, random_ptr },
    { "--long1", vtkKWArguments::NO_ARGUMENT, argument, random_ptr },
    { "--long2", vtkKWArguments::SPACE_ARGUMENT, argument, random_ptr },
    { "--long3", vtkKWArguments::EQUAL_ARGUMENT, argument, random_ptr },
    { "--long4", vtkKWArguments::CONCAT_ARGUMENT, argument, random_ptr },
    { 0, 0, 0, 0 }
    };

int main(int argc, char* argv[])
{
  int res = 0;
  vtkKWArguments *arg = vtkKWArguments::New();
  arg->Initialize(argc, argv);
  arg->AddCallbacks(callbacks);
  arg->SetClientData(random_ptr);
  arg->SetUnknownArgumentCallback(unknown_argument);
  if ( !arg->Parse() )
    {
    cerr << "Problem parsing arguments" << endl;
    res = 1;
    }
  arg->Delete();
  return res;
}
