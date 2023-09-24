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

#include "cello_path.h"

namespace
{
juce::ValueTree findRoot (juce::ValueTree& origin)
{
    auto current { origin };
    for (;;)
    {
        auto parent { current.getParent () };
        if (!parent.isValid ())
            return current;
        current = parent;
    }
}

juce::ValueTree findAncestor (juce::ValueTree& origin,
                              const juce::Identifier ancestorType)
{
    auto current { origin };
    for (;;)
    {
        // pop up a level.
        auto parent { current.getParent () };
        // we hit the root without finding that ancestor; bail out.
        if (!parent.isValid ())
            return {};
        // found it!
        if (parent.hasType (ancestorType))
            return parent;
        // keep looking up a level.
        current = parent;
    }
}

} // namespace

namespace cello
{
juce::ValueTree Path::findValueTree (juce::ValueTree& origin, Path::SearchType searchType)
{
    auto currentTree { path.startsWith (sep) ? findRoot (origin) : origin };
    const auto segments { juce::StringArray::fromTokens (path, sep, "") };

    for (int i = 0; i < segments.size (); ++i)
    {
        DBG ("segment " << i << "= '" << segments[i] << "'");
    }

    for (int i { 0 }; i < segments.size () && currentTree.isValid (); ++i)
    {
        const auto& segment { segments[i].trim () };
        const auto isLastSegment { i == (segments.size () - 1) };
        if (segment == parent)
        {
            currentTree = currentTree.getParent ();
        }
        else if ((segment == current) || (segment.isEmpty ()))
        {
            // do nothing -- current tree remains the same
        }
        else if (segment.startsWith (ancestor))
        {
            currentTree = findAncestor (
                currentTree, segment.trimCharactersAtStart (juce::String { ancestor }));
        }
        else
        {
            // next segment is a child of the current tree
            auto childTree { currentTree.getChildWithName (segment) };
            if (searchType == SearchType::query)
            {
                currentTree = childTree;
            }
            else
            {
                // doesn't exist...yet. Create and add to the current tree?
            }
        };
    }

    return currentTree;
}

} // namespace cello
#if RUN_UNIT_TESTS
#include "test/test_cello_path.inl"
#endif
