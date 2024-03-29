

#include <juce_core/juce_core.h>

class Test_cello_ipc : public TestSuite
{
public:
    Test_cello_ipc ()
    : TestSuite ("cello_ipc", "cello")
    {
    }

    void runTest () override { beginTest ("Not sure how to unit test IPC calls..."); }

private:
    // !!! test class member vars here...
};

static Test_cello_ipc testcello_ipc;
