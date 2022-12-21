

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

    void setValueForceUpdate (bool doForce) { val.forceUpdate (doForce); }

    static const inline juce::String classId { "OneValue" };
    static const inline juce::String valId { "val" };

private:
    cello::Value<int> val { *this, valId, 0 };
};

struct Vec2 : public cello::Object
{
    Vec2 (juce::Identifier id, float x_, float y_)
    : cello::Object (id, nullptr)
    {
        x = x_;
        y = y_;
    }

    Vec2 (juce::Identifier id, cello::Object* object)
    : cello::Object (id, object)
    {
        if (initRequired)
        {
            x.init ();
            y.init ();
        }
    }

    Vec2 (const Vec2& rhs)
    : cello::Object (rhs)
    {
    }

    Vec2& operator= (const Vec2& rhs)
    {
        cello::Object::operator= (rhs);
        return *this;
    }

    MAKE_VALUE_MEMBER (float, x, 0.f);
    MAKE_VALUE_MEMBER (float, y, {});
};

class Rectangle : public cello::Object
{
public:
    Rectangle (juce::Identifier id, cello::Object* object)
    : cello::Object { id, object }
    {
        /*
        NOTE -- we don't need this here; this block only needs to be added
        when an Object has Value members.
        if (initRequired)
        {
        }
        */
    }

    Rectangle (juce::Identifier id, float x, float y, float w, float h)
    : cello::Object { id, nullptr }
    {
        origin.x = x;
        origin.y = y;
        size.x   = w;
        size.y   = h;
    }

private:
    Vec2 origin { "origin", this };
    Vec2 size { "size", this };
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

                  expect (!ov.canUndo ());
                  expect (!ov.canRedo ());
                  ov.setValue (2);
                  expect (ov.getValue () == 2);
                  expect (ov.canUndo ());
                  expect (ov.undo ());
                  expect (!ov.canUndo ());
                  expect (ov.getValue () == 1);
                  expect (ov.canRedo ());
                  expect (ov.redo ());
                  expect (ov.getValue () == 2);
                  OneValue ov2 (ov);
                  expect (ov2.getValue () == 2);
                  ov2.setValue (100);
                  expect (ov.getValue () == 100);
                  expect (ov.undo ());
                  expect (ov.getValue () == 2);
                  ov.setValue (50);
                  expect (ov.canUndo ());
                  ov.clearUndoHistory ();
                  expect (!ov.canUndo ());
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
        test ("force updates",
              [&] ()
              {
                  OneValue ov (22);
                  OneValue ov2 (ov);
                  int count { 0 };
                  ov2.onPropertyChange (OneValue::valId,
                                        [&count] (juce::Identifier id) { ++count; });
                  ov.setValue (2);
                  expect (count == 1);
                  ov.setValue (2);
                  expect (count == 1);
                  ov.forceUpdates (true);
                  ov.setValue (2);
                  expect (count == 2);
                  ov.forceUpdates (false);
                  ov.setValue (2);
                  expect (count == 2);
                  ov.setValue (3);
                  expect (count == 3);
                  ov.setValueForceUpdate (true);
                  ov.setValue (3);
                  expect (count == 4);
                  ov.setValueForceUpdate (false);
                  ov.setValue (3);
                  expect (count == 4);
                  // TODO: Add a test for only enabling a single value's updates
                  // in an object with multiple values.
              });
        skipTest ("exclude listeners",
                  [&] ()
                  {
                      OneValue ov (22);
                      int count { 0 };
                      ov.onPropertyChange (OneValue::valId,
                                           [&count] (juce::Identifier id) { ++count; });
                      OneValue ov2 (ov);
                      int count2 { 0 };
                      ov2.onPropertyChange (
                          OneValue::valId, [&count2] (juce::Identifier id) { ++count2; });

                      ov.setValue (2);
                      expect (count == 1);
                      expect (count2 == 1);
                      ov.excludeListener (&ov2);
                      ov.setValue (100);
                      expect (count == 2);
                      expect (count2 == 1);
                  });
        test ("create child objects",
              [&] ()
              {
                  cello::Object root ("root", nullptr);
                  Vec2 pt ("point", &root);
                  expectWithinAbsoluteError<float> (pt.x, 0, 0.001);
                  expectWithinAbsoluteError<float> (pt.y, 0, 0.001);
                  pt.x = 3.1;
                  pt.y = -1.9;

                  // init from the root tree
                  Vec2 pt2 ("point", &root);
                  expectWithinAbsoluteError<float> (pt2.x, 3.1f, 0.001);
                  expectWithinAbsoluteError<float> (pt2.y, -1.9f, 0.001);
              });
        test ("change notify tree",
              [&] ()
              {
                  cello::Object root ("root", nullptr);
                  Vec2 pt ("point", &root);
                  // init from the root tree
                  Vec2 pt2 ("point", &root);
                  float x {};
                  float y {};
                  pt2.onPropertyChange ("point",
                                        [&] (juce::Identifier id)
                                        {
                                            x = pt2.x;
                                            y = pt2.y;
                                        });
                  expectWithinAbsoluteError<float> (pt2.x, 0.f, 0.001);
                  expectWithinAbsoluteError<float> (pt2.y, 0.f, 0.001);

                  // replace the entire tree with a new one (by copying its values)
                  pt = Vec2 ("point", 101.1, -33.2);
                  expectWithinAbsoluteError<float> (x, 101.1f, 0.001);
                  expectWithinAbsoluteError<float> (y, -33.2, 0.001);
                  expectWithinAbsoluteError<float> (pt2.x, 101.1f, 0.001);
                  expectWithinAbsoluteError<float> (pt2.y, -33.2, 0.001);
              });

        test ("set property lambda",
              [&] ()
              {
                  cello::Object root ("root", nullptr);
                  Vec2 pt ("point", &root);
                  pt.x.onSet = [] (float v) { return v * 2; };
                  pt.x       = 10;
                  expectWithinAbsoluteError<float> (pt.x, 20.f, 0.001f);
                  pt.x = 100;
                  expectWithinAbsoluteError<float> (pt.x, 200.f, 0.001f);
                  pt.x.onSet = nullptr;
                  pt.x       = 100;
                  expectWithinAbsoluteError<float> (pt.x, 100.f, 0.001f);
              });

        test ("embedded objects",
              [&] ()
              {
                  Rectangle box ("box", 100, -100, 200, 250);
                  Vec2 origin { "origin", &box };
                  expectWithinAbsoluteError<float> (origin.x, 100.f, 0.001f);
                  expectWithinAbsoluteError<float> (origin.y, -100.f, 0.001f);
                  Vec2 size { "size", &box };
                  expectWithinAbsoluteError<float> (size.x, 200.f, 0.001f);
                  expectWithinAbsoluteError<float> (size.y, 250.f, 0.001f);

                  // check a default initialized box as a child of another object.
                  cello::Object root ("root", nullptr);
                  Rectangle rect ("rect", &root);
                  Vec2 origin2 { "origin", &rect };
                  expectWithinAbsoluteError<float> (origin2.x, 0.f, 0.001f);
                  expectWithinAbsoluteError<float> (origin2.y, 0.f, 0.001f);
                  Vec2 size2 { "size", &rect };
                  expectWithinAbsoluteError<float> (size2.x, 0.f, 0.001f);
                  expectWithinAbsoluteError<float> (size2.y, 0.f, 0.001f);
              });

        test ("pythonic access",
              [&] ()
              {
                  // let's make an object that's purely dynamic.
                  cello::Object o { "pythonic", nullptr };
                  juce::Identifier foo { "foo" };
                  expect (!o.hasattr (foo));
                  juce::String str { "a string attribute" };
                  o.setattr (foo, str);
                  expect (o.hasattr (foo));
                  juce::String a { o.getattr (foo, juce::String ()) };
                  expect (a == str);
                  o.delattr (foo);
                  expect (!o.hasattr (foo));
              });
    }

private:
    // !!! test class member vars here...
};
static Test_cello_object testcello_object;
} // namespace cello
