/*
    Copyright (c) 2023 Brett g Porter
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include <complex>
#include <juce_core/juce_core.h>

#include "../cello_object.h"
#include "../cello_value.h"

namespace juce
{
/**
 * @brief A variant converter template specialization for
 * std::complex<float> <--> & juce::var.
 * VariantConverter structs need to be in the juce namespace for
 * them to work properly.
 */
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

namespace
{
class ObjectWithOperators : public cello::Object
{
public:
    ObjectWithOperators ()
    : cello::Object ("opObject", nullptr)
    {
    }

    // members public so we can verify arithmetic operators
    MAKE_VALUE_MEMBER (int, intVal, {});
    MAKE_VALUE_MEMBER (float, floatVal, {});
    MAKE_VALUE_MEMBER (juce::String, stringVal, {});
};

class ObjectWithConvertibleValue : public cello::Object
{
public:
    ObjectWithConvertibleValue ()
    : cello::Object ("convertible", nullptr)
    {
    }
    // verify the automatic use of variant converters.
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
                  ObjectWithConvertibleValue o;
                  std::complex<float> orig { 2.f, 3.f };
                  o.complexVal = orig;

                  std::complex<float> retrieved { o.complexVal };
                  expectWithinAbsoluteError<float> (orig.real (), retrieved.real (), 0.001f);
                  expectWithinAbsoluteError<float> (orig.imag (), retrieved.imag (), 0.001f);

                  // test using the `get()` method into an `auto` variable
                  auto retrieved2 { o.complexVal.get () };
                  expectWithinAbsoluteError<float> (orig.real (), retrieved2.real (), 0.001f);
                  expectWithinAbsoluteError<float> (orig.imag (), retrieved2.imag (), 0.001f);
              });

        test ("Cached value",
              [this] ()
              {
                  ObjectWithOperators obj;
                  // can create verbosely
                  cello::Value<float>::Cached cachedFloat (obj.floatVal);
                  // ...or just have the Value object return us one
                  auto cachedInt { obj.intVal.getCached () };

                  int updateCount { 0 };
                  // use the get validation function to increment a counter each time the
                  // value is actually retrieved, which will happen automatically when
                  // the cached value is updated on the value changing.
                  obj.intVal.onGet = [&updateCount] (const int& v)
                  {
                      ++updateCount;
                      return v;
                  };

                  expectEquals (static_cast<int> (cachedInt), 0);
                  expectEquals (updateCount, 0);
                  obj.intVal = 100;
                  expectEquals (static_cast<int> (cachedInt), 100);
                  expectEquals (updateCount, 1);
                  expectEquals (static_cast<int> (cachedInt), 100);
                  expectEquals (updateCount, 1);
                  obj.intVal = 100;
                  expectEquals (static_cast<int> (cachedInt), 100);
                  expectEquals (updateCount, 1);
                  obj.intVal = 200;
                  expectEquals (static_cast<int> (cachedInt), 200);
                  expectEquals (updateCount, 2);
              });

        test ("Cached member",
              [&] ()
              {
                  struct TestObject
                  {
                      ObjectWithConvertibleValue owcv;
                      CACHED_VALUE (cachedVal, owcv.complexVal);
                      // expands to
                      // decltype(owcv.complexVal.getCached()) cachedVal { owcv.complexVal };
                      // which expands to 
                      // cello::Value<std::complex<float>>::Cached cachedVal { owcv.complexVal };
                  };

                  TestObject to;

                  to.owcv.complexVal = { 1.f, 2.f };
                  expect (to.owcv.complexVal.get () == to.cachedVal.get ());
              });
    }

private:
    // !!! test class member vars here...
};

static Test_cello_value testcello_value;
