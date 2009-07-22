#ifndef PyMPI_COMPAT_DEINOMPI_H
#define PyMPI_COMPAT_DEINOMPI_H

/* ---------------------------------------------------------------- */

static int    PyMPI_DEINOMPI_argc    = 0;
static char **PyMPI_DEINOMPI_argv    = 0;
static char  *PyMPI_DEINOMPI_args[2] = {0, 0};

static void PyMPI_DEINOMPI_FixArgs(int **argc, char ****argv)
{
  if ((argc[0]==(int *)0) || (argv[0]==(char ***)0)) {
#ifdef Py_PYTHON_H
#if PY_MAJOR_VERSION >= 3
    PyMPI_DEINOMPI_args[0] = (char *) "python";
#else
    PyMPI_DEINOMPI_args[0] = Py_GetProgramName();
#endif
    PyMPI_DEINOMPI_argc = 1;
#endif
    PyMPI_DEINOMPI_argv = PyMPI_DEINOMPI_args;
    argc[0] = &PyMPI_DEINOMPI_argc;
    argv[0] = &PyMPI_DEINOMPI_argv;
  }
}

static int PyMPI_DEINOMPI_MPI_Init(int *argc, char ***argv)
{
  PyMPI_DEINOMPI_FixArgs(&argc, &argv);
  return MPI_Init(argc, argv);
}
#undef  MPI_Init
#define MPI_Init PyMPI_DEINOMPI_MPI_Init

static int PyMPI_DEINOMPI_MPI_Init_thread(int *argc, char ***argv,
                                          int required, int *provided)
{
  PyMPI_DEINOMPI_FixArgs(&argc, &argv);
  return MPI_Init_thread(argc, argv, required, provided);
}
#undef  MPI_Init_thread
#define MPI_Init_thread PyMPI_DEINOMPI_MPI_Init_thread

/* ---------------------------------------------------------------- */

#endif /* !PyMPI_COMPAT_DEINOMPI_H */
