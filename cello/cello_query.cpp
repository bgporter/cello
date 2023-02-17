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

Query& Query::addFilter (Predicate filter)
{
    return *this;
}

juce::ValueTree Query::search (juce::ValueTree tree, bool deep) const
{
    // error for now; return an empty tree.
    juce::ValueTree result { type };
    if (filters.size () == 0)
    {
        for (auto child : tree)
        {
            auto childCopy { juce::ValueTree { child.getType () } };
            if (deep)
                childCopy.copyPropertiesAndChildrenFrom (child, nullptr);
            else
                childCopy.copyPropertiesFrom (child, nullptr);
            result.appendChild (childCopy, nullptr);
        }
    }
    else
    {
        // todo: execute the query predicates!
    }
    // todo: sort
    return result;
}
} // namespace cello

#if RUN_UNIT_TESTS
#include "test/test_cello_query.inl"
#endif
