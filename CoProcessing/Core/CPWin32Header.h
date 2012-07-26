#if defined(VTK_BUILD_SHARED_LIBS)
#if defined(vtkCoProcessor_EXPORTS)
#define COPROCESSING_EXPORT VTK_ABI_EXPORT
#else
#define COPROCESSING_EXPORT VTK_ABI_IMPORT
#endif
#else
#define COPROCESSING_EXPORT
#endif
