## Test timeout adjustment

If you are developing XML tests in ParaView, you can now change
the default test timeout by setting a CMake variable which includes
the test name to the number of seconds for that test's timeout.

For example, to make the `UndoRedo1.xml` test timeout 300 seconds
instead of the default 100 seconds specified in `CTEST_TEST_TIMEOUT`,
just add

```cmake
set(UndoRedo1_TIMEOUT 300)
```

somewhere *before* you call a `paraview_add_*_tests()` macro to add
the test XML.
