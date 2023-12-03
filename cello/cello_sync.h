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

#include <deque>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

namespace cello
{

class Object;

class UpdateQueue
{
public:
    UpdateQueue (Object& consumer, juce::Thread* thread);
    virtual ~UpdateQueue () {};
    UpdateQueue (const UpdateQueue&)            = delete;
    UpdateQueue& operator= (const UpdateQueue&) = delete;
    UpdateQueue (UpdateQueue&&)                 = delete;
    UpdateQueue& operator= (UpdateQueue&&)      = delete;

    /**
     * @return int = number of updates that are ready to apply to the consumer side.
     */
    int getPendingUpdateCount () const;

    /**
     * @brief Execute each of the updates that are ready.
     */
    void performAllUpdates ();

    /**
     * @brief Pop the next event from the queue and apply the change to the
     * destination value tree.
     */
    void performNextUpdate ();

protected:
    void pushUpdate (juce::MemoryBlock&& update);

private:
    /// @brief  Cello object that is being updated
    Object& dest;
    /// @brief juce Thread object responsible for performing destination updates
    juce::Thread* destThread;
    /// @brief Critical section to maintain thread sanity
    juce::CriticalSection mutex;
    /// @brief Queue of tree updates to communicate between threads
    std::deque<juce::MemoryBlock> queue;
};

/**
 * @class Sync
 * @brief Permits thread-safe Object updates by using the
 * juce::ValueTreeSynchroniser class to generate small binary patches that
 * are used to pass updates from one copy of a ValueTree to another, each in
 * separate threads. This sync is only performed in one direction, so you will
 * need a pair of these objects to perform bidirectional syncs.
 *
 * Take care to not generate infinite update loops.
 */
class Sync : public UpdateQueue,
             public juce::ValueTreeSynchroniser
{
public:
    /**
     * @brief Construct a new Sync object
     *
     * @param producer cello::Object that will be sending updates
     * @param consumer cello::Object that will be kept in sync with the producer
     * @param thread non-owning pointer to the Thread on which the consumer will
     *              be updated. If the consumer object is to be updated on the
     *              message thread, pass a nullptr for this arg.
     */
    Sync (Object& producer, Object& consumer, juce::Thread* thread);

    Sync (const Sync&)            = delete;
    Sync& operator= (const Sync&) = delete;

private:
    /**
     * @brief Whenever the state of the producer tree changes, this callback will
     * be executed to push the delta onto the queue that connects the producer
     * and consumer threads and then alerts the consumer side that there's new
     * data ready for processing. If the consumer thread is the message thread,
     * we schedule an async update; otherwise we call `notify()` to awaken the
     * other thread if needed.
     * @param encodedChange     pointer to a block of binary data
     * @param encodedChangeSize  length of the data.
     */
    void stateChanged (const void* encodedChange, size_t encodedChangeSize) override;
};

} // namespace cello