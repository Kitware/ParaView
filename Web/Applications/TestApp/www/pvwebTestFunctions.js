
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
        connection.session.call('pv.test.reset').then(function(result) {
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

        var message = errMsg;

        if (typeof(errMsg) !== 'string' && 'error' in errMsg && 'args' in errMsg) {
            message = "Protocol error: " + errMsg.error + '\n  ' + errMsg.args.join('\n  ');
        }

        callback({ 'name': testName,
                   'success': false,
                   'message': message });
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
         * This function tests the 'pv.files.list' protocol.  It assumes
         * that the pv_web_test_app.py server was started with a data dir
         * which contains the cloned ParaViewData repository.
         */
        protocolListFilesTest: function(testName, resultCallback) {
            try {
                connection.session.call('pv.files.list').then(function(obj) {
                    var returnValue = true;
                    var msg = "Successful test of protocol pv.files.list";

                    var names = obj.map(function(elt) {
                        return elt.name;
                    });

                    var expectedNames = [ 'sonic.pht', 'can.ex2', 'GMV'];

                    var errorMsg = "Error testing pv.files.list -- ";

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
         * 'pv.pipeline.manager.file.ropen'
         * 'pv.pipeline.manager.proxy.representation.update'
         * 'pv.color.manager.rescale.transfer.function'
         * 'pv.pipeline.manager.lut.range.get'
         * 'pv.vcr.action'
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
                s.call('pv.color.manager.rescale.transfer.function', [rescaleOpts]).then(function(success) {
                    s.call('pv.pipeline.manager.lut.range.get', ['DISPL', '']).then(function(range) {
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
                s.call('pv.vcr.action', ['last']).then(function(lastTime) {
                    if (!epsilonCompare(lastTime, expectedLastTime, compareTolerance)) {
                        var msg = "Error, the time at can's last timestep was: " +
                            lastTime + ", expected: " + expectedLastTime;
                        pipelineCleanup();
                        protocolError(msg, testName, resultCallback);
                        return;
                    }
                    var rescaleOpts = { 'proxyId': proxyId,
                                        'type': 'data' };
                    s.call('pv.color.manager.rescale.transfer.function', [rescaleOpts]).then(function(success) {
                        s.call('pv.pipeline.manager.lut.range.get', ['DISPL', '']).then(function(range) {
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
                s.call('pv.color.manager.rescale.transfer.function', [rescaleOpts]).then(function(success) {
                    s.call('pv.pipeline.manager.lut.range.get', ['DISPL', '']).then(function(newRange) {
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
                s.call('pv.pipeline.manager.file.ropen', ['can.ex2']).then(function(dataProxy) {
                    var proxyId = dataProxy['proxy_id'];
                    var opts = { 'proxy_id': proxyId,
                                 'ColorArrayName': 'DISPL',
                                 'ColorAttributeType': 'POINT_DATA' }
                    s.call('pv.pipeline.manager.proxy.representation.update', [opts]).then(function(empty) {
                        s.call('pv.pipeline.manager.lut.range.get', ['DISPL', '']).then(function(range) {
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
         * 'pv.pipeline.manager.file.ropen'
         * 'pv.pipeline.manager.proxy.representation.update'
         * 'pv.color.manager.scalarbar.visibility.get'
         * 'pv.color.manager.scalarbar.visibility.set'
         *
         * It needs access to the 'can.ex2' dataset in order to test
         * these protocols.
         */
        protocolScalarBarVisibilityTests: function(testName, resultCallback) {
            var s = connection.session;

            try {
                s.call('pv.pipeline.manager.file.ropen', ['can.ex2']).then(function(dataProxy) {
                    var proxyId = dataProxy['proxy_id'];
                    var opts = { 'proxy_id': proxyId,
                                 'ColorArrayName': 'DISPL',
                                 'ColorAttributeType': 'POINT_DATA' };
                    s.call('pv.pipeline.manager.proxy.representation.update', [opts]).then(function(empty) {
                        var opts = {};
                        opts[proxyId] = 'true';
                        s.call('pv.color.manager.scalarbar.visibility.set', [opts]).then(function(visibilities) {
                            // Either of these next two lines will work as a protocol parameter
                            var params = [proxyId];
                            //var params = opts;
                            s.call('pv.color.manager.scalarbar.visibility.get', [params]).then(function(vizzies) {
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
         * 'pv.pipeline.manager.file.ropen'
         * 'pv.pipeline.manager.proxy.representation.update'
         * 'pv.pipeline.manager.proxy.add'
         * 'pv.color.manager.scalarbar.visibility.get'
         * 'pv.color.manager.scalarbar.visibility.set'
         *
         * It needs access to the 'can.ex2' dataset in order to test
         * these protocols.
         */
        protocolPipelineScalarBarTests: function(testName, resultCallback) {
            var s = connection.session;

            try {
                s.call('pv.pipeline.manager.file.ropen', ['can.ex2']).then(function(dataProxy) {
                    var proxyIdOne = dataProxy['proxy_id'];
                    var opts = { 'proxy_id': proxyIdOne,
                                 'ColorArrayName': 'DISPL',
                                 'ColorAttributeType': 'POINT_DATA' };
                    s.call('pv.pipeline.manager.proxy.representation.update', [opts]).then(function(empty) {
                        s.call('pv.pipeline.manager.proxy.add', ['Clip', proxyIdOne]).then(function(clipProxy) {
                            var proxyIdTwo = clipProxy['proxy_id'];
                            opts['proxy_id'] = proxyIdTwo;
                            s.call('pv.pipeline.manager.proxy.representation.update', [opts]).then(function(empty) {
                                s.call('pv.pipeline.manager.proxy.add', ['Slice', proxyIdTwo]).then(function(sliceProxy) {
                                    var proxyIdThree = sliceProxy['proxy_id'];
                                    opts['proxy_id'] = proxyIdTwo;
                                    s.call('pv.pipeline.manager.proxy.representation.update', [opts]).then(function(empty) {
                                        var setSbOpts = {};
                                        setSbOpts[proxyIdThree] = 'true';
                                        s.call('pv.color.manager.scalarbar.visibility.set', [setSbOpts]).then(function(visibilities) {
                                            var getSbOpts = {};
                                            getSbOpts[proxyIdOne] = '';
                                            getSbOpts[proxyIdTwo] = '';
                                            getSbOpts[proxyIdThree] = '';
                                            s.call('pv.color.manager.scalarbar.visibility.get', [getSbOpts]).then(function(vizzies) {
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
         * 'pv.pipeline.manager.file.ropen'
         * 'pv.color.manager.color.by'
         *
         * It needs access to the 'can.ex2' dataset in order to test
         * these protocols.
         */
        protocolColorByTests: function(testName, resultCallback) {
            var s = connection.session;

            try {
                s.call('pv.pipeline.manager.file.ropen', ['can.ex2']).then(function(dataProxy) {
                    var proxyId = dataProxy['proxy_id'];
                    s.call('pv.test.repr.get', [proxyId]).then(function(reprStruct) {
                        var reprId = reprStruct['reprProxyId'];
                        var args = [reprId, 'ARRAY'];
                        var kwargs = { 'arrayLocation': 'POINTS', 'arrayName': 'GlobalNodeId'};
                        s.call('pv.color.manager.color.by', args, kwargs).then(function(empty) {
                            s.call('pv.test.color.info.get', [proxyId]).then(function(firstInfo) {
                                if (firstInfo['arrayName'] !== 'GlobalNodeId') {
                                    pipelineCleanup();
                                    msg = "Error: expected to be coloring by GlobalNodeId, " +
                                        "instead coloring by: " + firstInfo['arrayName']
                                    resultCallback({ 'name': testName,
                                                     'success': false,
                                                     'message': msg });
                                }
                                args = [reprId, 'ARRAY'];
                                kwargs = {'arrayLocation': 'POINTS', 'arrayName': 'ACCL',
                                          'vectorMode': 'Component', 'vectorComponent': 0};
                                s.call('pv.color.manager.color.by', args, kwargs).then(function(empty) {
                                    s.call('pv.test.color.info.get', [proxyId]).then(function(secondInfo) {
                                        if (secondInfo['arrayName'] !== kwargs['arrayName'] ||
                                            secondInfo['vectorMode'] !== kwargs['vectorMode'] ||
                                            secondInfo['vectorComponent'] != kwargs['vectorComponent']) {
                                            pipelineCleanup();
                                            msg = "Error: expected results arrayName => " + kwargs['arrayName'] + ", vectorMode => " + kwargs['vectorMode'] + ", vectorComponent => " + kwargs['vectorComponent'] + ".  Instead got arrayName => " + secondInfo['arrayName'] + ", vectorMode => " + secondInfo['vectorMode'] + ", vectorComponent => " + secondInfo['vectorComponent'] + ".";
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
         * 'pv.pipeline.manager.file.ropen'
         * 'pv.color.manager.color.by'
         * 'pv.color.manager.select.preset'
         *
         * It needs access to the 'can.ex2' dataset in order to test
         * these protocols.
         */
        protocolSelectColorMapTests: function(testName, resultCallback) {
            var s = connection.session;

            try {
                s.call('pv.pipeline.manager.file.ropen', ['can.ex2']).then(function(dataProxy) {
                    var proxyId = dataProxy['proxy_id'];
                    s.call('pv.test.repr.get', [proxyId]).then(function(reprStruct) {
                        var reprId = reprStruct['reprProxyId'];
                        var args = [reprId, 'ARRAY'];
                        var kwargs = { 'arrayLocation': 'POINTS', 'arrayName': 'DISPL'};
                        s.call('pv.color.manager.color.by', args, kwargs).then(function(empty) {
                            s.call('pv.vcr.action', ['last']).then(function(lastTime) {
                                var rescaleOpts = { 'proxyId': proxyId,
                                                    'type': 'data' };
                                s.call('pv.color.manager.rescale.transfer.function', [rescaleOpts]).then(function(success) {
                                    args = [reprId, 'Blue to Red Rainbow'];
                                    // 'presetName': 'Brewer Qualitative Set3' };
                                    s.call('pv.color.manager.select.preset',args).then(function(testResult) {
                                        pipelineCleanup();
                                        resultCallback({ 'name': testName,
                                                         'success': true,
                                                         'message': "protocolSelectColorMapTests succeeded" });
                                    }, function(e) { protocolError(e, testName, resultCallback); });
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
         * 'pv.color.manager.list.preset'
         */
        protocolListColorMapsTests: function(testName, resultCallback) {
            var s = connection.session,
            // Arbitrarily choose a few of the default ParaView presets
            // to check for.
            expectedPresets = ["Cool to Warm", "Blue to Red Rainbow", "Cold and Hot", "Rainbow Desaturated", "Wild Flower", "Brewer Sequential Yellow-Orange-Brown (9)"];

            try {
                s.call('pv.color.manager.list.preset').then(function(list) {
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
         * 'pv.pipeline.manager.lut.range.update'
         * 'pv.pipeline.manager.lut.range.get'
         *
         * This is a test of backwards compatibility of pipeline protocol
         * methods for setting and getting custom color range information.
         */
        protocolGetSetLutDataRangeTests: function(testName, resultCallback) {
            var s = connection.session;

            try {
                s.call('pv.pipeline.manager.file.ropen', ['can.ex2']).then(function(dataProxy) {
                    var proxyId = dataProxy['proxy_id'];
                    var opts = { 'proxy_id': proxyId,
                                 'ColorArrayName': 'DISPL',
                                 'ColorAttributeType': 'POINT_DATA' }
                    s.call('pv.pipeline.manager.proxy.representation.update', [opts]).then(function(empty) {
                        var expectedLastTime = 0.004299988504499197;
                        s.call('pv.vcr.action', ['last']).then(function(lastTime) {
                            if (!epsilonCompare(lastTime, expectedLastTime, compareTolerance)) {
                                var msg = "Error, the time at can's last timestep was: " +
                                    lastTime + ", expected: " + expectedLastTime;
                                pipelineCleanup();
                                protocolError(msg, testName, resultCallback);
                                return;
                            }
                            var name = opts['ColorArrayName'];
                            var numComps = null;
                            var range = [ 5.0, 10.0 ];
                            s.call('pv.pipeline.manager.lut.range.update', [name, numComps, range]).then(function(empty) {
                                s.call('pv.pipeline.manager.lut.range.get', [name, numComps]).then(function(newRange) {
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
        },

        /**
         * This function exercises the following protocol rpc methods:
         *
         * 'pv.proxy.manager.create'
         * 'pv.proxy.manager.get'
         * 'pv.proxy.manager.list'
         * 'pv.proxy.manager.delete'
         *
         * This test creates three filters in a pipeline and then checks
         * the properties of each proxy for existence and to ensure that
         * certain relationships hold.  It tests the list proxies function
         * and then tests the delete proxy functionality.
         */
        protocolProxyCreateGetDeleteTests: function(testName, resultCallback) {
            var s = connection.session,
            expectedWaveletProps = [ "Center", "Maximum", "StandardDeviation",
                                     "SubsampleRate", "WholeExtent", "XFreq",
                                     "XMag", "YFreq", "YMag", "ZFreq", "ZMag" ],
            expectedContourProps = [ "ComputeGradients", "ComputeNormals", "ComputeScalars",
                                     "ContourValues", "GenerateTriangles", "Divisions",
                                     "NumberOfPointsPerBucket", "MaxPointsPerLeaf",
                                     "Tolerance", "Locator", "SelectInputScalars"],
            expectedClipProps = [ "Normal", "Offset", "Origin", "Bounds", "Position",
                                  "Rotation", "Scale", "Center", "Radius", "ClipFunction",
                                  "PreserveInputCells", "SelectInputScalars", "Value" ],
            clipDependencies = { "Box": ["Bounds", "Position", "Rotation", "Scale"],
                                 "Sphere": ["Center", "Radius"],
                                 "Plane": ["Normal", "Offset", "Origin"] },
            contourDependencies = { "Don't Merge Points": [ "Divisions", "NumberOfPointsPerBucket" ],
                                    "Octree Binning": ["Tolerance", "MaxPointsPerLeaf"],
                                    "Uniform Binning": [ "Divisions", "NumberOfPointsPerBucket" ] };

            function checkExtendedProperties(proxyObj, expectedDependencies, proxyName) {
                var targetProxy = null,
                targetUi = null,
                expectedProps = {};
                // find the proxyName property
                for (var pidx in proxyObj.properties) {
                    if (proxyObj.properties[pidx]['name'] === proxyName) {
                        targetProxy = proxyObj.properties[pidx];
                        targetUi = proxyObj.ui[pidx];
                        break;
                    }
                }
                if (targetProxy === null || targetUi === null) {
                    return [ false, 'Expected to find a proxy named ' + proxyName ];
                }
                var vals = targetUi['values'];
                for (var key in expectedDependencies) {
                    if (!(key in vals)) {
                        return [ false, proxyName + ' expected possible value ' + key ];
                    }
                    expectedProps[vals[key]] = key;
                }
                for (var pidx in proxyObj.properties) {
                    var prop = proxyObj.properties[pidx];
                    if (prop['id'] in expectedProps) {
                        var dependents = expectedDependencies[expectedProps[prop['id']]];
                        var idxOf = dependents.indexOf(prop['name']);
                        if (idxOf >= 0) {
                            dependents.splice(idxOf, 1);
                        } else {
                            var msg = 'Expected find proxy ' + prop['name'] +
                                ' in dependents of ' + expectedProps[prop['id']];
                            return [ false, msg ];
                        }
                    }
                }
                var missing = [];
                for (var key in expectedDependencies) {
                    missing.concat(expectedDependencies[key])
                }
                if (missing.length != 0) {
                    var msg = 'Proxy ' + proxyObj['id'] + ' missing expected ' +
                        'dependent properties: ' + missing.join(',')
                    return [ false, msg ];
                }
                return [ true, '' ];
            }

            function checkBasicProperties(proxyObj, expectedPropNames) {
                var expectedKeys = ['name', 'id', 'value'];
                for (var pidx in proxyObj.properties) {
                    var prop = proxyObj.properties[pidx];
                    for (var i = 0; i < expectedKeys.length; ++i) {
                        if (!(expectedKeys[i] in prop)) {
                            return [ false, 'property at index ' + pidx + ' has no ' + expectedKeys[i] ];
                        }
                    }
                    var indexOfExpected = expectedPropNames.indexOf(prop['name']);
                    if (indexOfExpected >= 0) {
                        expectedPropNames.splice(indexOfExpected, 1);
                    }
                }
                if (expectedPropNames.length > 0) {
                    return [ false, 'Did not find the following expected properties: ' +
                             expectedPropNames.join(',') ];
                }
                return [ true, '' ];
            }

            try {
                var waveletArgs = ['Wavelet', -1],
                expectedParents = {};
                s.call('pv.proxy.manager.create', waveletArgs).then(function(waveletProxy) {
                    var waveletOk = checkBasicProperties(waveletProxy, expectedWaveletProps);
                    if (waveletOk[0] === false) {
                        pipelineCleanup();
                        var waveletErrMsg = "Wavelet proxy did not pass basic checks: " + waveletOk[1];
                        protocolError(waveletErrMsg, testName, resultCallback);
                        return;
                    }
                    var waveletPid = waveletProxy['id'];
                    expectedParents[waveletPid] = "0";
                    var contourArgs = ['Contour', waveletPid];
                    s.call('pv.proxy.manager.create', contourArgs).then(function(contourProxy) {
                        var contourOk = checkBasicProperties(contourProxy, expectedContourProps);
                        if (contourOk[0] === false) {
                            pipelineCleanup();
                            var contourErrMsg = 'Contour proxy did not pass basic checks: ' + contourOk[1];
                            protocolError(contourErrMsg, testName, resultCallback);
                            return;
                        }
                        contourOk = checkExtendedProperties(contourProxy, contourDependencies, "Locator");
                        if (contourOk[0] === false) {
                            pipelineCleanup();
                            var contourErrMsg = 'Contour proxy did not pass extended checks: ' + contourOk[1];
                            protocolError(contourErrMsg, testName, resultCallback);
                            return;
                        }
                        var contourPid = contourProxy['id'];
                        expectedParents[contourPid] = waveletPid;
                        var clipArgs = ['Clip', contourPid];
                        s.call('pv.proxy.manager.create', clipArgs).then(function(clipProxy) {
                            var clipOk = checkBasicProperties(clipProxy, expectedClipProps);
                            if (clipOk[0] === false) {
                                pipelineCleanup();
                                var clipErrMsg = 'Clip proxy did not pass basic checks: ' + clipOk[1];
                                protocolError(clipErrMsg, testName, resultCallback);
                                return;
                            }
                            clipOk = checkExtendedProperties(clipProxy, clipDependencies, "ClipFunction");
                            if (clipOk[0] === false) {
                                pipelineCleanup();
                                var clipErrMsg = 'Clip proxy did not pass extended checks: ' + clipOk[1];
                                protocolError(clipErrMsg, testName, resultCallback);
                                return;
                            }
                            var clipPid = clipProxy['id'];
                            expectedParents[clipPid] = contourPid;
                            s.call('pv.proxy.manager.list').then(function(proxyList) {
                                for (var idx in proxyList) {
                                    var elt = proxyList[idx];
                                    if (elt['parent'] != expectedParents[elt['id']]) {
                                        var message = "protocolProxyCreateGetDeleteTests failed because at " +
                                            "least one proxy in the list had wrong parent: " +
                                            "proxy " + elt['id'] + " had parent " + elt['parent'] +
                                            ", but expected " + expectedParents[elt['id']];
                                        pipelineCleanup();
                                        resultCallback({ 'name': testName,
                                                         'success': false,
                                                         'message': message });
                                        return;
                                    }
                                }
                                s.call('pv.proxy.manager.delete', [contourPid]).then(function(result) {
                                    if (result['success'] === 1) {
                                        var message = "Should not have been able to delete proxy " +
                                            contourPid + " while it is the input to another filter " +
                                            "proxy".
                                        pipelineCleanup();
                                        resultCallback({ 'name': testName,
                                                         'success': false,
                                                         'message': message });
                                        return;
                                    }
                                    s.call('pv.proxy.manager.delete', [clipPid]).then(function(result) {
                                        if (result['success'] === 0) {
                                            pipelineCleanup();
                                            resultCallback({ 'name': testName,
                                                             'success': false,
                                                             'message': "Unable to delete clip proxy" });
                                            return;
                                        }
                                        s.call('pv.proxy.manager.delete', [contourPid]).then(function(r) {
                                            if (r['success'] === 0) {
                                                var m = "Unable to delete contour proxy";
                                                pipelineCleanup();
                                                resultCallback({ 'name': testName,
                                                                 'success': false,
                                                                 'message': m });
                                                return;
                                            }
                                            s.call('pv.proxy.manager.delete', [waveletPid]).then(function(d) {
                                                if (d['success'] === 0) {
                                                    var m = "Unable to delete wavelet proxy";
                                                    pipelineCleanup();
                                                    resultCallback({ 'name': testName,
                                                                     'success': false,
                                                                     'message': m });
                                                    return;
                                                }
                                                s.call('pv.proxy.manager.list').then(function(emptyList) {
                                                    if (emptyList.length > 0) {
                                                        var m = "All proxies should have been deleted, " +
                                                            "list still contains " + emptyList.join(",");
                                                        pipelineCleanup();
                                                        resultCallback({ 'name': testName,
                                                                         'success': false,
                                                                         'message': m });
                                                        return;
                                                    }
                                                    var m = "protocolProxyCreateGetDeleteTests succeeded";
                                                    pipelineCleanup();
                                                    resultCallback({ 'name': testName,
                                                                     'success': true,
                                                                     'message': m });
                                                }, function(e) { protocolError(e, testName, resultCallback); });
                                            }, function(e) { protocolError(e, testName, resultCallback); });
                                        }, function(e) { protocolError(e, testName, resultCallback); });
                                    }, function(e) { protocolError(e, testName, resultCallback); });
                                }, function(e) { protocolError(e, testName, resultCallback); });
                            }, function(e) { protocolError(e, testName, resultCallback); });
                        }, function(e) { protocolError(e, testName, resultCallback); });
                    }, function(e) { protocolError(e, testName, resultCallback); });
                }, function(e) { protocolError(e, testName, resultCallback); });
            } catch(error) {
                console.log("Caught exception running test sequence for protocolProxyCreateGetDeleteTests");
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
         * 'pv.proxy.manager.create'
         * 'pv.proxy.manager.update'
         *
         * This tests checks that the update function actually updates
         * the intended properties.
         */
        protocolProxyUpdateTests: function(testName, resultCallback) {
            var s = connection.session;

            try {
                var waveletArgs = ['Wavelet', -1];
                s.call('pv.proxy.manager.create', waveletArgs).then(function(waveletProxy) {
                    var waveletPid = waveletProxy['id'];
                    var clipArgs = ['Clip', waveletPid];
                    s.call('pv.proxy.manager.create', clipArgs).then(function(clipProxy) {
                        var clipPid = clipProxy['id'];
                        for (var idx in clipProxy.properties) {
                            var property = clipProxy.properties[idx];
                            if (property['name'] === "Radius") {
                                property['value'] = 3.0;
                            } else if (property['name'] === "ClipFunction") {
                                property['value'] = "Sphere";
                            } else if (property['name'] === "PreserveInputCells") {
                                property['value'] = 1;
                            } else if (property['name'] === "InsideOut") {
                                property['value'] = 1;
                            }
                        }
                        s.call('pv.proxy.manager.update', [clipProxy.properties]).then(function(result) {
                            var m = "protocolProxyUpdateTests succeeded";
                            var success = true;
                            if (!('success' in result) || result['success'] !== true) {
                                success = false;
                                if ('errorList' in result) {
                                    m = "Proxy update failed: " + result['errorList'].join(',');
                                }
                                m = "Proxy update failed, no further information";
                            }
                            pipelineCleanup();
                            resultCallback({ 'name': testName,
                                             'success': success,
                                             'message': m });
                        }, function(e) { protocolError(e, testName, resultCallback); });
                    }, function(e) { protocolError(e, testName, resultCallback); });
                }, function(e) { protocolError(e, testName, resultCallback); });
            } catch(error) {
                console.log("Caught exception running test sequence for protocolProxyUpdateTests");
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
         * 'pv.proxy.manager.create'
         * 'pv.proxy.manager.update'
         *
         * This function checks that updating properties which don't
         * actually exist results in the proper error message.
         */
        protocolProxyUpdateFailTests: function(testName, resultCallback) {
            var s = connection.session;

            try {
                var waveletArgs = ['Wavelet', -1];
                s.call('pv.proxy.manager.create', waveletArgs).then(function(waveletProxy) {
                    var waveletPid = waveletProxy['id'];
                    var clipArgs = ['Clip', waveletPid];
                    s.call('pv.proxy.manager.create', clipArgs).then(function(clipProxy) {
                        var updateProps = [ { "id": clipProxy['id'],
                                              "name": "NonexistentPropertyName",
                                              "value": "empty" },
                                            { "id": clipProxy['id'],
                                              "name": "AnotherFakeOne",
                                              "value": [ 0.0, 0.0, 0.0 ] } ];
                        s.call('pv.proxy.manager.update', [updateProps]).then(function(newProps) {
                            var m = "protocolProxyUpdateFailTests succeeded";
                            var success = true;
                            if (!('errorList' in newProps)) {
                                success = false;
                                m = "Expected this update to return an error";
                            }
                            pipelineCleanup();
                            resultCallback({ 'name': testName,
                                             'success': success,
                                             'message': m });
                        }, function(e) { protocolError(e, testName, resultCallback); });
                    }, function(e) { protocolError(e, testName, resultCallback); });
                }, function(e) { protocolError(e, testName, resultCallback); });
            } catch(error) {
                console.log("Caught exception running test sequence for protocolProxyUpdateFailTests");
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
         * 'pv.proxy.manager.create'
         * 'pv.proxy.manager.update'
         *
         * This test checks that getting data info with a proxy object
         * returns the expected information.
         */
        protocolProxyGetDataInfoTests: function(testName, resultCallback) {
            var s = connection.session,
            expectedInfo =   {
                "ACCL": {
                    "range": {
                        "magnitude": [ 0.0, 1.3329898584157294e-05 ],
                        "components": [ { "range": [ -4.965284006175352e-07, 3.212448973499704e-07 ],
                                          "name": "X"
                                        },
                                        { "range": [ -5.920087460253853e-07, 5.920081207477779e-07 ],
                                          "name": "Y"
                                        },
                                        { "range": [ -1.3328498425835278e-05, 1.0021663911174983e-05 ],
                                          "name": "Z"
                                        }
                                      ]
                    },
                    "type": "POINT",
                    "name": "ACCL",
                    "size": 3
                },
                "DISPL": {
                    "range": {
                        "magnitude": [ 0.0, 0.0 ],
                        "components": [ { "range": [ 0.0, 0.0 ],
                                          "name": "X"
                                        },
                                        { "range": [ 0.0, 0.0 ],
                                          "name": "Y"
                                        },
                                        { "range": [ 0.0, 0.0 ],
                                          "name": "Z"
                                        }
                                      ]
                    },
                    "type": "POINT",
                    "name": "DISPL",
                    "size": 3
                },
                "GlobalNodeId":  {
                    "range": {
                        "magnitude": [ 1.0, 10088.0 ]
                    },
                    "type": "POINT",
                    "name": "GlobalNodeId",
                    "size": 1
                },
                "PedigreeNodeId": {
                    "range": {
                        "magnitude": [ 1.0, 10088.0 ]
                    },
                    "type": "POINT",
                    "name": "PedigreeNodeId",
                    "size": 1
                },
                "VEL": {
                    "range": {
                        "magnitude": [ 0.0, 5000.0 ],
                        "components": [ { "range": [ 0.0, 0.0 ],
                                          "name": "X"
                                        },
                                        { "range": [ 0.0, 0.0 ],
                                          "name": "Y"
                                        },
                                        { "range": [ -5000.0, 0.0 ],
                                          "name": "Z"
                                        }
                                      ]
                    },
                    "type": "POINT",
                    "name": "VEL",
                    "size": 3
                },
                "EQPS": {
                    "range": {
                        "magnitude": [ 0.0, 0.0 ]
                    },
                    "type": "CELL",
                    "name": "EQPS",
                    "size": 1
                },
                "GlobalElementId": {
                    "range": {
                        "magnitude": [ 1.0, 7152.0 ]
                    },
                    "type": "CELL",
                    "name": "GlobalElementId",
                    "size": 1
                },
                "ObjectId": {
                    "range": {
                        "magnitude": [ 1.0, 2.0 ]
                    },
                    "type": "CELL",
                    "name": "ObjectId",
                    "size": 1
                },
                "PedigreeElementId": {
                    "range": {
                        "magnitude": [ 1.0, 7152.0 ]
                    },
                    "type": "CELL",
                    "name": "PedigreeElementId",
                    "size": 1
                }
            };

            function compareToExpected(arrayInfo, tname, cback) {
                var expected = expectedInfo[arrayInfo['name']];
                if (expected['type'] !== arrayInfo['type'] ||
                    expected['size'] !== arrayInfo['size']) {
                    pipelineCleanup();
                    cback({ 'name': tname,
                            'success': false,
                            'message': 'Size or type are not equal for ' + arrayInfo['name'] });
                    return false;
                }
                if (!checkRangeEqualWithTolerance(expected['range']['magnitude'],
                                                  arrayInfo['range']['magnitude'],
                                                  testName,
                                                  resultCallback,
                                                  "Data info magnitude range for " + arrayInfo['name'] + " incorrect")) {
                    // Only carry on if the last check was successful/true
                    return false;
                }
                return true;
            }

            try {
                s.call('pv.pipeline.manager.file.ropen', ['can.ex2']).then(function(dataProxy) {
                    var canProxyId = dataProxy['proxy_id'];
                    s.call('pv.proxy.manager.get', [canProxyId]).then(function(infoProxy) {
                        var arrayData = infoProxy['data']['arrayData'];
                        for (var i in arrayData) {
                            var arrayInfo = arrayData[i];
                            var ok = compareToExpected(arrayInfo, testName, resultCallback);
                            if (ok == false) {
                                return;
                            }
                        }
                        var m = "protocolProxyGetDataInfoTests succeeded";
                        var success = true;
                        pipelineCleanup();
                        resultCallback({ 'name': testName,
                                         'success': success,
                                         'message': m });
                    }, function(e) { protocolError(e, testName, resultCallback); });
                }, function(e) { protocolError(e, testName, resultCallback); });
            } catch(error) {
                console.log("Caught exception running test sequence for protocolProxyGetDataInfoTests");
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
         * 'pv.proxy.manager.create'
         *
         * This function checks that an attempt to create a proxy which is not
         * in the allowed list generates an error response.
         */
        protocolProxyIllegalCreateTests: function(testName, resultCallback) {
            var s = connection.session;

            try {
                var waveletArgs = ['Wavelet', -1];
                s.call('pv.proxy.manager.create', waveletArgs).then(function(waveletProxy) {
                    var waveletPid = waveletProxy['id'];
                    var sliceArgs = ['Compute Derivatives', waveletPid];
                    s.call('pv.proxy.manager.create', sliceArgs).then(function(whoopsError) {
                        var m = "protocolProxyIllegalCreateTests succeeded";
                        var success = true;
                        if (whoopsError['success'] !== false) {
                            success = false;
                            m = "Expected 'Compute Derivatives' creation to return an error";
                        }
                        pipelineCleanup();
                        resultCallback({ 'name': testName,
                                         'success': success,
                                         'message': m });
                    }, function(e) { protocolError(e, testName, resultCallback); });
                }, function(e) { protocolError(e, testName, resultCallback); });
            } catch(error) {
                console.log("Caught exception running test sequence for protocolProxyIllegalCreateTests");
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
         * 'pv.proxy.manager.available'
         *
         * This function checks that the 'available' rpc method returns
         * a properly formatted list of the available sources and filters.
         */
        protocolProxyAvailableTests: function(testName, resultCallback) {
            var s = connection.session,
            expectedFilters = ["Slice", "Warp By Scalar", "Clip", "Cell Data To Point Data", "Calculator", "Transform", "Extract CTH Parts", "Reflect", "Stream Tracer", "Threshold", "Contour"],
            expectedSources = ["Box", "Cylinder", "Sphere", "Plane", "Cone", "Annotate Time", "Wavelet"];

            function findMissingElements(expected, actual) {
                var missing = [];
                for (var idx in expected) {
                    if ($.inArray(expected[idx], actual) < 0) {
                        missing.push(expectedFilters[idx]);
                    }
                }
                return missing;
            }

            try {
                s.call('pv.proxy.manager.available', ['filters']).then(function(filterList) {
                    var missing = findMissingElements(expectedFilters, filterList);
                    if (missing.length > 0) {
                        pipelineCleanup();
                        resultCallback({ 'name': testName,
                                         'success': false,
                                         'message': 'Missing expected filters: ' + missing.join(',') });
                        return;
                    }
                    s.call('pv.proxy.manager.available', ['sources']).then(function(sourceList) {
                        missing = findMissingElements(expectedSources, sourceList);
                        if (missing.length > 0) {
                            pipelineCleanup();
                            resultCallback({ 'name': testName,
                                             'success': false,
                                             'message': 'Missing expected sources: ' + missing.join(',') });
                            return;
                        }
                        pipelineCleanup();
                        resultCallback({ 'name': testName,
                                         'success': true,
                                         'message': "protocolProxyAvailableTests succeeded" });
                    }, function(e) { protocolError(e, testName, resultCallback); });
                }, function(e) { protocolError(e, testName, resultCallback); });
            } catch(error) {
                console.log("Caught exception running test sequence for protocolProxyAvailableTests");
                console.log(error);
                pipelineCleanup();
                resultCallback({ 'name': testName,
                                 'success': false,
                                 'message': error });
            }
        }
    };
}
