
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
// callback to return results, the functions can do asynchronous work.  The
// resultCallback() should be the very last thing done in each test function,
// in other words, at the end of the innermost nested asynchronous call,
// because calling resultCallback() will trigger the next test to be run.
//----------------------------------------------------------------------------

function ParaViewWebTestFunctions(connection) {

    var compareTolerance = 0.0000001;

    // Issue the protocol rpc method to clean up the pipeline and all state
    function pipelineCleanup() {
        connection.session.call('vtk:clearAll').then(function(result) {
            console.log('Cleanup complete');
        }, function(error) {
            console.log('Encountered error attempting to clean up pipeline');
        });
    }

    // This is the error handler for the asynchronous rpc method calls.
    // By the time this function is invoked, the try/catch block surrounding
    // the rpc invocations is out of scope, so we need to just cleanup the
    // pipeline and call the resultCallback here.
    function protocolError(errMsg, testName, callback) {
        console.log("Encountered an error calling protocol:");
        console.log(errMsg);

        console.log("About to call resultCallback");
        callback({ 'name': testName,
                   'success': false,
                   'message': "Protocol error: " + errMsg.detail.join('\n') });
    }

    // Return true if expected and actual are within epsilon of each other
    function epsilonCompare(expected, actual, epsilon) {
        return Math.abs(expected - actual) < epsilon;
    }

    // Check a tuple (2 elts) of range elements to see whether they are
    // close enough to an expected range.
    function checkRangeEqualWithTolerance(expected, actual, testName, callback, errmsg) {
        if ( !(epsilonCompare(expected[0], actual[0], compareTolerance) &&
               epsilonCompare(expected[1], actual[1], compareTolerance)) ) {
            var msg = errmsg + ", expected range: " + expected + ", actual range: " + actual;
            console.log("Protocol range check error: " + msg);
            pipelineCleanup();
            callback({ 'name': testName,
                       'success': false,
                       'message': "Protocol range check error: " + msg });
            return false;
        }
        return true;
    }

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
                    var msg = "Successful test of protocol vtk:listFiles";

                    var names = obj.map(function(elt) {
                        return elt.name;
                    });

                    var expectedNames = [ 'sonic.pht', 'can.ex2', 'GMV'];

                    var errorMsg = "Error testing vtk:listFiles -- ";

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

                    if (returnValue === false) {
                        msg = errorMsg;
                    }

                    resultCallback({ 'name': testName,
                                     'success': returnValue,
                                     'message': msg });
                }, function(e) { protocolError(e, testName, resultCallback); });
            } catch(error) {
                resultCallback({ 'name': testName,
                                 'success': false,
                                 'message': error });
            }
        },

        /**
         * This function exercises the following protocol rpc methods:
         *
         * 'vtk:openRelativeFile'
         * 'vtk:updateDisplayProperty'
         * 'vtk:rescaleTransferFunction'
         * 'vtk:getLutDataRange'
         * 'vtk:updateTime'
         *
         * It needs access to the 'can.ex2' dataset in order to fully
         * test the scalar range protocols.
         */
        protocolScalarRangeTests: function(testName, resultCallback) {
            var s = connection.session;

            function testRescaleCustom(proxyId) {
                var rescaleOpts = { 'proxyId': proxyId,
                                    'type': 'custom',
                                    'min': 5,
                                    'max': 10 };
                s.call('vtk:rescaleTransferFunction', rescaleOpts).then(function(success) {
                    s.call('vtk:getLutDataRange', 'DISPL', '').then(function(range) {
                        if (checkRangeEqualWithTolerance(range,
                                                         [5, 10],
                                                         testName,
                                                         resultCallback,
                                                         "Color range incorrect after custom rescale")) {
                            pipelineCleanup();
                            resultCallback({ 'name': testName,
                                             'success': true,
                                             'message': 'Test of scalar range protocol successful' });
                        }
                    }, function(e) { protocolError(e, testName, resultCallback); });
                }, function(e) { protocolError(e, testName, resultCallback); });
            }

            function testRescaleData(proxyId) {
                var expectedLastTime = 0.004299988504499197
                s.call('vtk:updateTime', 'last').then(function(lastTime) {
                    if (!epsilonCompare(lastTime, expectedLastTime, compareTolerance)) {
                        throw "Error, the time at can's last timestep was: " +
                            lastTime + ", expected: " + expectedLastTime;
                    }
                    var rescaleOpts = { 'proxyId': proxyId,
                                        'type': 'data' };
                    s.call('vtk:rescaleTransferFunction', rescaleOpts).then(function(success) {
                        s.call('vtk:getLutDataRange', 'DISPL', '').then(function(range) {
                            if (checkRangeEqualWithTolerance(range,
                                                             [2.4611168269148833, 19.98399916713266],
                                                             testName,
                                                             resultCallback,
                                                             "Color range incorrect after data rescale")) {
                                // Only carry on if the last check was successful/true
                                testRescaleCustom(proxyId);
                            }
                        }, function(e) { protocolError(e, testName, resultCallback); });
                    }, function(e) { protocolError(e, testName, resultCallback); });
                }, function(e) { protocolError(e, testName, resultCallback); });
            }

            function testRescaleTime(proxyId) {
                var rescaleOpts = { 'proxyId': proxyId,
                                    'type': 'time' }
                s.call('vtk:rescaleTransferFunction', rescaleOpts).then(function(success) {
                    s.call('vtk:getLutDataRange', 'DISPL', '').then(function(newRange) {
                        if (checkRangeEqualWithTolerance(newRange,
                                                         [0, 19.98399916713266],
                                                         testName,
                                                         resultCallback,
                                                         "Color range incorrect after time rescale")) {
                            // Only carry on if the last check was successful/true
                            testRescaleData(proxyId);
                        }
                    }, function(e) { protocolError(e, testName, resultCallback); });
                }, function(e) { protocolError(e, testName, resultCallback); });
            }

            try {
                s.call('vtk:openRelativeFile', 'can.ex2').then(function(dataProxy) {
                    var proxyId = dataProxy['proxy_id'];
                    var opts = { 'proxy_id': proxyId,
                                 'ColorArrayName': 'DISPL',
                                 'ColorAttributeType': 'POINT_DATA' }
                    s.call('vtk:updateDisplayProperty', opts).then(function(empty) {
                        s.call('vtk:getLutDataRange', 'DISPL', '').then(function(range) {
                            if (checkRangeEqualWithTolerance(range,
                                                         [0, 0],
                                                         testName,
                                                         resultCallback,
                                                             "Initial color range incorrect")) {
                                // Only carry on if the last check was successful/true
                                testRescaleTime(proxyId);
                            }
                        }, function(e) { protocolError(e, testName, resultCallback); });
                    }, function(e) { protocolError(e, testName, resultCallback); });
                }, function(e) { protocolError(e, testName, resultCallback); });
            } catch(error) {
                console.log("Caught exception running test sequence for protocolScalarRangeTests");
                console.log(error);
                pipelineCleanup();
                resultCallback({ 'name': testName,
                                 'success': false,
                                 'message': error });
            }
        },

        /**
         * This function exercises the following protocol rpc methods:
         *
         * 'vtk:openRelativeFile'
         * 'vtk:updateDisplayProperty'
         * 'vtk:getScalarBarVisibilities'
         * 'vtk:setScalarBarVisibilities'
         *
         * It needs access to the 'can.ex2' dataset in order to test
         * these protocols.
         */
        protocolScalarBarVisibilityTests: function(testName, resultCallback) {
            var s = connection.session;

            try {
                s.call('vtk:openRelativeFile', 'can.ex2').then(function(dataProxy) {
                    var proxyId = dataProxy['proxy_id'];
                    var opts = { 'proxy_id': proxyId,
                                 'ColorArrayName': 'DISPL',
                                 'ColorAttributeType': 'POINT_DATA' };
                    s.call('vtk:updateDisplayProperty', opts).then(function(empty) {
                        var opts = {};
                        opts[proxyId] = 'true';
                        s.call('vtk:setScalarBarVisibilities', opts).then(function(visibilities) {
                            // Either of these next two lines will work as a protocol parameter
                            var params = [proxyId];
                            //var params = opts;
                            s.call('vtk:getScalarBarVisibilities', params).then(function(vizzies) {
                                pipelineCleanup();
                                var msg = "protocolScalarBarVisibilityTests successful";
                                var returnVal = true;
                                if (vizzies[proxyId] != true) {
                                    msg = "Error: proxy " + proxyId + " should now be visible";
                                    returnVal = false;
                                }
                                resultCallback({ 'name': testName,
                                                 'success': returnVal,
                                                 'message': msg });
                            }, function(e) { protocolError(e, testName, resultCallback); });
                        }, function(e) { protocolError(e, testName, resultCallback); });
                    }, function(e) { protocolError(e, testName, resultCallback); });
                }, function(e) { protocolError(e, testName, resultCallback); });
            } catch(error) {
                console.log("Caught exception running test sequence for protocolScalarBarVisibilityTests");
                console.log(error);
                pipelineCleanup();
                resultCallback({ 'name': testName,
                                 'success': false,
                                 'message': error });
            }
        },

        /**
         * In this test, we set up a pipeline with three nodes in it, color
         * each one by the same array, then turn on the scalar bar visibility
         * on just one of them.  Because they're all colored by the same
         * array, the scalar bar visibility of all of the nodes should
         * then be the same, that is, visible.
         *
         * This function exercises the following protocol rpc methods:
         *
         * 'vtk:openRelativeFile'
         * 'vtk:updateDisplayProperty'
         * 'vtk:addSource'
         * 'vtk:getScalarBarVisibilities'
         * 'vtk:setScalarBarVisibilities'
         *
         * It needs access to the 'can.ex2' dataset in order to test
         * these protocols.
         */
        protocolPipelineScalarBarTests: function(testName, resultCallback) {
            var s = connection.session;

            try {
                s.call('vtk:openRelativeFile', 'can.ex2').then(function(dataProxy) {
                    var proxyIdOne = dataProxy['proxy_id'];
                    var opts = { 'proxy_id': proxyIdOne,
                                 'ColorArrayName': 'DISPL',
                                 'ColorAttributeType': 'POINT_DATA' };
                    s.call('vtk:updateDisplayProperty', opts).then(function(empty) {
                        s.call('vtk:addSource', 'Clip', proxyIdOne).then(function(clipProxy) {
                            var proxyIdTwo = clipProxy['proxy_id'];
                            opts['proxy_id'] = proxyIdTwo;
                            s.call('vtk:updateDisplayProperty', opts).then(function(empty) {
                                s.call('vtk:addSource', 'Slice', proxyIdTwo).then(function(sliceProxy) {
                                    var proxyIdThree = sliceProxy['proxy_id'];
                                    opts['proxy_id'] = proxyIdTwo;
                                    s.call('vtk:updateDisplayProperty', opts).then(function(empty) {
                                        var setSbOpts = {};
                                        setSbOpts[proxyIdThree] = 'true';
                                        s.call('vtk:setScalarBarVisibilities', setSbOpts).then(function(visibilities) {
                                            var getSbOpts = {};
                                            getSbOpts[proxyIdOne] = '';
                                            getSbOpts[proxyIdTwo] = '';
                                            getSbOpts[proxyIdThree] = '';
                                            s.call('vtk:getScalarBarVisibilities', getSbOpts).then(function(vizzies) {
                                                pipelineCleanup();
                                                var msg = "protocolPipelineScalarBarTests successful";
                                                var returnVal = true;
                                                for (var proxyId in vizzies) {
                                                    if (vizzies[proxyId] != true) {
                                                        msg = "Error: proxy " + proxyId + " should have been visible";
                                                        returnVal = false;
                                                    }
                                                }
                                                resultCallback({ 'name': testName,
                                                                 'success': returnVal,
                                                                 'message': msg });
                                            }, function(e) { protocolError(e, testName, resultCallback); });
                                        }, function(e) { protocolError(e, testName, resultCallback); });
                                    }, function(e) { protocolError(e, testName, resultCallback); });
                                }, function(e) { protocolError(e, testName, resultCallback); });
                            }, function(e) { protocolError(e, testName, resultCallback); });
                        }, function(e) { protocolError(e, testName, resultCallback); });
                    }, function(e) { protocolError(e, testName, resultCallback); });
                }, function(e) { protocolError(e, testName, resultCallback); });
            } catch(error) {
                console.log("Caught exception running test sequence for protocolPipelineScalarBarTests");
                console.log(error);
                pipelineCleanup();
                resultCallback({ 'name': testName,
                                 'success': false,
                                 'message': error });
            }
        },

        /**
         * This function exercises the following protocol rpc methods:
         *
         * 'vtk:openRelativeFile'
         * 'vtk:colorBy'
         *
         * It needs access to the 'can.ex2' dataset in order to test
         * these protocols.
         */
        protocolColorByTests: function(testName, resultCallback) {
            var s = connection.session;

            try {
                s.call('vtk:openRelativeFile', 'can.ex2').then(function(dataProxy) {
                    var proxyId = dataProxy['proxy_id'];
                    var opts = { 'proxyId': proxyId,
                                 'arrayName': 'GlobalNodeId',
                                 'attributeType': 'POINTS' };
                    s.call('vtk:colorBy', opts).then(function(empty) {
                        s.call('vtk:getColoringInfo', proxyId).then(function(firstInfo) {
                            if (firstInfo['arrayName'] !== 'GlobalNodeId') {
                                pipelineCleanup();
                                msg = "Error: expected to be coloring by GlobalNodeId, " +
                                    "instead coloring by: " + firstInfo['arrayName']
                                resultCallback({ 'name': testName,
                                                 'success': false,
                                                 'message': msg });
                            }
                            opts['arrayName'] = 'ACCL';
                            opts['vectorMode'] = 'Component';
                            opts['vectorComponent'] = 0;
                            s.call('vtk:colorBy', opts).then(function(empty) {
                                s.call('vtk:getColoringInfo', proxyId).then(function(secondInfo) {
                                    if (secondInfo['arrayName'] !== opts['arrayName'] ||
                                        secondInfo['vectorMode'] !== opts['vectorMode'] ||
                                        secondInfo['vectorComponent'] != opts['vectorComponent']) {
                                        pipelineCleanup();
                                        msg = "Error: expected results arrayName => " + opts['arrayName'] + ", vectorMode => " + opts['vectorMode'] + ", vectorComponent => " + opts['vectorComponent'] + ".  Instead got arrayName => " + secondInfo['arrayName'] + ", vectorMode => " + secondInfo['vectorMode'] + ", vectorComponent => " + secondInfo['vectorComponent'] + ".";
                                        resultCallback({ 'name': testName,
                                                         'success': false,
                                                         'message': msg });
                                    } else {
                                        pipelineCleanup();
                                        resultCallback({ 'name': testName,
                                                         'success': true,
                                                         'message': "protocolColorByTests completed successfully" });
                                    }
                                }, function(e) { protocolError(e, testName, resultCallback); });
                            }, function(e) { protocolError(e, testName, resultCallback); });
                        }, function(e) { protocolError(e, testName, resultCallback); });
                    }, function(e) { protocolError(e, testName, resultCallback); });
                }, function(e) { protocolError(e, testName, resultCallback); });
            } catch(error) {
                console.log("Caught exception running test sequence for protocolColorByTests");
                console.log(error);
                pipelineCleanup();
                resultCallback({ 'name': testName,
                                 'success': false,
                                 'message': error });
            }
        },

        /**
         * This function exercises the following protocol rpc methods:
         *
         * 'vtk:openRelativeFile'
         * 'vtk:colorBy'
         * 'vtk:selectColorMap'
         *
         * It needs access to the 'can.ex2' dataset in order to test
         * these protocols.
         */
        protocolSelectColorMapTests: function(testName, resultCallback) {
            var s = connection.session;

            try {
                s.call('vtk:openRelativeFile', 'can.ex2').then(function(dataProxy) {
                    var proxyId = dataProxy['proxy_id'];
                    var opts = { 'proxyId': proxyId,
                                 'arrayName': 'DISPL',
                                 'attributeType': 'POINTS' };
                    s.call('vtk:colorBy', opts).then(function(empty) {
                        s.call('vtk:updateTime', 'last').then(function(lastTime) {
                            var rescaleOpts = { 'proxyId': proxyId,
                                                'type': 'data' };
                            s.call('vtk:rescaleTransferFunction', rescaleOpts).then(function(success) {
                                var mapOpts = { 'proxyId': proxyId,
                                                'arrayName': 'DISPL',
                                                'attributeType': 'POINTS',
                                                'presetName': 'Blue to Red Rainbow' };
                                                // 'presetName': 'Brewer Qualitative Set3' };
                                s.call('vtk:selectColorMap', mapOpts).then(function(testResult) {
                                    console.log('select color map result:');
                                    console.log(testResult);
                                    pipelineCleanup();
                                    resultCallback({ 'name': testName,
                                                     'success': true,
                                                     'message': "protocolSelectColorMapTests succeeded" });
                                }, function(e) { protocolError(e, testName, resultCallback); });
                            }, function(e) { protocolError(e, testName, resultCallback); });
                        }, function(e) { protocolError(e, testName, resultCallback); });
                    }, function(e) { protocolError(e, testName, resultCallback); });
                }, function(e) { protocolError(e, testName, resultCallback); });
            } catch(error) {
                console.log("Caught exception running test sequence for protocolSelectColorMapTests");
                console.log(error);
                pipelineCleanup();
                resultCallback({ 'name': testName,
                                 'success': false,
                                 'message': error });
            }
        },

        /**
         * This function exercises the following protocol rpc methods:
         *
         * 'vtk:listColorMapNames'
         */
        protocolListColorMapsTests: function(testName, resultCallback) {
            var s = connection.session,
            // Arbitrarily choose a few of the default ParaView presets
            // to check for.
            expectedPresets = ["Cool to Warm", "Blue to Red Rainbow", "Cold and Hot", "Rainbow Desaturated", "Wild Flower", "Brewer Sequential Yellow-Orange-Brown (9)"];

            try {
                s.call('vtk:listColorMapNames').then(function(list) {
                    var success = true;
                    var expectedButMissing = [];
                    for (var idx in expectedPresets) {
                        if ($.inArray(expectedPresets[idx], list) == -1) {
                            success = false;
                            expectedButMissing.push(expectedPresets[idx]);
                        }
                    }
                    var msg = '';
                    if (expectedButMissing.length > 0) {
                        msg = "Error missing expected presets: " + expectedPresets.join();
                    } else {
                        msg = "protocolListColorMapsTests succeeded";
                    }
                    pipelineCleanup();
                    resultCallback({ 'name': testName,
                                     'success': success,
                                     'message': msg });
                }, function(e) { protocolError(e, testName, resultCallback); });
            } catch(error) {
                console.log("Caught exception running test sequence for protocolListColorMapNamesTests");
                console.log(error);
                pipelineCleanup();
                resultCallback({ 'name': testName,
                                 'success': false,
                                 'message': error });
            }
        },

        /**
         * This function exercises the following protocol rpc methods:
         *
         * 'vtk:setLutDataRange'
         * 'vtk:getLutDataRange'
         *
         * This is a test of backwards compatibility of pipeline protocol
         * methods for setting and getting custom color range information.
         */
        protocolGetSetLutDataRangeTests: function(testName, resultCallback) {
            var s = connection.session;

            try {
                s.call('vtk:openRelativeFile', 'can.ex2').then(function(dataProxy) {
                    var proxyId = dataProxy['proxy_id'];
                    var opts = { 'proxy_id': proxyId,
                                 'ColorArrayName': 'DISPL',
                                 'ColorAttributeType': 'POINT_DATA' }
                    s.call('vtk:updateDisplayProperty', opts).then(function(empty) {
                        var expectedLastTime = 0.004299988504499197
                        s.call('vtk:updateTime', 'last').then(function(lastTime) {
                            if (!epsilonCompare(lastTime, expectedLastTime, compareTolerance)) {
                                throw "Error, the time at can's last timestep was: " +
                                    lastTime + ", expected: " + expectedLastTime;
                            }
                            var name = opts['ColorArrayName'];
                            var numComps = null;
                            var range = [ 5.0, 10.0 ];
                            s.call('vtk:setLutDataRange', name, numComps, range).then(function(empty) {
                                s.call('vtk:getLutDataRange', name, numComps).then(function(newRange) {
                                    if (checkRangeEqualWithTolerance(newRange,
                                                                     range,
                                                                     testName,
                                                                     resultCallback,
                                                                     "Color range incorrect after data rescale")) {
                                        pipelineCleanup();
                                        resultCallback({ 'name': testName,
                                                         'success': true,
                                                         'message': "protocolGetSetLutDataRangeTests succeeded" });
                                    }
                                }, function(e) { protocolError(e, testName, resultCallback); });
                            }, function(e) { protocolError(e, testName, resultCallback); });
                        }, function(e) { protocolError(e, testName, resultCallback); });
                    }, function(e) { protocolError(e, testName, resultCallback); });
                }, function(e) { protocolError(e, testName, resultCallback); });
            } catch(error) {
                console.log("Caught exception running test sequence for protocolGetSetLutDataRangeTests");
                console.log(error);
                pipelineCleanup();
                resultCallback({ 'name': testName,
                                 'success': false,
                                 'message': error });
            }
        }
    };
}
