

#include <juce_core/juce_core.h>

class Test_cello_ipc : public TestSuite
{
public:
    Test_cello_ipc() 
    : TestSuite("cello_ipc", "!!! category !!!")
    {

    }

    void runTest() override
    {
        beginTest("!!! WRITE SOME TESTS FOR THE cello_ipc Class !!!");

        /*
          To create a test, call `test("testName", testLambda);`
          To (temporarily) skip a test, call `skipTest("testName", testLambda);`
          To define setup for a block of tests, call `setup(setupLambda);`
          To define cleanup for a block of tests, call `tearDown(tearDownLambda);`

          Setup and TearDown lambdas will be called before/after each test that 
          is executed, and remain in effect until explicitly replaced. 

          All the functionality of the JUCE `UnitTest` class is available from
          within these tests. 
        */
    }
private:
    // !!! test class member vars here...
};

static Test_cello_ipc testcello_ipc;
