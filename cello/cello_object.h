//
// Copyright (c) 2022 Brett g Porter. All Rights Reserved.
//

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
namespace cello
{
class ValueBase;

class Object : public juce::ValueTree::Listener
{
public:
    /**
     * @brief Construct a new cello::Object object, which will attempt to
     * initialize from the 'state' parameter. If 'state' contains a ValueTree of the
     * requested type, we'll use that as our store.
     *
     * Otherwise, we look for a child of our type:
     * if found, we use that as our data.
     * if not found, we create and (default) intialize a new tree of our type and add it
     * as a child to the tree pointed to by state. If state is nullptr, we create and
     * default-initialize a new tree object.
     *
     * We register as a listener to whatever value tree we just found or created.
     *
     * @param type
     * @param state pointer to a cello::Object; pass nullptr to default initialize.
     */
    Object (juce::Identifier type, Object* state);

    /**
     * @brief Construct a new Object from a raw juce ValueTree. Its behavior
     * mimics that of the ctor that accepts a pointer to object, attempting to
     * either:
     * - use the tree directly (if its type matches ours)
     * - look inside it for a tree of the correct type
     * - if that's not found (or the initial tree wasn't valid) , create a
     *   tree/Object of the correct type and add it to the tree that was passed in.
     *
     * @param type
     * @param tree
     */
    Object (juce::Identifier type, juce::ValueTree tree);

    /**
     * @brief Destroy the Object object
     * The important thing done here is to remove ourselves as a listener to the
     * value tree we're attached to.
     */
    ~Object () override;

    /**
     * @brief Construct a new Object object as a copy of an existing one.
     * We register as a listener, but this new copy does not have any callbacks
     * registered. Both objects will point at the same shared value tree.
     *
     * @param rhs Object to initialize ourselves from.
     */
    Object (const Object& rhs);

    /**
     * @brief set this object to use a different Object's value tree, which we will
     * begin listening to. Our `valueTreeRedirected` callback should be executed.
     *
     * @param rhs
     * @return Object&
     */
    Object& operator= (const Object& rhs);

    /**
     * @brief test for true equivalence: does this object point to the same
     * underlying tree as the tree on the right hand side? Note that because
     * cello::Object has `operator juce::ValueTree`, you can pass a reference
     * to Object as the rhs and it will work correctly.
     *
     * @param rhs
     * @return true if the same tree is on both sides.
     */
    bool operator== (const juce::ValueTree& rhs) const noexcept { return data == rhs; }

    bool operator!= (const juce::ValueTree& rhs) const noexcept { return data != rhs; }

    /**
     * @brief Get the type of this object.
     *
     * @return juce::Identifier
     */
    juce::Identifier getType () const { return data.getType (); }

    /**
     * @brief Get the ValueTree we're using as our data store.
     *
     * @return juce::ValueTree
     */
    operator juce::ValueTree () const { return data; }

    /**
     * @name Undo/redo functionality
     */
    ///@{

    /**
     * @brief Set the undo manager to use in this object
     * (and others created from it).
     *
     * @param undo
     */
    void setUndoManager (juce::UndoManager* undo);

    /**
     * @brief Get the current undo manager; only useful to this object's Value
     * objects and when creating other Objects to wrap our subtrees.
     *
     * @return juce::UndoManager*
     */
    juce::UndoManager* getUndoManager () const;

    /**
     * @brief Test whether this object/tree has anything that can be
     * undone.
     * @return false if there's no undo manager or nothing to undo
     */
    bool canUndo () const;

    /**
     * @brief Attempt to undo the last transaction.
     *
     * @return false if there's no undo manager, nothing to undo, or
     * the attempt to undo fails.
     */
    bool undo ();

    /**
     * @brief Test whether this object/tree has anything that can be
     * redone.
     * @return false if there's no undo manager or nothing to redo
     */
    bool canRedo () const;

    /**
     * @brief Attempt to redo the last transaction.
     *
     * @return false if there's no undo manager, nothing to redo, or
     * the attempt to redo fails.
     */
    bool redo ();

    /**
     * @brief reset the undo manager
     */
    void clearUndoHistory ();
    ///@}

    /**
     * @name Child Operations
     *
     */
    ///@{

    // An iterator to access child objects; note that it works in terms
    // of ValueTrees, not objects (since our list of children can be
    // heterogeneous)
    using Iterator = juce::ValueTree::Iterator;
    Iterator begin () { return data.begin (); }
    Iterator end () { return data.end (); }

    /**
     * @brief return a child tree of this object by its index.
     * NOTE that it does not return an object; to work with this data in
     * its cello::Object form, you'll need to use this tree to create a new
     * one, probably testing its type to make sure you're creating the correct
     * Object type from it.
     *
     * @param index
     * @return juce::ValueTree; will be invalid if the index is out of range.
     */
    juce::ValueTree operator[] (int index) const;

    /**
     * @brief Check how many children this object has.
     *
     * @return int
     */
    int getNumChildren () const;

    /**
     * @brief Add a new child object to the end of our child object list,
     *
     * @param object
     */
    void append (Object* object);

    /**
     * @brief add a new child object at a specific index in the list.
     *
     * @param object
     * @param index
     */
    void insert (Object* object, int index);

    /**
     * @brief Attempt to remove a child object from this.
     *
     * @param object Object containing sub-tree to remove
     * @return nullptr on failure (the specified object wasn't a child)
     */
    Object* remove (Object* object);

    /**
     * @brief remove a child by its index.
     *
     * @param index
     * @return Invalid tree if the index was out of bounds.
     */
    juce::ValueTree remove (int index);

    /**
     * @brief Change the position of one of this object's children
     *
     * @param fromIndex
     * @param toIndex
     */
    void move (int fromIndex, int toIndex);

    ///@}

    /**
     * @brief A listener to exclude from property change updates.
     *
     * @param listener
     */
    void excludeListener (juce::ValueTree::Listener* listener)
    {
        excludedListener = listener;
    }

    /**
     * @brief Get a pointer to the listener to exclude from property change updates.
     *
     * @return juce::ValueTree::Listener*
     */
    juce::ValueTree::Listener* getExcludedListener () const { return excludedListener; }

    /**
     * @brief If passed true, any call that sets any Value property on this
     * Object will result in a property change update callback being executed.
     * Default (false) behavior only performs this callback when the underlying
     * value is changed.
     *
     * This may also be controlled on a per-Value basis as well.
     *
     * @param shouldForceUpdates
     */
    void forceUpdates (bool shouldForceUpdates) { doForceUpdates = shouldForceUpdates; }

    /**
     * @return true if this Object should always issue property changed callbacks.
     */
    bool shouldForceUpdates () const { return doForceUpdates; }

    /**
     * @name Callbacks
     *
     */
    ///@{

    using PropertyUpdateFn = std::function<void (juce::Identifier)>;
    /**
     * @brief Install (or clear) a function to be called when one of this Object's
     * properties changes. A cello extension to this mechanism is that you can pass
     * in the type id of this tree, and you'll receive a callback on that key when any
     * of the other properties that don't have a handler have changed.
     *
     * @param id the ID of the property that has changed.
     * @param callback function to call on update.
     */
    void onPropertyChange (juce::Identifier id, PropertyUpdateFn callback);

    /**
     * @brief register a property change callback by passing in a reference
     * to a Value object instead of its id.
     *
     * @param val
     * @param callback
     */
    void onPropertyChange (const ValueBase& val, PropertyUpdateFn callback);

    using ChildUpdateFn =
        std::function<void (juce::ValueTree& child, int oldIndex, int newIndex)>;

    ChildUpdateFn onChildAdded;
    ChildUpdateFn onChildRemoved;
    ChildUpdateFn onChildMoved;

    using SelfUpdateFn = std::function<void (void)>;

    SelfUpdateFn onParentChanged;
    SelfUpdateFn onTreeRedirected;

    ///@}

    /**
     * @name Pythonesque access
     *
     * We use names borrowed from Python for this set of functions to
     * make them stand out. When using these, the Object becomes more
     * dynamically typed; the type-safety provided by working through
     * the cello::Value class is bypassed, and you can add/remove
     * attributes/properties and change their types from the object
     * at runtime as is useful for you.
     */
    ///@{

    /**
     * @brief Get a property value from this object, or default if it
     * doesn't have a property with that name.
     *
     * @tparam T
     * @param attr
     * @param defaultVal
     * @return T
     */
    template <typename T>
    T getattr (const juce::Identifier& attr, const T& defaultVal) const;

    /**
     * @brief test the object to see if it has an attribute with this id.
     *
     * @param attr
     * @return true
     * @return false
     */
    bool hasattr (const juce::Identifier& attr) const;

    /**
     * @brief Set a new value for the specified attribute/property.
     * @tparam T
     * @param attr
     * @param attrVal
     */
    template <typename T>
    Object& setattr (const juce::Identifier& attr, const T& attrVal);

    /**
     * @brief Remove the specified property from this object.
     * @param attr
     */
    void delattr (const juce::Identifier& attr);

    ///@}
private:
    /**
     * @brief Handle property changes in this tree by calling a registered
     * callback function for the property that changed (if one was registered).
     * As an extension, if no callback exists for a property, we will attempt to
     * execute a callback registered with the type-name of this tree/object,
     * so you can register a single catch-all handler if desired.
     *
     * Obviously, you can further derive from this and install some other
     * update mechanism logic as needed.
     *
     * @param treeWhosePropertyHasChanged
     * @param property
     */
    void valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
                                   const juce::Identifier& property) override;

    /**
     * @brief Will execute the callback `onChildAdded` if it exists.
     *
     * @param parentTree
     * @param childTree
     */
    void valueTreeChildAdded (juce::ValueTree& parentTree,
                              juce::ValueTree& childTree) override;

    /**
     * @brief Will execute the callback `onChildRemoved` if it exists.
     *
     * @param parentTree
     * @param childTree
     * @param index
     */
    void valueTreeChildRemoved (juce::ValueTree& parentTree, juce::ValueTree& childTree,
                                int index) override;

    /**
     * @brief will execute the callback `onChildMoved` if it exists.
     *
     * @param parentTree
     * @param oldIndex
     * @param newIndex
     */
    void valueTreeChildOrderChanged (juce::ValueTree& childTree, int oldIndex,
                                     int newIndex) override;

    /**
     * @brief Will execute the `onParentChanged` callback if it exists.
     *
     * @param tree
     */
    void valueTreeParentChanged (juce::ValueTree& tree) override;

    /**
     * @brief will execute the `onRedirected` callback if it exists.
     *
     * @param tree
     */
    void valueTreeRedirected (juce::ValueTree& tree) override;

protected:
    ///  The tree where our data lives.
    juce::ValueTree data;
    /// The undo manager to use for set() operations.
    juce::UndoManager* undoManager { nullptr };
    /// True if we are creating a new tree (as opposed to wrapping an existing one).
    bool initRequired { false };
    /// a listener to *not* update when properties change.
    juce::ValueTree::Listener* excludedListener { nullptr };
    /// should we send property change notifications even if a property doesn't
    /// change?
    bool doForceUpdates { false };

private:
    /**
     * @brief key/value mapping between a property ID and the callback
     * to execute when its value is updated. We create a vector of these
     * structs as update functions are registered, and perform a linear
     * search of them as needed.
     */
    struct PropertyUpdate
    {
        PropertyUpdate (juce::Identifier id_, PropertyUpdateFn fn_)
        : id { id_ }
        , fn { fn_ }
        {
        }

        juce::Identifier id;
        PropertyUpdateFn fn;
    };

    std::vector<PropertyUpdate> propertyUpdaters;
};

} // namespace cello
