

#include <juce_core/juce_core.h>
namespace cello
{
class Test_cello_path : public TestSuite
{
public:
    Test_cello_path ()
    : TestSuite ("cello_path", "paths")
    {
    }

    void isExpected (juce::ValueTree tree, const juce::Identifier& type)
    {
        expect (tree.isValid ());
        expect (tree.hasType (type));
    }
    void runTest () override
    {
        setup (
            [this] ()
            {
                rootTree = juce::ValueTree { "root" };

                juce::ValueTree left { "left" };
                left.appendChild (juce::ValueTree { "leftleft" }, nullptr);
                left.appendChild (juce::ValueTree { "leftright" }, nullptr);
                rootTree.appendChild (left, nullptr);

                juce::ValueTree right { "right" };
                left.appendChild (juce::ValueTree { "rightleft" }, nullptr);
                left.appendChild (juce::ValueTree { "rightright" }, nullptr);
                rootTree.appendChild (right, nullptr);
            });

        tearDown ([this] () { rootTree = juce::ValueTree {}; });

        test ("get root",
              [this] ()
              {
                  auto root { findRoot (rootTree) };
                  isExpected (rootTree, "root");

                  // descend by hand
                  auto left { rootTree.getChildWithName ("left") };
                  expect (left.isValid ());
                  auto leftRight { left.getChildWithName ("leftright") };
                  expect (leftRight.isValid ());

                  auto root1 { findRoot (left) };
                  isExpected (root1, "root");

                  auto root2 { findRoot (leftRight) };
                  isExpected (root2, "root");

                  // use a path object to do the same thing.
                  Path path { "/" };
                  isExpected (path.findValueTree (rootTree, Path::SearchType::query),
                              "root");
                  isExpected (Path { ".." }.findValueTree (left, Path::SearchType::query),
                              "root");
                  isExpected (
                      Path { "../.." }.findValueTree (leftRight, Path::SearchType::query),
                      "root");
                  isExpected (
                      Path { "^root" }.findValueTree (leftRight, Path::SearchType::query),
                      "root");
              });
    }

private:
    // !!! test class member vars here...

    juce::ValueTree rootTree;
};

static Test_cello_path testcello_path;
} // namespace cello
