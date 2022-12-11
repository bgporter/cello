//
// Copyright (c) 2022 Brett g Porter. All Rights Reserved.
//

#pragma once

namespace cello
{

class Object;
/**
 * @brief A class to abstract away the issues around storing and retrieving
 * a value from a ValueTree. Designed to make working with VT values more
 * like working with regular class/struct members.
 *
 * @tparam T
 */
template <typename T> class Value
{
public:
    /**
     * @brief Construct a new Value object
     *
     * @param data_ The cello::Object that owns this Value
     * @param id_ Identifier of the data
     * @param init_ default initialized state for this value.
     */
    Value (Object& data_, juce::Identifier id_, T init_ = {})
    : object { data_ }
    , id { id_ }
    , initVal { init_ }
    {
    }
    /**
     * @brief Assign a new value, setting it in the underlying tree and
     * perhaps notifying listeners.
     *
     * @param val
     * @return Value& reference to this value so you could e.g. do
     * ```
     * this.x = this.y = 42;
     */
    Value& operator= (const T& val)
    {
        set (val);
        return *this;
    }
    /**
     * @brief Set property value in the tree.
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
    operator T () const
    {
        juce::ValueTree tree { object };
        return tree.getProperty (id);
    }

    juce::Identifier getId () const { return id; }

    void forceUpdate (bool forceUpdate_) { doForceUpdate = forceUpdate_; }
    /**
     * @brief reset to our initialized state.
     */
    void init () { set (initVal); }

    using SetPropertyFn = std::function<T (const T&)>;
    SetPropertyFn onSet;

private:
    void doSet (const T& val)
    {
        juce::ValueTree tree { object };

        // check if this call should change the current value.
        // TODO: Replace this with a call that handles floating point values
        // responsibly.
        if (val != *this)
        {
            // check if this value or our parent object have a listener to exclude
            // from updates.
            auto* excluded = (excludedListener != nullptr)
                                 ? excludedListener
                                 : object.getExcludedListener ();
            if (excluded)
                tree.setPropertyExcludingListener (excluded, id, val,
                                                   object.getUndoManager ());
            else
                tree.setProperty (id, val, object.getUndoManager ());
        }
        else
        {
            // check if we or our parent object want us to always send
            // a property change callback for this value.
            const auto forceUpdate = doForceUpdate || object.shouldForceUpdates ();
            if (forceUpdate)
                tree.sendPropertyChangeMessage (id);
        }
    }

private:
    /// cello::Object containing the tree for this property.
    Object& object;
    /// identifier of this value/property.
    const juce::Identifier id;
    /// initial value.
    T initVal;
    /// always send updates on set() even if value doesn't change?
    bool doForceUpdate { false };
    /// pointer to a listener to exclude from property change callbacks.
    juce::ValueTree::Listener* excludedListener { nullptr };
};

} // namespace cello

/**
 * @brief a useful macro to create and default initialize a cello::Value
 * as a member of a cello::Object, using the same name for the variable
 * as the identifier used for the property in its ValueTree.
 *
 */
#define MAKE_VALUE_MEMBER(type, name, init) \
    cello::Value<type> name { *this, #name, init };
