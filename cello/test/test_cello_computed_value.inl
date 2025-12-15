/*
    Copyright (c) 2025 Brett g Porter
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


#include <juce_core/juce_core.h>
#include "../cello_object.h"
#include "../cello_value.h"

class ObjectWithArea : public cello::Object
{
public:
    ObjectWithArea()
    : cello::Object("area", nullptr)
    {
    }
    MAKE_VALUE_MEMBER (float, width, 0.f);
    MAKE_VALUE_MEMBER (float, height, 0.f);
    cello::ComputedValue<float> area { *this, "area", [this] () -> float { return width * height; } };
};

class ObjectWithConversion : public cello::Object
{
public:
    ObjectWithConversion()
    : cello::Object("conversion", nullptr)
    {
    }
    MAKE_VALUE_MEMBER (float, metric, 0.f);
    cello::ComputedValue<float> imperial { *this, "imperial", [this] () -> float { return metric / 2.54f; }, [this] (const float& val) { metric = val * 2.54f; } };
};

class Test_ComputedValue : public TestSuite
{
public:
    Test_ComputedValue() 
    : TestSuite("computed_value", "cello")
    {

    }

    void runTest() override
    {
        test("Read-only computed value", [&] ()
        {
            ObjectWithArea obj;
            obj.width = 10.f;
            obj.height = 20.f;
            expectWithinAbsoluteError (obj.area.get(), 200.f, 0.001f);
            obj.height = 30.f;
            expectWithinAbsoluteError (obj.area.get(), 300.f, 0.001f);

            // asserts because we didn't set the setImpl lambda
            // obj.area = 400.f;
            obj.area.getImpl = nullptr;
            // accessing a ComputedValue with no getImpl lambda will assert
            // expectWithinAbsoluteError (obj.area.get(), 300.f, 0.001f);

        });

        test ("bi-directional computed value", [&] ()
        {
            ObjectWithConversion obj;
            obj.metric = 100.f;
            expectWithinAbsoluteError (obj.imperial.get(), 39.3701f, 0.001f);
            obj.imperial = 10.f;
            expectWithinAbsoluteError (obj.metric.get(), 25.4f, 0.001f);
        });
    }
};

static Test_ComputedValue testComputedValue;