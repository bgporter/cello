

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
            stringVal.init ();
        }
    }

    MAKE_VALUE_MEMBER (int, intVal, {});
    MAKE_VALUE_MEMBER (float, floatVal, {});
    MAKE_VALUE_MEMBER (juce::String, stringVal, {});
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
        test ("operator +=",
              [&] ()
              {
                  ObjectWithOperators o;
                  o.intVal = 100;
                  expect (o.intVal == 100);
                  o.floatVal  = 3.14f;
                  o.stringVal = "this is a";
                  expect (!std::is_arithmetic<juce::String>::value);

                  // next line should NOT compile
                  //   o.stringVal += " test!";

                  o.intVal += 55;
                  expect (o.intVal == 155);
                  o.floatVal += 5.f;
                  expectWithinAbsoluteError<float> (o.floatVal, 8.14f, 0.001f);
              });

        test ("operator++ (pre/post)",
              [&] ()
              {
                  ObjectWithOperators o;
                  expect (1 == ++o.intVal);
                  expect (o.intVal == 1);
                  expect (1 == o.intVal++);
                  expect (o.intVal == 2);
              });
    }

private:
    // !!! test class member vars here...
};

static Test_cello_value testcello_value;
