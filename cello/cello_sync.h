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
    virtual ~UpdateQueue () {}
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

    /**
     * @brief Check if the given thread is the destination thread for this update queue.
     * 
     * @param thread pointer to the thread to check
     * @return true if the thread is the destination thread, false otherwise
     */
    bool isDestinationThread (juce::Thread* thread) const { return thread == destThread; }

protected:
    void pushUpdate (juce::MemoryBlock&& update);

    /**
     * @brief Called when a new update is pushed onto the queue. We use this 
     * to prevent feedback loops.
     * 
     * @param data pointer to the update data
     * @param size size of the update data
     */
    virtual void startUpdate (void* data, size_t size) = 0;
    /**
     * @brief Called when the update is complete. clear the update data. 
     */
    virtual void endUpdate () = 0;

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

class SyncController;

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
/**
 * @struct SyncData
 * @brief Data structure for holding synchronization update information
 */
struct SyncData
{
    const char* data { nullptr };
    size_t size { 0 };

    SyncData () = default;
    SyncData (const void* data_, size_t size_)
    : data (static_cast<const char*> (data_))
    , size (size_)
    {
    }
    bool operator== (const SyncData& other) const
    {
        if (size != other.size)
            return false;
        for (size_t i { 0 }; i < size; ++i)
            if (data[i] != other.data[i])
                return false;
        return true;
    }
    bool operator!= (const SyncData& other) const { return !(*this == other); }
};

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
     * @param controller pointer to the (optional) SyncController that will be 
     *                   used when we are performing a bidirectional sync.
     */
    Sync (Object& producer, Object& consumer, juce::Thread* thread, SyncController* controller = nullptr);

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

    void startUpdate (void* data, size_t size) override;
    void endUpdate () override;

    SyncController* controller { nullptr };
};

/**
 * @brief Class to manage bi-directional sync between two Objects in different
 * threads, preventing feedback loops. Each SyncController contains a pair
 * of Sync objects, one for each direction of the sync.
 */
class SyncController
{
public:
    /**
     * @brief Construct a new SyncController object.
     * 
     * @param obj1 pointer to the first Object
     * @param threadForObj1 pointer to the thread for the first Object
     * @param obj2 pointer to the second Object
     * @param threadForObj2 pointer to the thread for the second Object
     */
    SyncController (Object& obj1, juce::Thread* threadForObj1, Object& obj2, juce::Thread* threadForObj2);
    ~SyncController () = default;

    SyncController (const SyncController&)            = delete;
    SyncController& operator= (const SyncController&) = delete;
    SyncController (SyncController&&)                 = delete;
    SyncController& operator= (SyncController&&)      = delete;

    /**
     * @brief Perform the next update for the given thread.
     *
     * @param thread pointer to the thread to perform the update
     */
    void performNextUpdate (juce::Thread* thread);

    /**
     * @brief Perform all updates for the given thread.
     */
    void performAllUpdates (juce::Thread* thread);

private:
    Sync sync1to2;
    Sync sync2to1; 

    SyncData update1to2;
    SyncData update2to1;

    friend class Sync;
    /**
     * @brief Called by Sync object to let us know that it's about to apply an update.
     * 
     * @param sync pointer to the Sync object that is about to apply the update
     * @param data pointer to the update data
     * @param size size of the update data
     */
    void startUpdate (Sync* sync, void* data, size_t size);
    /**
     * @brief Called by Sync object to let us know that it's finished applying the update.
     * 
     * @param sync pointer to the Sync object that has finished applying the update
     */
    void endUpdate (Sync* sync);

    /**
     * @brief Check if the update is valid. If this is the data the other side 
     * just sent us, we don't want to apply it again.
     * 
     * @param sync pointer to the Sync object that is about to apply the update
     * @param data pointer to the update data
     * @param size size of the update data
     */
    bool shouldHandleUpdate (Sync* sync, void* data, size_t size) const;

};

} // namespace cello