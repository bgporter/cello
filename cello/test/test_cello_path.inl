

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

    void createPath (const Path& p) { expect (true); }

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

        test ("creation",
              [this] ()
              {
                  createPath (Path ("foo"));
                  createPath (juce::String { "bar" });
                  juce::String longPath { "/foo/bar/baz" };
                  createPath (longPath);
                  createPath ({ "x/y/z" });
              });

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
                  juce::String rootPath { "/" };
                  Path path { rootPath };
                  isExpected (path.findValueTree (rootTree, Path::SearchType::query),
                              "root");
                  expectEquals (path.getSearchResult (), Path::SearchResult::found);
                  isExpected (Path { ".." }.findValueTree (left, Path::SearchType::query),
                              "root");
                  isExpected (
                      Path { "../.." }.findValueTree (leftRight, Path::SearchType::query),
                      "root");
                  isExpected (
                      Path { "^root" }.findValueTree (leftRight, Path::SearchType::query),
                      "root");
                  isExpected (
                      Path { "root" }.findValueTree (rootTree, Path::SearchType::query),
                      "root");
              });

        test (
            "just find",
            [this] ()
            {
                Path leftPath { "left" };
                auto leftTree1 { leftPath.findValueTree (rootTree,
                                                         Path::SearchType::query) };
                // if we start with the left tree and just ask for "left", we should
                // always return that tree.
                expect (leftTree1.isValid ());
                expectEquals (leftPath.getSearchResult (), Path::SearchResult::found);
                auto leftTree2 { leftPath.findValueTree (leftTree1,
                                                         Path::SearchType::query) };
                expect (leftTree2.isValid ());
                expectEquals (leftPath.getSearchResult (), Path::SearchResult::found);

                // if we start with a single segment path that's a child, search for
                // that.
                Path leftleftPath { "leftleft" };
                auto leftTree2a { leftleftPath.findValueTree (leftTree1,
                                                              Path::SearchType::query) };
                expect (leftTree2a.isValid ());
                expect (leftTree2a.hasType ("leftleft"));
                expectEquals (leftleftPath.getSearchResult (), Path::SearchResult::found);

                // ...but a path of "./left" will look for a *child* named 'left'
                Path leftChildPath { "./left" };
                auto leftTree3 { leftChildPath.findValueTree (leftTree1,
                                                              Path::SearchType::query) };
                expect (!leftTree3.isValid ());
                expectEquals (leftChildPath.getSearchResult (),
                              Path::SearchResult::notFound);
                leftTree3 = { leftChildPath.findValueTree (
                    leftTree1, Path::SearchType::createTarget) };
                expect (leftTree3.isValid ());
                expectEquals (leftChildPath.getSearchResult (),
                              Path::SearchResult::created);
            });

        test ("variations searching from root",
              [this] ()
              {
                  // I expect all of these to find the same thing
                  Path p1 { "/left/leftleft" };
                  Path p2 { "left/leftleft" };
                  //   Path p3 { "root/left/leftleft" };

                  const auto t1 { p1.findValueTree (rootTree, Path::SearchType::query) };
                  isExpected (t1, "leftleft");
                  const auto t2 { p2.findValueTree (rootTree, Path::SearchType::query) };
                  expect (t1 == t2);
                  //   const auto t3 { p3.findValueTree (rootTree,
                  //   Path::SearchType::query) }; expect (t1 == t3);
              });

        test ("create target(s)",
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

                  Path p4 { "/rootChild/w/x/y/z" };
                  // creating without intermediate trees should fail
                  auto t3 { p4.findValueTree (rootTree, Path::SearchType::createTarget) };
                  expect (!t3.isValid ());
                  auto t4 { p4.findValueTree (rootTree, Path::SearchType::createAll) };
                  expect (t4.isValid ());
                  //   DBG (rootTree.toXmlString ());
              });

        test ("paths with ancestors",
              [this] ()
              {
                  Path p1 { "/left/leftleft" };
                  auto t1 { p1.findValueTree (rootTree, Path::SearchType::query) };
                  expect (t1.isValid ());

                  // go up to an ancestor named 'left' and find its child named
                  // 'leftright'
                  Path p2 { "^left/leftright" };
                  auto t2 { p2.findValueTree (t1, Path::SearchType::query) };
                  expect (t2.isValid ());

                  Path p3 { "^left/bogus" };
                  auto t3 { p3.findValueTree (t2, Path::SearchType::query) };
                  expect (!t3.isValid ());
              });
        test ("ex nihilo",
              [this] ()
              {
                  Path p1 { "foo" };
                  auto nullTree { juce::ValueTree () };
                  auto t1 { p1.findValueTree (nullTree, Path::SearchType::query) };
                  expect (!t1.isValid ());
                  expect (p1.getSearchResult () == Path::SearchResult::notFound);

                  auto t2 { p1.findValueTree (nullTree, Path::SearchType::createTarget) };
                  expect (t2.isValid ());
                  expect (p1.getSearchResult () == Path::SearchResult::created);

                  // if we create, we should get back a new (root) tree with a single
                  // element.
                  auto t3 { p1.findValueTree (nullTree, Path::SearchType::createAll) };
                  expect (t3.isValid ());

                  Path p2 { "foo/bar" };
                  auto t4 { p2.findValueTree (nullTree, Path::SearchType::createAll) };
                  expect (!t4.isValid ());
              });
    }

private:
    juce::ValueTree rootTree;
};

static Test_cello_path testcello_path;
} // namespace cello
