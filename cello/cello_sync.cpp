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

#include "cello_sync.h"
#include "cello_object.h"

namespace cello
{
Sync::Sync (Object& producer, Object& consumer, juce::Thread* thread)
: juce::ValueTreeSynchroniser { producer }
, src { producer }
, dest { consumer }
, destThread { thread }
{
    // cannot sync to yourself!
    jassert (static_cast<juce::ValueTree> (producer) !=
             static_cast<juce::ValueTree> (consumer));
}

void Sync::stateChanged (const void* encodedChange, size_t encodedChangeSize)
{
    // push the update data onto the queue
    {
        const juce::ScopedLock lock { mutex };
        queue.push_back (juce::MemoryBlock { encodedChange, encodedChangeSize });
    }
    if (destThread == nullptr)
        juce::MessageManager::callAsync (
            [this] ()
            {
                jassert (juce::MessageManager::existsAndIsCurrentThread ());
                performAllUpdates ();
            });
    else
        // wake the consumer thread up if it's sleeping. It's the duty
        // of that thread to call either `performNextUpdate()` (iterating through
        // pending updates on its own) or `performAllUpdates()` to apply any
        // pending changes waiting in the queue.
        destThread->notify ();
}

int Sync::getPendingUpdateCount () const
{
    const juce::ScopedLock lock { mutex };
    return static_cast<int> (queue.size ());
}

void Sync::performAllUpdates ()
{
    while (getPendingUpdateCount () > 0)
        performNextUpdate ();
}

void Sync::performNextUpdate ()
{
    if (getPendingUpdateCount () == 0)
        return;

    // lock the queue and get the block at its head
    juce::MemoryBlock block;
    {
        const juce::ScopedLock lock { mutex };
        block = std::move (queue.front ());
        queue.pop_front ();
    }
    juce::ValueTree destTree { dest };
    juce::ValueTreeSynchroniser::applyChange (destTree, block.getData (),
                                              block.getSize (), dest.getUndoManager ());
}

} // namespace cello

#if RUN_UNIT_TESTS
#include "test/test_cello_sync.inl"
#endif
