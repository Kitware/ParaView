/* Author:  Lisandro Dalcin
 * Contact: dalcinl@gmail.com
 */

#include <Python.h>

#ifdef __FreeBSD__
#include <floatingpoint.h>
#endif

#define MPICH_IGNORE_CXX_SEEK
#define OMPI_IGNORE_CXX_SEEK
#include <mpi.h>


#if PY_MAJOR_VERSION >= 3

#include <locale.h>

static wchar_t **args_copy = NULL;

static void Py3_Main_DelArgs(wchar_t **args)
{
  int i = 0;
  if (args_copy)
    while (args_copy[i])
      PyMem_Free(args_copy[i++]);
  if (args_copy)
    PyMem_Free(args_copy);
  if (args)
    PyMem_Free(args);
  args_copy = NULL;
}

static wchar_t ** Py3_Main_GetArgs(int argc, char **argv)
{
  int i;
  wchar_t **args = NULL;
  char *locsave = NULL;

  args_copy = (wchar_t **)PyMem_Malloc((argc+1)*sizeof(wchar_t*));
  args = (wchar_t **)PyMem_Malloc((argc+1)*sizeof(wchar_t*));
  if (!args_copy || !args) {
    fprintf(stderr, "out of memory\n");
    goto fail;
  }
  for (i=0; i<argc; i++) {
    args_copy[i] = args[i] = NULL;
  }
  args_copy[argc] = args[argc] = NULL;

  locsave = setlocale(LC_ALL, NULL);
  setlocale(LC_ALL, "");
  for (i=0; i<argc; i++) {
    size_t argsz, count;
    argsz = strlen(argv[i]);
    if (argsz == (size_t)-1) {
      fprintf(stderr, "Could not convert argument %d to string\n", i);
      goto fail;
    }
    args[i] = (wchar_t *)PyMem_Malloc((argsz+1)*sizeof(wchar_t));
    if (!args[i]) {
      fprintf(stderr, "out of memory\n");
      goto fail;
    }
    args_copy[i] = args[i];
    count = mbstowcs(args[i], argv[i], argsz+1);
    if (count == (size_t)-1) {
      fprintf(stderr, "Could not convert argument %d to string\n", i);
      goto fail;
    }
  }
  setlocale(LC_ALL, locsave);

  return args;

 fail:

  Py3_Main_DelArgs(args);
  return NULL;
}

static int Py3_Main(int argc, char **argv)
{
  int sts = 0;
  wchar_t **wargv = Py3_Main_GetArgs(argc, argv);
  if (!wargv) sts = 1;
  else sts = Py_Main(argc, wargv);
  Py3_Main_DelArgs(wargv);
  return sts;
}

#define Py_Main Py3_Main

#endif /* !(PY_MAJOR_VERSION >= 3) */



#define CHKIERR(ierr) if (ierr) return 2

static int PyMPI_Main(int argc, char **argv)
{
  int sts=0, flag=0, ierr=0;

  /* MPI Initalization */
  ierr = MPI_Initialized(&flag); CHKIERR(ierr);
  if (!flag) {
#if 0
    int required = MPI_THREAD_MULTIPLE;
    int provided = MPI_THREAD_SINGLE;
    ierr = MPI_Init_thread(&argc, &argv,
                           required, &provided); CHKIERR(ierr);
#else
    ierr = MPI_Init(&argc, &argv); CHKIERR(ierr);
#endif
  }

  /* Python main */
  sts = Py_Main(argc, argv);

  /* MPI finalization */
  ierr = MPI_Finalized(&flag); CHKIERR(ierr);
  if (!flag) {
    ierr = MPI_Finalize(); CHKIERR(ierr);
  }

  /* return */
  return sts;
}



int main(int argc, char **argv)
{
#ifdef __FreeBSD__
  fp_except_t m;
  m = fpgetmask();
  fpsetmask(m & ~FP_X_OFL);
#endif
  return PyMPI_Main(argc, argv);
}


/*
   Local variables:
   c-basic-offset: 2
   indent-tabs-mode: nil
   End:
*/
