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

#include "cello_query.h"

namespace cello
{
Query::Query (const juce::Identifier& resultType)
: type { resultType }
{
}

Query::Query (Predicate filter, const juce::Identifier& resultType)
: Query { resultType }
{
    addFilter (filter);
}

Query& Query::addFilter (Predicate filter)
{
    filters.push_back (filter);
    return *this;
}

juce::ValueTree Query::search (juce::ValueTree tree, bool deep, bool returnFirstFound) const
{
    juce::ValueTree result { type };
    for (auto child : tree)
    {
        if (filter (child))
        {
            auto childCopy { juce::ValueTree { child.getType () } };
            if (deep)
                childCopy.copyPropertiesAndChildrenFrom (child, nullptr);
            else
                childCopy.copyPropertiesFrom (child, nullptr);
            if (returnFirstFound)
                return childCopy;
            result.appendChild (childCopy, nullptr);
        }
    }

    if (returnFirstFound)
        return {};

    return sort (result);
    // return result;
}

int Query::remove (juce::ValueTree tree) const
{
    int removed { 0 };
    for (int i = tree.getNumChildren () - 1; i >= 0; i--)
    {
        if (filter (tree.getChild (i)))
        {
            tree.removeChild (i, nullptr);
            removed++;
        }
    }
    return removed;
}


bool Query::filter (juce::ValueTree tree) const
{
    if (filters.size () > 0)
    {
        for (auto fn : filters)
        {
            if (!fn (tree))
                return false;
        }
    }
    return true;
}

Query& Query::addComparison (Comparison sorter)
{
    sorters.push_back (sorter);
    return *this;
}

juce::ValueTree Query::sort (juce::ValueTree tree, juce::UndoManager* undo, bool stableSort) const
{
    if (sorters.size () > 0)
        tree.sort (*this, undo, stableSort);
    return tree;
}

int Query::compareElements (const juce::ValueTree& left, const juce::ValueTree& right) const
{
    for (auto sorter : sorters)
    {
        auto sortOrder { sorter (left, right) };
        if (sortOrder != 0)
            return sortOrder;
    }
    return 0;
}
} // namespace cello

#if RUN_UNIT_TESTS
#include "test/test_cello_query.inl"
#endif
