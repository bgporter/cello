

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
            sleep (1000);
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
                  expect (juce::MessageManager::existsAndIsCurrentThread ());
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

        skipTest ("two-way thread updates",
                  [this] ()
                  {
                      ThreadTestObject src;
                      WorkerThread thread ("oneway");
                      cello::Sync syncToWorker (src, thread.tto, &thread);
                      cello::Sync syncToMessage (thread.tto, src, nullptr);

                      const int updateCount { 100 };
                      thread.tto.x.onPropertyChange (
                          [&thread] (juce::Identifier)
                          {
                              // make a change in the worker thread that should be
                              // propagated back to the main thread
                              int newVal   = thread.tto.x;
                              thread.tto.y = newVal;

                              if (newVal >= updateCount)
                                  thread.signalThreadShouldExit ();
                          });

                      src.y.onPropertyChange ([this, &src] (juce::Identifier)
                                              { DBG ("src.y =" << (int) src.y); });

                      thread.setSync (&syncToWorker);
                      thread.startThread ();
                      for (int i { 0 }; i < updateCount + 1; ++i)
                      {
                          src.x = i;
                      }
                      do
                      {
                          // loop here until thread finishes running...
                          juce::Thread::sleep (10);
                      } while (syncToMessage.getPendingUpdateCount () > 0);

                      expect ((int) thread.tto.x == updateCount);
                      DBG ("src.y = " << (int) src.y);
                      expect ((int) src.y == updateCount);
                  });
    }

private:
    // !!! test class member vars here...
};

static Test_cello_sync testcello_sync;
