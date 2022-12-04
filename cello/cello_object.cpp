//
// Copyright (c) 2022 Brett g Porter. All Rights Reserved.
//

#include "JuceHeader.h"

#include "cello_object.h"

namespace cello
{

Object::Object (juce::Identifier type, Object* state)
{
    if (state != nullptr)
    {
        // case 1: We're passed the tree we use as our store directly.
        if (state->getType () == type)
        {
            data = *state;
        }
        else
        {
            juce::ValueTree tree { *state };

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
        undoManager = state->getUndoManager ();
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
