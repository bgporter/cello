

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

                  cello::Query query;

                  // a query with no predicates means select all...
                  auto result { root.find (query, false) };
                  expect (result.hasType (cello::Query::Result));
                  expectEquals (result.getNumChildren (), 100);
              });

        test ("select none",
              [this] ()
              {
                  cello::Object root { "root", parentTree };
                  cello::Query query { [] (juce::ValueTree tree)
                                       {
                                           cello::Object o ("data", tree);
                                           return o.getattr ("val", 0.f) < 0;
                                       } };
                  //   query.addFilter (
                  //       [] (juce::ValueTree tree)
                  //       {
                  //           cello::Object o ("data", tree);
                  //           return o.getattr ("val", 0.f) < 0;
                  //       });
                  auto result { root.find (query, false) };
                  DBG (result.toXmlString ());
                  expect (result.hasType ("result"));
                  expectEquals (result.getNumChildren (), 0);

                  cello::Query q2 { "result" };
                  q2.addFilter (
                      [] (juce::ValueTree tree)
                      {
                          cello::Object o ("data", tree);
                          auto val = o.getattr ("val", 0.f);
                          return val > 1.f;
                      });

                  result = root.find (q2, false);
                  expect (result.hasType ("result"));
                  expectEquals (result.getNumChildren (), 0);
              });

        test ("select some",
              [this] ()
              {
                  cello::Object root { "root", parentTree };
                  cello::Query lo { "result" };
                  lo.addFilter (
                      [] (juce::ValueTree tree)
                      {
                          cello::Object o ("data", tree);
                          return o.getattr ("val", 0.f) < 0.5f;
                      });
                  cello::Query hi { "result" };
                  hi.addFilter (
                      [] (juce::ValueTree tree)
                      {
                          cello::Object o ("data", tree);
                          return o.getattr ("val", 0.f) >= 0.5f;
                      });

                  auto loResult = root.find (lo, false);
                  auto hiResult = root.find (hi, false);

                  expectEquals (loResult.getNumChildren () + hiResult.getNumChildren (),
                                root.getNumChildren ());
              });
    }

private:
    juce::ValueTree parentTree;
};

static Test_cello_query testcello_query;
