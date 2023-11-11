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

#include "JuceHeader.h"

#include "cello_object.h"

namespace cello
{

Object::Object (const juce::String& type, const Object* state)
: Object { type, (state != nullptr ? static_cast<juce::ValueTree> (*state)
                                   : juce::ValueTree ()) }
{
    if (state != nullptr)
        undoManager = state->getUndoManager ();
}

Object::Object (const juce::String& type, const Object& state)
: Object (type, &state)
{
}

#define PATH_IMPL 1
Object::Object (const juce::String& type, juce::ValueTree tree)
{
    wrap (type, static_cast<juce::ValueTree> (tree));
}

Object::Object (const juce::String& type, juce::File file, Object::FileFormat format)
: Object { type, Object::load (file, format) }
{
}

Object::Object (const Object& rhs)
: data { rhs.data }
, undoManager { rhs.undoManager }
{
    // register to receive callbacks when the tree changes.
    data.addListener (this);
}

Object::CreationType Object::wrap (const Object& other)
{
    data.removeListener (this);
    const auto result { wrap (getType ().toString (), other) };
    undoManager = other.getUndoManager ();
    return result;
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

juce::ValueTree Object::clone (bool deep) const
{
    auto cloneTree { juce::ValueTree { getType () } };
    if (deep)
        cloneTree.copyPropertiesAndChildrenFrom (data, nullptr);
    else
        cloneTree.copyPropertiesFrom (data, nullptr);
    return cloneTree;
}

juce::ValueTree Object::find (const cello::Query& query, bool deep)
{
    return query.search (data, deep);
}

bool Object::upsert (const Object* object, const juce::Identifier& key, bool deep)
{
    if (!object->hasattr (key))
        return false;

    const auto val { object->data[key] };

    auto existingItem { data.getChildWithProperty (key, val) };
    if (existingItem.isValid ())
    {
        // we found the match -- update in place.
        if (deep)
            existingItem.copyPropertiesAndChildrenFrom (*object, getUndoManager ());
        else
            existingItem.copyPropertiesFrom (*object, getUndoManager ());
        return true;
    }
    // else, we need to add a copy to the end of our children.
    data.appendChild (object->clone (deep), getUndoManager ());
    return true;
}

void Object::upsertAll (const Object* parent, const juce::Identifier& key, bool deep)
{
    juce::ValueTree parentTree { *parent };
    for (const auto& child : parentTree)
    {
        const auto type { child.getType () };
        Object item { type.toString (), child };

        if (!upsert (&item, key, deep))
            jassertfalse;
    }
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
        undoMgr->clearUndoHistory ();
}

int Object::getNumChildren () const
{
    return data.getNumChildren ();
}

juce::ValueTree Object::operator[] (int index) const
{
    if (index < 0 || index >= data.getNumChildren ())
        return {};

    return data.getChild (index);
}

void Object::append (Object* object)
{
    insert (object, -1);
}

void Object::insert (Object* object, int index)
{
    if (object == this)
    {
        // can't add an object to itself!
        jassertfalse;
        return;
    }
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
    // make sure that the new child is using this object's undo manager.
    object->setUndoManager (getUndoManager ());
}

Object* Object::remove (Object* object)
{
    if (object == this)
    {
        jassertfalse;
        return nullptr;
    }
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

void Object::move (int fromIndex, int toIndex)
{
    data.moveChild (fromIndex, toIndex, getUndoManager ());
}

template <typename Comparator> void Object::sort (Comparator& comp, bool stableSort)
{
    data.sort (comp, getUndoManager (), stableSort);
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

void Object::onPropertyChange (const ValueBase& val, PropertyUpdateFn callback)
{
    onPropertyChange (val.getId (), callback);
}

bool Object::hasattr (const juce::Identifier& attr) const
{
    return data.hasProperty (attr);
}

void Object::delattr (const juce::Identifier& attr)
{
    data.removeProperty (attr, getUndoManager ());
}

juce::ValueTree Object::load (juce::File file, FileFormat format)
{
    if (format == Object::FileFormat::xml)
    {
        const auto xmlText { file.loadFileAsString () };
        return juce::ValueTree::fromXml (xmlText);
    }

    // one of the binary formats
    juce::MemoryBlock mb;
    if (!file.loadFileAsData (mb))
    {
        jassertfalse;
        return {};
    }
    if (format == Object::FileFormat::binary)
        return juce::ValueTree::readFromData (mb.getData (), mb.getSize ());
    else if (format == Object::FileFormat::zipped)
        return juce::ValueTree::readFromGZIPData (mb.getData (), mb.getSize ());

    // unknown format
    jassertfalse;
    return {};
}

juce::Result Object::save (juce::File file, FileFormat format) const
{
    if (format == FileFormat::xml)
    {
        auto res { file.create () };
        if (res.wasOk ())
        {
            if (file.replaceWithText (data.toXmlString ()))
                return juce::Result::ok ();
            res = juce::Result::fail ("Error writing to " + file.getFullPathName ());
        }
        return res;
    }

    juce::FileOutputStream fos { file };
    if (!fos.openedOk ())
    {
        jassertfalse;
        return juce::Result::fail ("Unable to open " + file.getFullPathName () +
                                   " for writing");
    }

    if (format == FileFormat::binary)
    {
        data.writeToStream (fos);
        return juce::Result::ok ();
    }

    else if (format == FileFormat::zipped)
    {
        juce::GZIPCompressorOutputStream zipper { fos };
        data.writeToStream (zipper);
        return juce::Result::ok ();
    }

    // unknown format
    jassertfalse;
    return juce::Result::fail ("Unknown file format");
}

Object::CreationType Object::wrap (const juce::String& type, juce::ValueTree tree)
{
    creationType = CreationType::wrapped;
#if PATH_IMPL
    Path path { type };
    // DBG(tree.toXmlString());
    data = path.findValueTree (tree, Path::SearchType::createAll, nullptr);
    // DBG(data.toXmlString());
    if (path.getSearchResult () == Path::SearchResult::created)
        creationType = CreationType::initialized;
#else
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
                // as having been default-initialized.
                data         = juce::ValueTree (type);
                creationType = CreationType::initialized;
                tree.appendChild (data, getUndoManager ());
            }
        }
    }
    else
    {
        // case 4: There's no state, just create an empty tree of the correct
        // type and mark ourselves as needing to be initialized.
        data         = juce::ValueTree (type);
        creationType = CreationType::initialized;
    }
#endif

    // register to receive callbacks when the tree changes.
    data.addListener (this);
    return creationType;
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

void Object::valueTreeChildAdded (juce::ValueTree& parentTree, juce::ValueTree& childTree)
{
    if (parentTree == data && onChildAdded != nullptr)
        onChildAdded (childTree, -1, data.indexOf (childTree));
}

void Object::valueTreeChildRemoved (juce::ValueTree& parentTree,
                                    juce::ValueTree& childTree, int index)
{
    if (parentTree == data && onChildRemoved != nullptr)
        onChildRemoved (childTree, index, -1);
}

void Object::valueTreeChildOrderChanged (juce::ValueTree& parentTree, int oldIndex,
                                         int newIndex)
{
    if (parentTree == data && onChildMoved != nullptr)
    {
        auto childTree { data.getChild (newIndex) };
        onChildMoved (childTree, oldIndex, newIndex);
    }
}

void Object::valueTreeParentChanged (juce::ValueTree& tree)
{
    if (tree == data && onParentChanged != nullptr)
        onParentChanged ();
}

void Object::valueTreeRedirected (juce::ValueTree& tree)
{
    if (tree == data && onTreeRedirected != nullptr)
        onTreeRedirected ();
}

} // namespace cello

#if RUN_UNIT_TESTS
#include "test/test_cello_object.inl"
#endif
