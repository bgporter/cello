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

UpdateQueue::UpdateQueue (Object& consumer, juce::Thread* thread)
: dest (consumer)
, destThread (thread)
{
}

int UpdateQueue::getPendingUpdateCount () const
{
    const juce::ScopedLock lock { mutex };
    return static_cast<int> (queue.size ());
}

void UpdateQueue::performAllUpdates ()
{
    while (getPendingUpdateCount () > 0)
        performNextUpdate ();
}

void UpdateQueue::performNextUpdate ()
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

    startUpdate (block.getData (), block.getSize ());
    dest.update (block);
    endUpdate ();
}

void UpdateQueue::pushUpdate (juce::MemoryBlock&& update)
{
    // push the update data onto the queue
    {
        const juce::ScopedLock lock { mutex };
        queue.push_back (update);
    }
    if (destThread == nullptr)
        juce::MessageManager::callAsync (
            [this] ()
            {
                jassert (juce::MessageManager::existsAndIsCurrentThread ());
                performAllUpdates ();
            });
    else
        // wake the consumer thread up if it's waiting. It's the duty
        // of that thread to call either `performNextUpdate()` (iterating through
        // pending updates on its own) or `performAllUpdates()` to apply any
        // pending changes waiting in the queue.
        destThread->notify ();
}

//
//////////////////////////////////////////////////////////////////////////
//

Sync::Sync (Object& producer, Object& consumer, juce::Thread* thread, SyncController* controller)
: UpdateQueue (consumer, thread)
, juce::ValueTreeSynchroniser { producer }
, controller (controller)
{
    // cannot sync to yourself!
    jassert (static_cast<juce::ValueTree> (producer) != static_cast<juce::ValueTree> (consumer));
}

void Sync::stateChanged (const void* encodedChange, size_t encodedChangeSize)
{
    if (controller != nullptr)
    {
        if (!controller->shouldHandleUpdate (this, (char*)encodedChange, encodedChangeSize))
            return;
    }

    pushUpdate (juce::MemoryBlock (encodedChange, encodedChangeSize));
}

void Sync::startUpdate (void* data, size_t size)
{
    if (controller != nullptr)
        controller->startUpdate (this, data, size);
}


void Sync::endUpdate ()
{
    if (controller != nullptr)
        controller->endUpdate (this);
}

//
//////////////////////////////////////////////////////////////////////////
//

SyncController::SyncController (Object& obj1, juce::Thread* thread1, Object& obj2, juce::Thread* thread2)
: sync1to2 (obj1, obj2, thread2, this)
, sync2to1 (obj2, obj1, thread1, this)
{
    jassert (thread2 != thread1);
}

void SyncController::startUpdate (Sync* sync, void* data, size_t size)
{
    if (sync == &sync1to2)
        update1to2 = SyncData ((char*)data, size);
    else if (sync == &sync2to1)
        update2to1 = SyncData ((char*)data, size);
    else
        jassertfalse;
}

void SyncController::endUpdate (Sync* sync)
{
    if (sync == &sync1to2)
        update1to2.data = {};
    else if (sync == &sync2to1)
        update2to1.data = {};
    else
        jassertfalse;
}

bool SyncController::shouldHandleUpdate (Sync* sync, void* data, size_t size) const
{
    if (sync == &sync1to2)
        return update2to1 != SyncData { data, size };
    else if (sync == &sync2to1)
        return update1to2 != SyncData { data, size };
    else
        jassertfalse;
    return false;
}

void SyncController::performNextUpdate (juce::Thread* thread)
{
    // This method should perform the next update for the specified thread
    if (sync1to2.isDestinationThread (thread))
        sync1to2.performNextUpdate ();
    else if (sync2to1.isDestinationThread (thread))
        sync2to1.performNextUpdate ();
    else
        jassertfalse;
}

void SyncController::performAllUpdates (juce::Thread* thread)
{   
    // This method should perform all updates for the specified thread
    if (sync1to2.isDestinationThread (thread))
        sync1to2.performAllUpdates ();
    else if (sync2to1.isDestinationThread (thread))
        sync2to1.performAllUpdates ();
    else
        jassertfalse;
}
} // namespace cello

#if RUN_UNIT_TESTS
#include "test/test_cello_sync.inl"
#endif
