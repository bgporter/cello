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

#pragma once

#include <optional>

#include "cello_value.h"

namespace cello
{

/**
 * @class ComputedValue
 *
 * @brief A Value-like object that lets us use parts of the API we have
 * for Value objects (the get and set parts), generating a computed value
 * based on whatever is needed for your application.
 *
 * There's a (required) lambda for computing the value that's called when
 * we perform a `get()` operation.
 *
 * You can provide a separate lambda to be called when setting the value;
 * this should perform the reverse operation of the get lambda. As a simple example,
 * consider a case where we store data in metric units, but want to display it in imperial units.
 * We can create a ComputedValue<double> with a get lambda that converts the data
 * from metric to imperial, and a set lambda that converts the data from imperial to metric.
 *
 * This class is (intentionally) simpler than a Value object, in that it doesn't
 * permit listening to changes in the computed value. If you need to know when
 * the computed value changes, add a listener to the Value used as the source of the computed value.
 *
 * This also doesn't support the `onSet` and `onGet` validation functions that are in
 * Value objects; any validation that you need to perform should be done in the get and set lambdas.
 *
 * @tparam T The type of the computed value.
 */
template <typename T> class ComputedValue : public ValueBase
{
public:
    /**
     * @brief The type of the function that will be called to set the value of the computed value.
     */
    using SetImplFn = std::function<void (const T&)>;
    using GetImplFn = std::function<T ()>;

    ComputedValue (Object& object, const juce::Identifier& id, GetImplFn getImpl = nullptr, SetImplFn setImpl = nullptr)
    : ValueBase { id }
    , getImpl { getImpl }
    , setImpl { setImpl }
    , object { object }
    {
    }

    /**
     * @brief Assignment operator for the computed value.
     *
     * @param val The new value to assign to the computed value.
     * @return ComputedValue& reference to this value so you could e.g. do
     * `this.x = this.y = 42;`
     *
     * @note This operator will call the `setImpl` lambda if it is set, otherwise it will assert.
     */
    ComputedValue& operator= (const T& val)
    {
        set (val);
        return *this;
    }

    /**
     * @brief Set the value of the computed value.
     *
     * @param val The new value to set the computed value to.
     * @note This operator will call the `setImpl` lambda if it is set,
     *      otherwise it will assert. Note that the assertion does not necessarily mean
     *      "hey, you forgot to set the setImpl lambda!", it means "hey, this
     *      is a read-only computed value, and you're trying to set it."
     */
    void set (const T& val)
    {
        if (setImpl != nullptr)
            setImpl (val);
        else
            jassertfalse;
    }

    /**
     * @brief Conversion operator to the type of the computed value.
     *
     * @return T The current value of the computed value.
     */
    operator T () const { return get (); }

    /**
     * @brief Get the current value of the computed value.
     *
     * @return T The current value of the computed value.
     */
    T get () const
    {
        if (getImpl != nullptr)
            return getImpl ();
        else
            jassertfalse;
        return {};
    }

    GetImplFn getImpl;
    SetImplFn setImpl;

private:
    Object& object;
};

} // namespace cello
