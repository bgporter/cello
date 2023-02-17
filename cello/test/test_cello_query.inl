

#include <juce_core/juce_core.h>

class Test_cello_query : public TestSuite
{
public:
    Test_cello_query ()
    : TestSuite ("cello_query", "database")
    {
    }

    void runTest () override
    {
        beginTest ("!!! WRITE SOME TESTS FOR THE cello_query Class !!!");

        // replace the tree with an empty one.
        tearDown ([this] () { parentTree = {}; });

        // create a tree with 100 children , each containing a random float value
        // between 0..1
        setup (
            [this] ()
            {
                cello::Object root { "root", nullptr };
                auto& r = juce::Random::getSystemRandom ();
                for (int i { 0 }; i < 100; ++i)
                {
                    cello::Object d { "data", nullptr };
                    d.setattr ("val", r.nextFloat ());
                    root.append (&d);
                }
                parentTree = root;
            });

        test ("select all",
              [this] ()
              {
                  cello::Object root { "root", parentTree };
                  expectEquals (parentTree.getNumChildren (), 100);

                  cello::Query query { "result" };
                  // a query with no predicates means select all...
                  auto result { root.find (query, false) };
                  expect (result.hasType ("result"));
                  expectEquals (result.getNumChildren (), 100);
              });

        /*
          To create a test, call `test("testName", testLambda);`
          To (temporarily) skip a test, call `skipTest("testName", testLambda);`
          To define setup for a block of tests, call `setup(setupLambda);`
          To define cleanup for a block of tests, call `tearDown(tearDownLambda);`

          Setup and TearDown lambdas will be called before/after each test that
          is executed, and remain in effect until explicitly replaced.

          All the functionality of the JUCE `UnitTest` class is available from
          within these tests.
        */
    }

private:
    // !!! test class member vars here...
    juce::ValueTree parentTree;
};

static Test_cello_query testcello_query;
