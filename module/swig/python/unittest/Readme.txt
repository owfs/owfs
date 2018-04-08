$HeadURL: http://subversion/stuff/svn/owfs/trunk/unittest/Readme.txt $

Unittests

Status

    As of 2/7/2005, the OWSensor.testCacheUncached and
    OWSensor.testSensorList unittest will fail. This seems to be
    caused by a reading from the cached and then uncached trees. Under
    the right circumstances, it looks like the uncached tree doesn't
    show sensors connected to a ds2409 microlan controller. The
    problem has been reproduced with the Perl interface so it appears
    that it's not directly related to the Python module. But more work
    is needed to verify and resolve the issue.


Environment

    The unittests expect that ow has been installed into the Python
    installation such that "import ow" works. This can be accomplished
    via the "python setup.py install" command or by adding the
    directory that contains the ow module to the PYTHONPATH
    environment variable.

    For example, to setup the unittest environment on a linux system
    after OWFS has been built but before is has been installed:

       export PYTHONPATH=$(PATH_TO_OWFS)/module/swig/python/build/lib.linux-i686-2.3/


Configuration

    The unittests require a configuration file that defines the layout
    of the 1-wire network to be used in testing. The configuration
    file should be named owtest.ini and should exist in the unittest
    directory.

    See owtest_sample.ini for a description of the configuration file.


Running Unittests

    The owtest.py program is the main driver to run all unittests. It
    will look for all python modules in the current directory, include
    the test suites that it defines if the module's load variable is
    true.

    For example:

        root@think:/home/peter/src/owfs/module/swig/python/unittest> ./owtest.py
        testAttributes (ds1420.DS1420) ... ok
        testBaseAttributes (owsensors.OWSensors) ... ok
        testEntries (owsensors.OWSensors) ... ok
        testFindAll (owsensors.OWSensors) ... ok
        testFindAllNone (owsensors.OWSensors) ... ok
        testFindAnyNoValue (owsensors.OWSensors) ... ok
        testFindAnyNone (owsensors.OWSensors) ... ok
        testFindAnyValue (owsensors.OWSensors) ... ok
        testSensors (owsensors.OWSensors) ... ok

        ----------------------------------------------------------------------
        Ran 9 tests in 0.688s

        OK

    The individual test cases can also be run.

    For example:

        $ ./ds1420.py
        testAttributes (__main__.DS1420) ... ok

        ----------------------------------------------------------------------
        Ran 1 test in 0.117s

        OK
