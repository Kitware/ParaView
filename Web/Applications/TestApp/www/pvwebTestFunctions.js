
//----------------------------------------------------------------------------
// This function contains the test functions which can test the individual
// protocols as well as the javascript module registration, and possibly other
// things as well.  Adding test functions inside the function
// ParaViewWebTestFunctions will result in their being auto-registered in the
// TestApp.
//
// Test functions should follow the examples in here, e.g.:
//
//    someRandomTestName: function(testName, resultCallback) {
//        doSomePossiblyAsynchronousWork(function(asyncResult) {
//            if (asyncResult == "ok") {
//                resultCallback({ 'name': testName,
//                                 'pass': true|false,
//                                 'message': "Some important message to report" });
//            }
//        });
//    },
//
// These functions are given the test name so that the test framework can
// know who's results are coming back in the resultCallback.  By using a
// callback to return results, the functions can do asynchronous work.
//----------------------------------------------------------------------------

function ParaViewWebTestFunctions(connection) {

  return {
        /**
         * This function tests whether all the necessary javascript modules
         * have properly registered themselves.
         */
         moduleRegistrationTest: function(testName, resultCallback) {
          if (vtkWeb.allModulesPresent(['vtkweb-base',
            'vtkweb-launcher',
            'vtkweb-connect',
            'vtkweb-viewport',
            'vtkweb-viewport-image',
            'vtkweb-viewport-webgl',
            'paraview-ui-pipeline',
            'paraview-ui-toolbar-connect',
            'paraview-ui-toolbar-vcr',
            'paraview-ui-toolbar-viewport'])) {
                // return { 'pass': true,
                //          'message': 'All required modules are registered' };
                resultCallback({ 'name': testName,
                 'success': true,
                 'message': 'All required modules are registered' });
              } else {
                // return { 'pass': false,
                //          'message': 'Failed test for registered modules' };
                resultCallback({ 'name': testName,
                 'success': false,
                 'message': 'Failed test for registered modules' });
              }
            },

        /**
         * This function tests the 'vtk:listFiles' protocol.  It assumes
         * that the pv_web_test_app.py server was started with a data dir
         * which contains the cloned ParaViewData repository.
         */
         protocolListFilesTest: function(testName, resultCallback) {
          try {
            connection.session.call('vtk:listFiles').then(function(obj) {
              var returnValue = true;
              var successMsg = "Successful test of protocol vtk:listFiles";

              var names = obj.map(function(elt) {
                  return elt.name;
              });

              var expectedNames = [ 'sonic.pht', 'GMV'];

              var errorMsg = "Error testing vtk:listFiles -- ";

              // TODO: This is O(n*m) in the lengths of the two lists,
              // which could made more efficient
              for (var idx = 0; idx < expectedNames.length; ++idx) {
                var foundExpectedName = false;

                // Check if the expected name is in the larger list
                for (var namesIdx = 0; namesIdx < names.length; ++namesIdx) {
                    if (expectedNames[idx] === names[namesIdx]) {
                        foundExpectedName = true;
                    }
                }

                if (foundExpectedName === false) {
                    errorMsg += "Did not find expected data item: " + expectedNames[idx] + " ";
                    returnValue = false;
                }
              }

              var msg = successMsg;

              if (returnValue === false) {
                msg = errorMsg;
              }

              resultCallback({ 'name': testName,
               'success': returnValue,
               'message': msg });
            });
          } catch(error) {
            resultCallback({ 'name': testName,
             'success': false,
             'message': error });
          }
        }
      };
    }
