#include "vtkObject.h"

int TestConst(char* s){ return 1; }
int TestConst(const char* s) { return 0; }

int main(int, char** argv)
{
  char* s = new char[ 10 ];
  int res = TestConst(s);
  delete [] s;
  cout << "Result: " << res << endl;
  res = TestConst("andy");
  cout << "Result: " << res << endl;
  return res;
}

