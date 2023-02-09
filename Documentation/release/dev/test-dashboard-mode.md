## Add dashboard mode support to XML testing utility and file dialog

In ParaView, when playing test, a environement variable (`DASHBOARD_TEST_FROM_CTEST`) is positioned so that
some dialog and mechansim are not displayed to shortcut them.

This is very useful but makes it hard to actually test specifically the dialog being shortcutted.

This is now possible thanks to the dashboard mode infrastructure provided by QtTesting.

For standard test, there is no change. The environnement variable will be automatically set when
playing/recording tests so that ParaView behave in dashboard mode in that case.

However, when needing to test a shortcut dialog, then the Record test dialog can be used to disable dashboard mode.
Then ParaView will behave as if not in dashboard mode.

A dedicated event will be recorded in the XML of the test so that, when playing the test, dashboard mode
is also deactivated.

FileDialog player/tranlators are now also only enabled when using dashboard mode.


Developer notes:

- `DASHBOARD_TEST_FROM_CTEST` is now set when recording tests using Tools->RecordTest.
- Some Qt dialog changes their behavior when `DASHBOARD_TEST_FROM_CTEST` (already the case before)
- FileDialog QtTesting translator is now only created when `DASHBOARD_TEST_FROM_CTEST` is set
- QtTesting record dialog now let us disable/enable dashboard mode (QtTesting) and unset/put the `DASHBOARD_TEST_FROM_CTEST` env var (pqCoreTestUtility)
- When unchecking/checking, an event (`dashboard_mode`) is recorded in the XML test  (handled by QtTesting)
- When playing a test, either through ctest, `--play-test` or Tools->PlayTest, the `DASHBOARD_TEST_FROM_CTEST` is set automatically
- When a `dashboard_mode` event happens, the `DASHBOARD_TEST_FROM_CTEST`  is unset/put, changing behavior of ParaView accordingly
