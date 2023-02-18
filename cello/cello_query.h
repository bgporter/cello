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

namespace cello
{

class Query
{
public:
    /// The default identifier for the query results tree.
    static inline const juce::Identifier Result { "result" };

    // query function, returns true if the tree it is pass should
    // be included in the result set.
    using Predicate = std::function<bool (juce::ValueTree)>;

    /**
     * @brief Construct a new Query object.
     *
     * @param resultType type id of the ValueTree that we should return.
     */
    Query (const juce::Identifier& resultType = Result);

    /**
     * @brief Construct a new Query object that has a single filter predicate
     * ready to run.
     *
     * @param filter Predicate function to run. You can add additional predicates
     * (that will be logically ANDed) with the `addFilter` method.
     * @param resultType
     */
    Query (Predicate filter, const juce::Identifier& resultType = Result);

    ~Query ()                       = default;
    Query (const Query&)            = default;
    Query& operator= (const Query&) = default;
    Query (Query&&)                 = default;
    Query& operator= (Query&&)      = default;

    /**
     * @brief Append a filter predicate to the end of our list; these are
     * executed in the sequence they're added, and we stop testing at the first
     * filter that returns false.
     *
     * @param filter
     * @return Query& reference to this so we can use the builder pattern.
     */
    Query& addFilter (Predicate filter);

    /**
     * @brief Execute the query we're programmed for -- iterate through the children
     * of `tree`, returning a new tree of type `resultType` that contains a copy
     * (either shallow or deep) of each child that fulfills the query, sorted according
     * to the sort criteria we've been given.
     *
     * @param tree ValueTree to search.
     * @param deep  If true, the result tree will contain a deep copy of each
     *      chid found.
     * @return juce::ValueTree with query results.
     */
    juce::ValueTree search (juce::ValueTree tree, bool deep) const;

private:
    /**
     * @brief Execute the filter predicates against this child tree, and return
     * false as soon as we know that we should filter it out.
     *
     * @param tree
     * @return true to include this item in the search results.
     */
    bool filter (juce::ValueTree tree) const;

private:
    /// @brief The type of the return data ValueTree
     juce::Identifier type;
    /// @brief List of predicates to execute as a query.
    std::vector<Predicate> filters;
};

} // namespace cello
