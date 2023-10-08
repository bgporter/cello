

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
                right.appendChild (juce::ValueTree { "rightleft" }, nullptr);
                right.appendChild (juce::ValueTree { "rightright" }, nullptr);
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

        test ("just find",
              [this] ()
              {
                  Path leftPath { "left" };
                  auto leftTree1 { leftPath.findValueTree (rootTree,
                                                           Path::SearchType::query) };
                  // if we start with the left tree and just ask for "left", we should
                  // always return that tree.
                  expect (leftTree1.isValid ());
                  auto leftTree2 { leftPath.findValueTree (leftTree1,
                                                           Path::SearchType::query) };
                  expect (leftTree2.isValid ());

                  // if we start with a single segment path that's a child, search for
                  // that.
                  Path leftleftPath { "leftleft" };
                  auto leftTree2a { leftleftPath.findValueTree (
                      leftTree1, Path::SearchType::query) };
                  expect (leftTree2a.isValid ());
                  expect (leftTree2a.hasType ("leftleft"));

                  // ...but a path of "./left" will look for a *child* named 'left'
                  Path leftChildPath { "./left" };
                  auto leftTree3 { leftChildPath.findValueTree (
                      leftTree1, Path::SearchType::query) };
                  expect (!leftTree3.isValid ());
                  leftTree3 = { leftChildPath.findValueTree (
                      leftTree1, Path::SearchType::createTarget) };
                  expect (leftTree3.isValid ());
              });

        test ("create target",
              [this] ()
              {
                  Path p1 { "/left/leftright" };
                  isExpected (p1.findValueTree (rootTree, Path::SearchType::query),
                              "leftright");
                  isExpected (p1.findValueTree (rootTree, Path::SearchType::createTarget),
                              "leftright");

                  Path p2 { "/left/leftright/leftrightleft" };
                  auto t2 { p2.findValueTree (rootTree, Path::SearchType::query) };
                  expect (!t2.isValid ());

                  isExpected (p2.findValueTree (rootTree, Path::SearchType::createTarget),
                              "leftrightleft");

                  Path p3 { "rootChild" };
                  isExpected (p3.findValueTree (rootTree, Path::SearchType::createTarget),
                              "rootChild");
              });
    }

private:
    // !!! test class member vars here...

    juce::ValueTree rootTree;
};

static Test_cello_path testcello_path;
} // namespace cello
