

#include <juce_core/juce_core.h>

#include "../cello_object.h"

namespace
{
class ObjectWithOperators : public cello::Object
{
public:
    ObjectWithOperators ()
    : cello::Object ("opObject", nullptr)
    {
        if (initRequired)
        {
            intVal.init ();
            floatVal.init ();
        }
    }

    MAKE_VALUE_MEMBER (int, intVal, {});
    MAKE_VALUE_MEMBER (float, floatVal, {});
};

} // namespace

class Test_cello_value : public TestSuite
{
public:
    Test_cello_value ()
    : TestSuite ("cello_value", "cello")
    {
    }

    void runTest () override
    {
        beginTest ("!!! WRITE SOME TESTS FOR THE cello_value Class !!!");

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

        test ("operator +=",
              [&] ()
              {
                  ObjectWithOperators o;
                  o.intVal = 100;
                  expect (o.intVal == 100);
                  o.floatVal = 3.14f;

                  o.intVal += 55;
                  expect (o.intVal == 155);
                  o.floatVal += 5;
                  expectWithinAbsoluteError<float> (o.floatVal, 8.14f, 0.001f);
              });
    }

private:
    // !!! test class member vars here...
};

static Test_cello_value testcello_value;
