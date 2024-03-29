

#include <juce_core/juce_core.h>

class Test_cello_ipc : public TestSuite
{
public:
    Test_cello_ipc ()
    : TestSuite ("cello_ipc", "!!! category !!!")
    {
    }

    void runTest () override { beginTest ("!!! WRITE SOME TESTS FOR THE cello_ipc Class !!!"); }

private:
    // !!! test class member vars here...
};

static Test_cello_ipc testcello_ipc;
