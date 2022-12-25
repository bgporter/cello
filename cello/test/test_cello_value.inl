

#include <complex>
#include <juce_core/juce_core.h>

#include "../cello_object.h"

#if 1
namespace juce
{
template <> struct VariantConverter<std::complex<float>>
{
    static std::complex<float> fromVar (const var& v)
    {
        if (const auto* array = v.getArray (); array != nullptr && array->size () == 2)
            return { array->getUnchecked (0), array->getUnchecked (1) };
        jassertfalse;
        return {};
    }

    static var toVar (const std::complex<float>& val)
    {
        Array<var> array;
        array.set (0, val.real ());
        array.set (1, val.imag ());
        return { std::move (array) };
    }
};

} // namespace juce
#endif
namespace
{
class ObjectWithOperators : public cello::Object
{
public:
    ObjectWithOperators ()
    : cello::Object ("opObject", nullptr)
    {
    }

    MAKE_VALUE_MEMBER (int, intVal, {});
    MAKE_VALUE_MEMBER (float, floatVal, {});
    MAKE_VALUE_MEMBER (juce::String, stringVal, {});
};

class ObjectWithConvertibleObject : public cello::Object
{
public:
    ObjectWithConvertibleObject ()
    : cello::Object ("convertible", nullptr)
    {
    }

    MAKE_VALUE_MEMBER (std::complex<float>, complexVal, {});
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

        test ("operator-- (pre/post)",
              [&] ()
              {
                  ObjectWithOperators o;
                  o.intVal = 5;
                  expect (4 == --o.intVal);
                  expect (o.intVal == 4);
                  expect (4 == o.intVal--);
                  expect (o.intVal == 3);
              });

        test ("variant conversion",
              [&] ()
              {
                  ObjectWithConvertibleObject o;
                  std::complex<float> orig { 2.f, 3.f };
                  o.complexVal = orig;

                  std::complex<float> retrieved { o.complexVal };
                  expectWithinAbsoluteError<float> (orig.real (), retrieved.real (),
                                                    0.001f);
                  expectWithinAbsoluteError<float> (orig.imag (), retrieved.imag (),
                                                    0.001f);
              });
    }

private:
    // !!! test class member vars here...
};

static Test_cello_value testcello_value;
