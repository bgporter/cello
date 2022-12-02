

#include <juce_core/juce_core.h>

namespace
{

class OneValue : public cello::Object
{
public:
    OneValue (int value)
    : cello::Object (classId, nullptr)
    {
        val = value;
    }

    OneValue (const OneValue& rhs)
    : cello::Object (rhs)
    {
    }

    OneValue& operator= (const OneValue& rhs)
    {
        cello::Object::operator= (rhs);
        return *this;
    }

    void setValue (int newValue) { val = newValue; }

    int getValue () const { return val; }

    static const inline juce::String classId { "OneValue" };
    static const inline juce::String valId { "val" };

private:
    cello::Value<int> val { *this, valId, 0 };
};

} // namespace
namespace cello
{
class Test_cello_object : public TestSuite
{
public:
    Test_cello_object ()
    : TestSuite ("cello_object", "cello")
    {
    }

    void runTest () override
    {
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

        test ("basic initialization",
              [&] ()
              {
                  // create an empty tree.
                  auto o1 { Object ("root", nullptr) };
                  juce::ValueTree root { o1 };
                  expect (root.isValid ());
                  expect (root.hasType ("root"));
                  // create a child
                  auto o2 { Object ("child1", &o1) };
                  juce::ValueTree childTree { o2 };
                  expect (childTree.isValid ());
                  expect (childTree.hasType ("child1"));

                  // get that child in a separate object.
                  auto o2Copy { Object ("child1", &o1) };
                  juce::ValueTree childCopy { o2Copy };
                  expect (childTree == childCopy);
              });

        test ("basic get/set",
              [&] ()
              {
                  OneValue ov (100);
                  expect (ov.getValue () == 100);
                  ov.setValue (-400);
                  expect (ov.getValue () == -400);
              });

        test ("copy ctor",
              [&] ()
              {
                  OneValue ov (22);
                  OneValue ov2 (ov);

                  // verify 'copy' has the right value
                  expect (ov.getValue () == 22);
                  // verify data visible on both sides.
                  ov.setValue (77);
                  expect (ov.getValue () == 77);
                  expect (ov2.getValue () == 77);
                  ov2.setValue (99);
                  expect (ov.getValue () == 99);
                  expect (ov2.getValue () == 99);
              });

        test ("op=",
              [&] ()
              {
                  OneValue ov (1);
                  OneValue ov2 (2);
                  expect (ov.getValue () == 1);
                  expect (ov2.getValue () == 2);
                  ov.setValue (100);
                  expect (ov2.getValue () == 2);
                  ov2 = ov;
                  ov.setValue (100);
                  expect (ov2.getValue () == 100);
              });

        test ("undo/redo",
              [&] ()
              {
                  OneValue ov (1);
                  juce::UndoManager undo;
                  ov.setUndoManager (&undo);

                  ov.setValue (2);
                  expect (ov.getValue () == 2);
                  undo.undo ();
                  expect (ov.getValue () == 1);
                  undo.redo ();
                  expect (ov.getValue () == 2);
                  OneValue ov2 (ov);
                  expect (ov2.getValue () == 2);
                  ov2.setValue (100);
                  expect (ov.getValue () == 100);
                  undo.undo ();
                  expect (ov.getValue () == 2);
              });

        test ("property change callbacks",
              [&] ()
              {
                  OneValue ov (0);
                  int count { 0 };
                  ov.onPropertyChange (OneValue::valId,
                                       [&count] (juce::Identifier id) { ++count; });
                  ov.setValue (2);
                  expect (count == 1);
                  // we shouldn't get a callback if the value doeesn't change.
                  ov.setValue (2);
                  expect (count == 1);
                  ov.setValue (3);
                  expect (count == 2);
                  // remove the update fn
                  ov.onPropertyChange (OneValue::valId, nullptr);
                  ov.setValue (4);
                  expect (count == 2);
              });
        skipTest ("force updates", [&] () { expect (false); });
        skipTest ("exclude listeners", [&] () { expect (false); });
    }

private:
    // !!! test class member vars here...
};
static Test_cello_object testcello_object;
} // namespace cello
