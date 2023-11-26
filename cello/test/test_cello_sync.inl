

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

    void run () override
    {
        jassert (sync != nullptr);
        while (!threadShouldExit ())
        {
            sync->performAllUpdates ();
            wait (1000);
        }
    }

    cello::Sync* sync;
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
            Thread::sleep (250);
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
                      [&thread] (juce::Identifier)
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

        test ("one way from worker",
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
                      juce::Thread::wait (100);
                  } while (thread.isThreadRunning ());
              });

        test ("two-way thread updates",
              [this] ()
              {
                  WorkerThread leftThread ("left");
                  WorkerThread rightThread ("right");
                  // we need a pair of cello::Sync objects; one in each direction.
                  cello::Sync syncLeftToRight (leftThread.tto, rightThread.tto,
                                               &rightThread);
                  cello::Sync syncRightToLeft (rightThread.tto, leftThread.tto,
                                               &leftThread);

                  leftThread.setSync (&syncRightToLeft);
                  rightThread.setSync (&syncLeftToRight);

                  // each of the WorkerThread objects listens to a different value in the
                  // shared tree; when the value being watched changes, the thread updates
                  // the value of the *other* value so we get a cascade of update messages
                  // between the threads. We tell the threads to exit when the value they
                  // are watching is greater than 100.
                  leftThread.tto.y.onPropertyChange (
                      [&leftThread] (juce::Identifier)
                      {
                          int yVal { leftThread.tto.y };
                          //   DBG ("leftThread x = " << yVal + 1);
                          leftThread.tto.x = yVal + 1;
                          if (yVal > 100)
                              leftThread.signalThreadShouldExit ();
                      });
                  rightThread.tto.x.onPropertyChange (
                      [&rightThread] (juce::Identifier)
                      {
                          int xVal { rightThread.tto.x };
                          //   DBG ("rightThread y = " << xVal + 1);
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
                  while (leftThread.isThreadRunning () && rightThread.isThreadRunning ())
                  {
                      juce::Thread::sleep (100);
                  }
                  expectEquals ((int) leftThread.tto.x, 103);
                  expectEquals ((int) rightThread.tto.y, 102);
              });
    }
};

static Test_cello_sync testcello_sync;
