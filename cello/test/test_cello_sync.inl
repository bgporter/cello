

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

class DestThread : public juce::Thread
{
public:
    DestThread (const juce::String& name)
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
        test ("one-way thread updates",
              [this] ()
              {
                  ThreadTestObject src;
                  DestThread thread ("oneway");
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
    }

private:
    // !!! test class member vars here...
};

static Test_cello_sync testcello_sync;
