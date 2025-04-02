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

#pragma once

#include "cello_update_source.h"

namespace cello
{

class Object;

class ValueBase : public UpdateSource
{
public:
    /**
     * @return this value's type ID.
     */
    juce::Identifier getId () const { return id; }

protected:
    /**
     * @brief ctor is protected -- you can't create an object of type
     * `ValueBase` directly, this only exists so we have a common base type that's
     * shared by all the templated Value<T> classes.
     *
     * @param id_ type identifier of this value.
     */
    ValueBase (const juce::Identifier& id_)
    : id { id_ }
    {
    }

protected:
    /// identifier of this value/property.
    const juce::Identifier id;
};

/**
 * @brief A class to abstract away the issues around storing and retrieving
 * a value from a ValueTree. Designed to make working with VT values more
 * like working with regular class/struct members.
 *
 * Data types to be stored as Values must:
 * - have an `operator !=` (so we can execute change callbacks)
 * - be supported by the `juce::var` type, or define a
 *   `juce::VariantConverter` structure to round-trip through a `juce::var`
 *
 * **NOTE** that we have a special case for floating point types -- we compare
 * the old and new versions of the value with a small epsilon value to let your
 * code control how 'close' two floating point values must be to be considered
 * equivalent. There's a static `epsilon` member of this class that you can
 * set as needed in your application; the default is 0.001.
 *
 * @tparam T Data type handled by this Value.
 */
template <typename T> class Value : public ValueBase
{
public:
    /**
     * @brief Construct a new Value object
     *
     * @param data The cello::Object that owns this Value
     * @param id Identifier of the data
     * @param initVal default initialized state for this value.
     */
    Value (Object& data, const juce::Identifier& id_, T initVal = {})
    : ValueBase { id_ }
    , object { data }
    {
        // if the object doesn't have this value yet, add it and set it
        // to the initial value. This will happen as part of initializing a
        // new Object, but may also happen if new values are added to an existing
        // type.
        if (!object.hasattr (id))
            object.setattr<T> (id, initVal);
    }

    /**
     * @brief Assign a new value, setting it in the underlying tree and
     * perhaps notifying listeners.
     *
     * @param val
     * @return Value& reference to this value so you could e.g. do
     * `this.x = this.y = 42;`
     */
    Value& operator= (const T& val)
    {
        set (val);
        return *this;
    }

    /**
     * @brief Set property value in the tree. If the `onSet` validator function
     * has been configured, the `val` argument will be passed through that function
     * (and possibly modified) before being stored into the tree.
     *
     * @param val
     */
    void set (const T& val)
    {
        if (onSet != nullptr)
            doSet (onSet (val));
        else
            doSet (val);
    }

    /**
     * @brief Get the current value of this property from the tree.
     *
     * @return T
     */
    operator T () const { return get (); }

    /**
     * @brief Get the current value of this property from the tree.
     *
     * @return T
     */
    T get () const
    {
        if (onGet != nullptr)
            return onGet (doGet ());
        return doGet ();
    }

    /**
     * @class Cached
     * @brief A utility class to maintain the last known value of a cello::Value
     * object -- each call that fetches from a Value does two things that may be
     * more costly than we like in some cases:
     * - fetch the current value from the underlying value tree
     * - execute the `onGet` validation function if one is defined for this value.
     *
     * Objects of this type will store the last value of the associated Value object
     * each time it's changed, and can be used directly without additional overhead.
     *
     * NOTE that we store a reference to a Value object owned by another cello::Object;
     * be careful that the lifetime of this cached value object is not longer than
     * that owning object.
     */
    class Cached
    {
    public:
        Cached (Value<T>& val)
        : value { val }
        , cachedValue { static_cast<T> (value) }
        {
            // when the underlying value changes, cache it here so it can
            // be used without needing to look it up, go through validation, etc.
            value.onPropertyChange ([this] (const juce::Identifier& /*id*/) { cachedValue = static_cast<T> (value); });
        }

        ~Cached () { value.onPropertyChange (nullptr); }

        operator T () const { return cachedValue; }

    private:
        Value<T>& value;
        T cachedValue;
    };

    /**
     * @return an initialized `Cached` object that will always contain the current state
     * of this `Value`.
     */
    Cached getCached () { return Cached (*this); }

    /**
     * @brief We define the signature of a 'validator' function that
     * can validate/modify/replace values as your application requires.
     *
     * These will be called (if present) whenever this value is set or
     * retrieved.
     */
    using ValidatePropertyFn = std::function<T (const T&)>;

    /**
     * @brief validator function called before setting this Value.
     */
    ValidatePropertyFn onSet;

    /**
     * @brief validator function called when retrieving this Value.
     * This function is called with the current stored value, and might
     * return a different value.
     */
    ValidatePropertyFn onGet;

    /**
     * @brief A listener to exclude from property change updates.
     *
     * @param listener
     */
    void excludeListener (juce::ValueTree::Listener* listener) { excludedListener = listener; }

    /**
     * @brief Register (or clear) a callback function to execute when this value
     * changes.
     *
     * @param callback
     */
    void onPropertyChange (PropertyUpdateFn callback) { object.onPropertyChange (getId (), callback); }

private:
    void doSet (const T& val)
    {
        juce::ValueTree tree { object };

        // check if this call should change the current value.
        if (notEqualTo (val))
        {
            // check if this value or our parent object have a listener to exclude
            // from updates.
            auto* excluded = (excludedListener != nullptr) ? excludedListener : object.getExcludedListener ();
            const auto asVar { juce::VariantConverter<T>::toVar (val) };
            if (excluded)
                tree.setPropertyExcludingListener (excluded, id, asVar, object.getUndoManager ());
            else
                tree.setProperty (id, asVar, object.getUndoManager ());
        }
        else
        {
            // check if we or our parent object want us to always send
            // a property change callback for this value.
            if (shouldForceUpdate () || object.shouldForceUpdate ())
                tree.sendPropertyChangeMessage (id);
        }
    }

    T doGet () const
    {
        juce::ValueTree tree { object };
        return juce::VariantConverter<T>::fromVar (tree.getProperty (id));
    }

    /**
     * @brief Compare some value to our current value; for floating point types, we
     * check against an epsilon value (that is static for all cello::Value objects)
     *
     * @param newValue
     * @return true if the two values are sufficiently unequal.
     */
    bool notEqualTo (const T& newValue)
    {
        if constexpr (std::is_floating_point_v<T>)
            return std::fabs (newValue - doGet ()) > epsilon;
        else
            return (newValue != doGet ());
    }

public:
    /// when setting a floating point value, delta must be larger than this to
    /// cause a property change callback.
    static inline float epsilon { 0.001f };

private:
    /// cello::Object containing the tree for this property.
    Object& object;

    /// pointer to a listener to exclude from property change callbacks.
    juce::ValueTree::Listener* excludedListener { nullptr };
};

template <typename T, // the actual type
          typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
Value<T>& operator+= (Value<T>& val, const T& rhs)
{
    const auto current { static_cast<T> (val) };
    val = current + rhs;
    return val;
}

template <typename T, // the actual type
          typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
Value<T>& operator-= (Value<T>& val, const T& rhs)
{
    const auto current { static_cast<T> (val) };
    val = current - rhs;
    return val;
}

template <typename T, // the actual type
          typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
Value<T>& operator*= (Value<T>& val, const T& rhs)
{
    const auto current { static_cast<T> (val) };
    val = current * rhs;
    return val;
}

template <typename T, // the actual type
          typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
Value<T>& operator/= (Value<T>& val, const T& rhs)
{
    jassert (rhs != 0);
    const auto current { static_cast<T> (val) };
    val = current / rhs;
    return val;
}

/**
 * @brief Pre-increment
 *
 * @tparam T
 * @tparam std::enable_if<std::is_arithmetic<T>::value, T>::type
 * @param val
 * @return Value<T>&
 */
template <typename T, // the actual type
          typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
T operator++ (Value<T>& val)
{
    const auto newVal { static_cast<T> (val) + static_cast<int> (1) };
    val.set (newVal);
    return newVal;
}

/**
 * @brief post-increment; note that the semantics of this don't follow 'real'
 * C++ usage -- because this type relies on an underlying ValueTree object to
 * provide the actual data storage, the idea of 'returning a copy of this object
 * in its original state' doesn't work. Instead, we return an instance of the
 * `T` data type itself.
 *
 * @tparam T
 * @tparam std::enable_if<std::is_arithmetic<T>::value, T>::type
 * @param val
 * @return Value<T>&
 */
template <typename T, // the actual type
          typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
T operator++ (Value<T>& val, int)
{
    const auto original { static_cast<T> (val) };
    val.set (original + static_cast<T> (1));
    return original;
}

/**
 * @brief Pre-decrement
 *
 * @tparam T
 * @tparam std::enable_if<std::is_arithmetic<T>::value, T>::type
 * @param val
 * @return T
 */
template <typename T, // the actual type
          typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
T operator-- (Value<T>& val)
{
    const auto newVal { static_cast<T> (val) - static_cast<T> (1) };
    val.set (newVal);
    return newVal;
}

/**
 * @brief post-decrement; note that the semantics of this don't follow 'real'
 * C++ usage -- because this type relies on an underlying ValueTree object to
 * provide the actual data storage, the idea of 'returning a copy of this object
 * in its original state' doesn't work. Instead, we return an instance of the
 * `T` data type itself.
 *
 * @tparam T
 * @tparam std::enable_if<std::is_arithmetic<T>::value, T>::type
 * @param val
 * @return T
 */
template <typename T, // the actual type
          typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
T operator-- (Value<T>& val, int)
{
    const auto original { static_cast<T> (val) };
    val.set (original - static_cast<T> (1));
    return original;
}

} // namespace cello

/**
 * @brief a useful macro to create and default initialize a cello::Value
 * as a member of a cello::Object, using the same name for the variable
 * as the identifier used for the property in its ValueTree.
 */
// clang-format off
#define MAKE_VALUE_MEMBER(type, name, init)                  \
    static const inline juce::Identifier name##Id { #name }; \
    cello::Value<type> name { *this, name##Id, init }
// clang-format on