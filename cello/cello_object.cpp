//
// Copyright (c) 2022 Brett g Porter. All Rights Reserved.
//

#include "JuceHeader.h"

#include "cello_object.h"

namespace cello
{

Object::Object (juce::Identifier type, Object* state)
: Object { type, (state != nullptr ? static_cast<juce::ValueTree> (*state)
                                   : juce::ValueTree ()) }
{
    if (state != nullptr)
        undoManager = state->getUndoManager ();
}

Object::Object (juce::Identifier type, juce::ValueTree tree)
{
    if (tree.isValid ())
    {
        // case 1: We're passed the tree we use as our store directly.
        if (tree.getType () == type)
            data = tree;
        else
        {
            // case 2: look in the state tree for our data.
            auto childTree = tree.getChildWithName (type);
            if (childTree.isValid ())
            {
                data = childTree;
            }
            else
            {
                // case 3: the state tree doesn't have a tree for our data type
                // yet. Create an empty tree of the correct type, mark ourselves
                // as needing to be initialized, and add the empty tree to the state
                // tree. Derived classes will need to track this `initRequired` flag in
                // their ctors and respond appropriately.
                data         = juce::ValueTree (type);
                initRequired = true;
                tree.appendChild (data, getUndoManager ());
            }
        }
    }
    else
    {
        // case 4: There's no state, just create an empty tree of the correct
        // type and mark ourselves as needing to be initialized.
        data         = juce::ValueTree (type);
        initRequired = true;
    }
    // register to receive callbacks when the tree changes.
    data.addListener (this);
}

Object::Object (const Object& rhs)
: data { rhs.data }

, undoManager { rhs.undoManager }
, initRequired { false }
{
    // register to receive callbacks when the tree changes.
    data.addListener (this);
}

Object& Object::operator= (const Object& rhs)
{
    // can't change this object's type by doing this.
    jassert (getType () == rhs.getType ());
    data.copyPropertiesAndChildrenFrom (rhs.data, getUndoManager ());
    return *this;
}

Object::~Object ()
{
    data.removeListener (this);
}

void Object::setUndoManager (juce::UndoManager* undo)
{
    undoManager = undo;
}

bool Object::canUndo () const
{
    if (auto* undoMgr = getUndoManager ())
        return undoMgr->canUndo ();
    return false;
}

bool Object::undo ()
{
    if (auto* undoMgr = getUndoManager ())
        return undoMgr->undo ();
    return false;
}

bool Object::canRedo () const
{
    if (auto* undoMgr = getUndoManager ())
        return undoMgr->canRedo ();
    return false;
}

bool Object::redo ()
{
    if (auto* undoMgr = getUndoManager ())
        return undoMgr->redo ();
    return false;
}

void Object::clearUndoHistory ()
{
    if (auto* undoMgr = getUndoManager ())
        return undoMgr->clearUndoHistory ();
}

void Object::append (Object* object)
{
    insert (object, -1);
}

void Object::insert (Object* object, int index)
{
    // a value tree can only have 1 parent -- if the new object has a parent,
    // remove it there first.
    juce::ValueTree newChild { *object };
    juce::ValueTree parent { newChild.getParent () };

    if (parent.isValid ())
    {
        // we can get into a weird state if we try to mix operations on
        // different undo managers.
        jassert (getUndoManager () == object->getUndoManager ());
        parent.removeChild (newChild, getUndoManager ());
    }
    data.addChild (*object, index, getUndoManager ());
}

Object* Object::remove (Object* object)
{
    auto removedTree { remove (data.indexOf (*object)) };

    return removedTree.isValid () ? object : nullptr;
}

juce::ValueTree Object::remove (int index)
{
    auto treeToRemove { data.getChild (index) };
    if (treeToRemove.isValid ())
        data.removeChild (treeToRemove, getUndoManager ());
    return treeToRemove;
}

juce::UndoManager* Object::getUndoManager () const
{
    return undoManager;
}

void Object::onPropertyChange (juce::Identifier id, PropertyUpdateFn callback)
{
    // replace an existing callback?
    for (auto& updater : propertyUpdaters)
    {
        if (updater.id == id)
        {
            updater.fn = callback;
            return;
        }
    }
    // nope, append to the list.
    propertyUpdaters.emplace_back (id, callback);
}

template <typename T>
T Object::getattr (const juce::Identifier& attr, const T& defaultVal) const
{
    return static_cast<T> (data.getProperty (attr, defaultVal));
}

bool Object::hasattr (const juce::Identifier& attr) const
{
    return data.hasProperty (attr);
}

template <typename T>
Object& Object::setattr (const juce::Identifier& attr, const T& attrVal)
{
    data.setProperty (attr, attrVal, getUndoManager ());
    return (*this);
}

void Object::delattr (const juce::Identifier& attr)
{
    data.removeProperty (attr, getUndoManager ());
}

void Object::valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
                                       const juce::Identifier& property)
{
    if (treeWhosePropertyHasChanged == data)
    {
        // first, try to find a callback for that exact property.
        for (const auto& updater : propertyUpdaters)
        {
            if (updater.id == property)
            {
                if (updater.fn != nullptr)
                    updater.fn (property);
                return;
            }
        }
        // a cello extension: register a callback on the name of the tree's
        // type, and you'll get a callback there for any property change that
        // didn't have its own callback registered.
        if (property != getType ())
            valueTreePropertyChanged (data, getType ());
    }
}

} // namespace cello

#if RUN_UNIT_TESTS
#include "test/test_cello_object.inl"
#endif
