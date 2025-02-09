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

/**
 * @class Path
 * @brief Class to navigate between subtrees that are all connected together.
 *
 * This is designed to operate similarly to directory paths, using a slash-separated
 * string to declare a path between a ValueTree and some other ancestor, sibling,
 * or descendant tree that can be reached from it.
 *
 * Syntax works as follows:
 *
 * - `"child"` without additional indications, a segment refers to a child.
 * - `".."` the parent of the current tree
 * - `"/"` separates segments. Begin a path string with this to indicate paths
 *      beginning at the root tree.
 * - `"^{treeType}"` navigate to the first parent tree of this type.
 * - `""` (empty path string) refers to the current tree.
 *
 * These work together as expected, so that `"../siblingName"` would specify a sibling
 * of the current tree, `"child/grandChild`" specifies a descendant tree two levels below
 * the current one.
 */
class Path
{
public:
    /// @brief  Path separator
    static const inline juce::String sep { "/" };
    static const inline juce::String ancestor { "^" };
    static const inline juce::String parent { ".." };
    static const inline juce::String current { "." };

    Path (const juce::String& pathString)
    : pathSegments { parsePathSegments (pathString) }
    {
    }

    enum class SearchType
    {
        query,        ///< only search, do not create anything.
        createTarget, ///< create final tree in specification, but no intermediate trees
        createAll     ///< create final tree and all intermediate trees needed to reach it.
    };

    enum SearchResult
    {
        notFound, ///< unable to find the requested tree
        found,    ///< the sought tree existed already and was found
        created   ///< performing a search created a new tree
    };

    /**
     * @brief Navigate the path from `origin` to a tree that is expected at the end
     * of the current path specification.
     *
     * @param origin Initial/"current" value tree from which to begin search
     * @param searchType
     * @return juce::ValueTree; if search type was `query` may be an invalid tree
     */
    juce::ValueTree findValueTree (juce::ValueTree& origin, SearchType searchType, juce::UndoManager* undo = nullptr);

    /**
     * @brief Find out whether performing a search succeeded, and if so, needed to
     * create a new tree.
     *
     * @return SearchResult
     */
    SearchResult getSearchResult () const { return searchResult; }

private:
    /**
     * @brief parse the path string into its segments, cleaning and verifying as needed.
     *
     * @param pathString
     * @return juce::StringArray
     */
    juce::StringArray parsePathSegments (const juce::String& pathString);

    const juce::StringArray pathSegments;

    SearchResult searchResult { SearchResult::notFound };
};

} // namespace cello
