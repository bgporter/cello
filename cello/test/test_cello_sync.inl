

#include <juce_core/juce_core.h>

namespace
{

class ThreadTestObject : public cello::Object
{
public:
    ThreadTestObject ()
    : cello::Object ("tto", nullptr)
    {
    }

    MAKE_VALUE_MEMBER (int, x, {});
    MAKE_VALUE_MEMBER (int, y, {});
};

class WorkerThread : public juce::Thread
{
public:
    WorkerThread (const juce::String& name)
    : juce::Thread (name)
    {
    }

    void setSync (cello::Sync* syncObject)
    {
        jassert (syncObject != nullptr);
        sync = syncObject;
    }

    void setSyncController (cello::SyncController* syncControllerObject)
    {
        jassert (syncControllerObject != nullptr);
        syncController = syncControllerObject;
    }

    void run () override
    {
        jassert (sync != nullptr || syncController != nullptr);
        while (!threadShouldExit ())
        {
            if (syncController != nullptr)
                syncController->performAllUpdates (this);
            else if (sync != nullptr)
                sync->performAllUpdates ();
            else
                jassertfalse;
            wait (1000);
        }
    }

    cello::Sync* sync { nullptr };
    cello::SyncController* syncController { nullptr };
    ThreadTestObject tto;
};

class GeneratorThread : public juce::Thread
{
public:
    GeneratorThread (int max = 100)
    : juce::Thread ("generator")
    , maxVal { max }
    {
    }

    void run ()
    {
        for (int i { 0 }; i < maxVal; ++i)
        {
            tto.x = i + 1;
            juce::Thread::sleep (250);
        }
    }

    int maxVal;
    ThreadTestObject tto;
};

} // namespace

class Test_cello_sync : public TestSuite
{
public:
    Test_cello_sync ()
    : TestSuite ("cello_sync", "thread sync")
    {
    }

    void runTest () override
    {
        test ("one-way to worker",
              [this] ()
              {
                  ThreadTestObject src;
                  WorkerThread thread ("oneway");
                  cello::Sync sync (src, thread.tto, &thread);

                  const int updateCount { 100 };
                  thread.tto.x.onPropertyChange (
                      [&thread] (const juce::Identifier&)
                      {
                          if ((int) thread.tto.x >= updateCount)
                              thread.signalThreadShouldExit ();
                      });
                  thread.setSync (&sync);
                  thread.startThread ();
                  for (int i { 0 }; i < updateCount + 1; ++i)
                  {
                      src.x = i;
                  }
                  while (thread.isThreadRunning ())
                  {
                      // loop here until thread finishes running...
                      juce::Thread::sleep (10);
                  }
                  expect ((int) thread.tto.x == updateCount);
              });

        // Looking for suggestions of how to test async updates into the
        // message thread while keeping the tests located here and not
        // intruding into the main application. The approach here (obviously)
        // doesn't work because we loop and sleep and the message queue never
        // gets serviced while we're doing this.
        skipTest ("one way from worker",
                  [this] ()
                  {
                      // if the test runner isn't on the message thread, skip this
                      // test.
                      if (!juce::MessageManager::existsAndIsCurrentThread ())
                          return;
                      const int updateCount { 100 };
                      ThreadTestObject dest;
                      GeneratorThread thread (updateCount);
                      cello::Sync sync (thread.tto, dest, nullptr);

                      thread.startThread ();
                      do
                      {
                          DBG ("dest.x = " << (int) dest.x);
                          juce::Thread::sleep (100);
                      } while (thread.isThreadRunning ());
                  });

        test ("two-way thread updates",
              [this] ()
              {
                  WorkerThread leftThread ("left");
                  WorkerThread rightThread ("right");   
                  cello::SyncController syncController (leftThread.tto, &leftThread, rightThread.tto, &rightThread);

                  leftThread.setSyncController (&syncController);
                  rightThread.setSyncController (&syncController);
                  

                  // each of the WorkerThread objects listens to a different value in the
                  // shared tree; when the value being watched changes, the thread updates
                  // the value of the *other* value so we get a cascade of update messages
                  // between the threads. We tell the threads to exit when the value they
                  // are watching is greater than 100.
                  leftThread.tto.y.onPropertyChange (
                      [&leftThread] (const juce::Identifier&)
                      {
                          int yVal { leftThread.tto.y };
                          leftThread.tto.x = yVal + 1;
                          if (yVal > 100)
                              leftThread.signalThreadShouldExit ();
                      });
                  rightThread.tto.x.onPropertyChange (
                      [&rightThread] (const juce::Identifier&)
                      {
                          int xVal { rightThread.tto.x };
                          rightThread.tto.y = xVal + 1;
                          if (xVal > 100)
                              rightThread.signalThreadShouldExit ();
                      });

                  leftThread.startThread ();
                  rightThread.startThread ();

                  // start the cascade
                  leftThread.tto.x = 1;
                  // spin here a bit while waiting for the two worker threads to
                  // update each other...
                  while (leftThread.isThreadRunning () || rightThread.isThreadRunning ())
                  {
                      juce::Thread::sleep (100);
                  }
                  expectEquals (leftThread.tto.x.get(), 103);
                  expectEquals (rightThread.tto.y.get(), 102);
              });

        test ("prevent feedback loops",
              [this] ()
              {
                  WorkerThread leftThread ("left");
                  WorkerThread rightThread ("right");
                  cello::SyncController syncController (leftThread.tto, &leftThread, rightThread.tto, &rightThread);
                  
                  leftThread.setSyncController (&syncController);
                  rightThread.setSyncController (&syncController);

                  leftThread.startThread ();
                  rightThread.startThread ();

                  // adding a child to one of the Objects should add a child to the other 
                  // and NOT keep echoing. 
                  auto child { cello::Object("childType", nullptr) };
                  leftThread.tto.append (&child);
                  auto child2 { cello::Object("childType", nullptr) };
                  leftThread.tto.append (&child2);
                  juce::Thread::sleep (100);
                  expectEquals (rightThread.tto.getNumChildren(), 2);

                  leftThread.signalThreadShouldExit ();
                  rightThread.signalThreadShouldExit ();

                  while (leftThread.isThreadRunning () || rightThread.isThreadRunning ())
                  {
                      juce::Thread::sleep (100);
                  }
              });
    }
};

static Test_cello_sync testcello_sync;
