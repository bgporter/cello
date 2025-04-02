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

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

#include "cello_update_source.h"

namespace cello
{
class ValueBase;
class Query;

class Object : public UpdateSource,
               public juce::ValueTree::Listener
{
public:
    enum class FileFormat
    {
        xml,    // load/store as XML text.
        binary, // load/store in juce's binary format.
        zipped  // GZIPped juce binary.
    };

    enum class CreationType
    {
        initialized, // this object was default initialized when created.
        wrapped      // this object wrapped an existing tree.
    };

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
    Object (const juce::String& type, const Object* state);

    /**
     * @brief Construct a new Object, initializing from the `state` argument.
     * Follows the same descent logic used in the above constructor.
     *
     * @param type
     * @param state
     */
    Object (const juce::String& type, const Object& state);

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
    Object (const juce::String& type, juce::ValueTree tree);

    /**
     * @brief Construct a new Object by attempting to load it from a file on disk.
     * You can test whether this succeeded by checking the return value of
     * `getCreationType()` -- if its value is `CreationType::initialized`, the load
     * from disk failed, and this instance was default-initialized.
     *
     * @param type
     * @param file
     * @param format
     */
    Object (const juce::String& type, juce::File file, FileFormat format = FileFormat::xml);

    /**
     * @brief Construct a new Object object as a copy of an existing one.
     * We register as a listener, but this new copy does not have any callbacks
     * registered. Both objects will point at the same shared value tree.
     *
     * @param rhs Object to initialize ourselves from.
     */
    Object (const Object& rhs);

    /**
     * @brief Wrap another Object's tree after this object is created.
     *
     * @param other
     * @return CreationType, whether we were able to wrap that object or
     * created a newly initialized child of it.
     */
    CreationType wrap (const Object& other);

    /**
     * @brief set this object to use a different Object's value tree, which we will
     * begin listening to. Our `valueTreeRedirected` callback should be executed.
     *
     * @param rhs
     * @return Object&
     */
    Object& operator= (const Object& rhs);

    /**
     * @brief Destroy the Object object
     * The important thing done here is to remove ourselves as a listener to the
     * value tree we're attached to.
     */
    ~Object () override;

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
     * @brief Get the type of this object as a juce::Identifier.
     *
     * @return juce::Identifier
     */
    juce::Identifier getType () const { return data.getType (); }

    /**
     * @brief Get the type of this object as a string.
     *
     * @return juce::String
     */
    juce::String getTypeName () const { return getType ().toString (); }

    /**
     * @brief Generate a string representation of this object's tree.
     *
     * @param format specifies details of the output.
     * @return juce::String
     */
    juce::String toXmlString (const juce::XmlElement::TextFormat& format = {}) const
    {
        return data.toXmlString (format);
    }

    /**
     * @brief Determine how this object was created, which will be one of:
     * * CreationType::initialized -- All values were default-initialized
     * * CreationType::wrapped -- this object refers to a value tree that already
     *   existed
     *
     * It might be an error in your application to expect one or the other
     * and not find it at runtime.
     *
     * @return CreationType
     */
    CreationType getCreationType () const { return creationType; }

    /**
     * @brief utility method to test the creation type as a bool.
     *
     * @return true if this object was created by wrapping an existing tree.
     */
    bool wasWrapped () const { return creationType == CreationType::wrapped; }

    /**
     * @brief utility method to test the creation type as a bool.
     *
     * @return true if this object was created by default initialization.
     */
    bool wasInitialized () const { return creationType == CreationType::initialized; }

    /**
     * @brief Get the ValueTree we're using as our data store.
     *
     * @return juce::ValueTree
     */
    operator juce::ValueTree () const { return data; }

    /**
     * @brief Make and return a copy of our underlying value tree.
     *
     * @param deep Include children?
     * @return a copy of this thing.
     */
    juce::ValueTree clone (bool deep) const;

    /**
     * @brief Apply delta/update generated by the juce::ValueTreeSynchroniser
     * class; this is used in the sync and ipc implementations.
     *
     * @param updateBlock Binary data to apply to this object.
     */
    void update (const juce::MemoryBlock& updateBlock);

    /**
     * @name Database functionality
     */
    ///@{
    /**
     * @brief Perform a query against the children of this Object, returning
     * a new ValueTree containing zero or more copies of child trees that
     * match the query, possibly sorted into a different order than they
     * exist in this tree.
     *
     * @param query Query object that defines the search/sort criteria
     * @param deep if true, also copy sub-items from object.
     * @return juce::ValueTree
     */
    juce::ValueTree find (const cello::Query& query, bool deep = false);

    /**
     * @brief Perform a query against the children of this object, returning
     * a copy of the first child found that meets the predicates in the
     * query object, or an empty tree if none is found.
     *
     * @param query Query object that defines the search/sort criteria
     * @param deep if true, also copy sub-items from object.
     * @return juce::ValueTree copy of a matching child tree or {}
     */
    juce::ValueTree findOne (const cello::Query& query, bool deep = false);

    /**
     * @brief Update or insert a child object (concept borrowed from MongoDB)
     * Looks for a child with a 'key' value that matches the one found in the
     * object we've been passed. If a match is found, we update the entry in place
     * (update). If no match is found, we append a copy of `object` to our children.
     *
     * @param object Object with data to update or add
     * @param key property name to use to match the two entries
     * @param deep if true, also copy sub-items from object.
     * @return false if the object being added doesn't have the key property.
     */
    bool upsert (const Object* object, const juce::Identifier& key, bool deep = false);

    /**
     * @brief Perform an upsert using each of the children of the parent being passed.
     * Common workflow here:
     * 1. perform a query to get a list of copies of some children.
     * 2. modify those copies
     * 3. Update them in place in their original parent container.
     *
     * @param parent object with children to use as update sources
     * @param key   key to match children together
     * @param deep  copy subtrees as well?
     */
    void upsertAll (const Object* parent, const juce::Identifier& key, bool deep = false);
    ///@}

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

    /**
     * @brief Sort this object's children using the provided comparison object.
     *
     * The `comp` object must contain a method that uses the signature:
     * `int compareElements (const ValueTree& first, const ValueTree& second)`
     * and returns
     * * a value of < 0 if the first comes before the second
     * * a value of 0 if the two objects are equivalent
     * * a value of > 0 if the second comes before the first
     *
     * @param comp
     * @param stableSort true to keep equivalent items in the same order after
     *                   sorting.
     */
    template <typename Comparator> void sort (Comparator& comp, bool stableSort);

    ///@}

    /**
     * @brief A listener to exclude from property change updates.
     *
     * @param listener
     */
    void excludeListener (juce::ValueTree::Listener* listener) { excludedListener = listener; }

    /**
     * @brief Get a pointer to the listener to exclude from property change updates.
     *
     * @return juce::ValueTree::Listener*
     */
    juce::ValueTree::Listener* getExcludedListener () const { return excludedListener; }

    /**
     * @name Callbacks
     */
    ///@{

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
     * @brief install or clear a generic callback that will be called when any
     * property in the object changes. The identifier of the property that changed
     * will be passed to the callback.
     *
     * @param callback
     */
    void onPropertyChange (PropertyUpdateFn callback) { onPropertyChange (getType (), callback); }

    /**
     * @brief register a property change callback by passing in a reference
     * to a Value object instead of its id.
     *
     * @param val
     * @param callback
     */
    void onPropertyChange (const ValueBase& val, PropertyUpdateFn callback);

    using ChildUpdateFn = std::function<void (juce::ValueTree& child, int oldIndex, int newIndex)>;

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
     * We use names (ending in `-attr`) borrowed from Python for this
     * set of functions to make them stand out. When using these, the
     * Object becomes more dynamically typed; the type-safety provided
     * by working through * the cello::Value class is bypassed, and you
     * can add/remove attributes/properties and change their types from
     * the object at runtime as is useful for you.
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
    template <typename T> T getattr (const juce::Identifier& attr, const T& defaultVal) const
    {
        return juce::VariantConverter<T>::fromVar (data.getProperty (attr, defaultVal));
    }

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
     * We return a reference to this object so that setattr calls
     * may be chained.
     * @tparam T
     * @param attr
     * @param attrVal
     */
    template <typename T> Object& setattr (const juce::Identifier& attr, const T& attrVal)
    {
        data.setProperty (attr, juce::VariantConverter<T>::toVar (attrVal), getUndoManager ());
        return (*this);
    }

    /**
     * @brief Remove the specified property from this object.
     * @param attr
     */
    void delattr (const juce::Identifier& attr);

    ///@}

    /**
     * @name File operations
     * @brief save/load objects to/from disk.
     */
    ///@{
    /**
     * @brief Reload data from disk. Used in the ctor that accepts file name and
     * format.
     *
     * @param file
     * @param format one of (xml, binary, zipped)
     * @return ValueTree, invalid if the attempt to load failed.
     */
    static juce::ValueTree load (juce::File file, FileFormat format = FileFormat::xml);

    /**
     * @brief Save the object tree to disk.
     *
     * @param file
     * @param format one of (xml, binary, zipped)
     * @return Result of the save operation.
     */
    juce::Result save (juce::File file, FileFormat format = FileFormat::xml) const;

    ///@}
private:
    /**
     * @brief connect this object to the provided tree or one of its children,
     * creating a newly-initialized object if we don't find a tree of the
     * required type.
     *
     * @param type
     * @param tree
     * @return CreationType
     */
    CreationType wrap (const juce::String& type, juce::ValueTree tree);

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
    void valueTreeChildAdded (juce::ValueTree& parentTree, juce::ValueTree& childTree) override;

    /**
     * @brief Will execute the callback `onChildRemoved` if it exists.
     *
     * @param parentTree
     * @param childTree
     * @param index
     */
    void valueTreeChildRemoved (juce::ValueTree& parentTree, juce::ValueTree& childTree, int index) override;

    /**
     * @brief will execute the callback `onChildMoved` if it exists.
     *
     * @param parentTree
     * @param oldIndex
     * @param newIndex
     */
    void valueTreeChildOrderChanged (juce::ValueTree& childTree, int oldIndex, int newIndex) override;

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

    /// Remember how this Object was created.
    CreationType creationType { CreationType::wrapped };

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
