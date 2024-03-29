/*
    Copyright (c) 2023 Brett g Porter
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include <juce_core/juce_core.h>

/// @cond TEST
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

    OneValue (juce::ValueTree tree)
    : cello::Object (classId, tree)
    {
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
    Vec2 (const juce::String& id, float x_, float y_)
    : cello::Object (id, nullptr)
    {
        x = x_;
        y = y_;
    }

    Vec2 (const juce::String& id, cello::Object* object)
    : cello::Object (id, object)
    {
    }

    Vec2 (const juce::String& id, juce::ValueTree tree)
    : cello::Object (id, tree)
    {
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

class Rect : public cello::Object
{
public:
    Rect (const juce::String& id, cello::Object* object)
    : cello::Object { id, object }
    {
    }

    Rect (const juce::String& id, float x, float y, float w, float h)
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
                  expect (o1.getCreationType () == Object::CreationType::initialized);
                  juce::ValueTree root { o1 };
                  expect (root.isValid ());
                  expect (root.hasType ("root"));
                  // create a child
                  auto o2 { Object ("child1", o1) };
                  juce::ValueTree childTree { o2 };
                  expect (childTree.isValid ());
                  expect (childTree.hasType ("child1"));

                  // get that child in a separate object.
                  auto o2Copy { Object ("child1", o1) };
                  expect (o2Copy.getCreationType () == Object::CreationType::wrapped);
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

        test ("op=/!=",
              [&] ()
              {
                  OneValue ov1_1 (100);
                  OneValue ov1_2 (ov1_1);

                  OneValue ov2_1 (200);
                  OneValue ov2_2 (ov2_1);

                  expect (ov1_1 == ov1_2);
                  expect (ov1_2 == ov1_1);
                  expect (ov1_1 != ov2_1);
                  expect (ov1_2 != ov2_2);
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
                  cello::PropertyUpdateFn callback = [&count] (juce::Identifier) { ++count; };
                  ov.onPropertyChange (OneValue::valId, callback);
                  ov.setValue (2);
                  expect (count == 1);
                  // we shouldn't get a callback if the value
                  // doeesn't change.
                  ov.setValue (2);
                  expect (count == 1);
                  ov.setValue (3);
                  expect (count == 2);
                  // remove the update fn
                  ov.onPropertyChange (OneValue::valId, nullptr);
                  ov.setValue (4);
                  expect (count == 2);

                  Vec2 pt { "point", 0, 0 };
                  count = 0;
                  pt.onPropertyChange (pt.x, callback);
                  // you can also set a callback on a value member directly without
                  // needing to bring its owning tree/object into matters at all.
                  pt.y.onPropertyChange (callback);

                  pt.x = 100;
                  expect (count == 1);
                  // don't trigger callback if we don't change value
                  pt.x = 100;
                  expect (count == 1);
                  pt.y = -50;
                  expect (count == 2);
              });
        test ("force updates",
              [&] ()
              {
                  OneValue ov (22);
                  OneValue ov2 (ov);
                  int count { 0 };
                  ov2.onPropertyChange (OneValue::valId, [&count] (juce::Identifier) { ++count; });
                  ov.setValue (2);
                  expect (count == 1);
                  ov.setValue (2);
                  expect (count == 1);
                  ov.forceUpdate (true);
                  ov.setValue (2);
                  expect (count == 2);
                  ov.forceUpdate (false);
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

                  // test scoped force updates.
                  {
                      ScopedForceUpdater sfu (ov);
                      ov.setValue (3);
                      expect (count == 5);
                  }
                  ov.setValue (3);
                  expect (count == 5);
              });

        test ("exclude listeners",
              [&] ()
              {
                  OneValue ov (22);
                  int count { 0 };
                  ov.onPropertyChange (OneValue::valId, [&count] (juce::Identifier) { ++count; });
                  OneValue ov2 (ov);
                  int count2 { 0 };
                  ov2.onPropertyChange (OneValue::valId, [&count2] (juce::Identifier) { ++count2; });

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
                  Vec2 pt ("point", root);
                  expectWithinAbsoluteError<float> (pt.x, 0.f, 0.001f);
                  expectWithinAbsoluteError<float> (pt.y, 0.f, 0.001f);
                  pt.x = 3.1f;
                  pt.y = -1.9f;

                  // init from the root tree
                  Vec2 pt2 ("point", root);
                  expectWithinAbsoluteError<float> (pt2.x, 3.1f, 0.001f);
                  expectWithinAbsoluteError<float> (pt2.y, -1.9f, 0.001f);
              });

        test ("create/wrap hierarchy",
              [this] ()
              {
                  cello::Object root ("root", nullptr);
                  expect (root.getType ().toString () == "root");
                  // create 3 levels below root
                  cello::Object fooBarBaz { "/foo/bar/baz", root };
                  juce::ValueTree dbgTree { root };
                  //   DBG (dbgTree.toXmlString ());
                  expect (fooBarBaz.getType ().toString () == "baz");
                  expect (fooBarBaz.getCreationType () == Object::CreationType::initialized);
                  // find bar relative to the root, but starting at the bottom level.
                  cello::Object bar { "/foo/bar", fooBarBaz };
                  expect (bar.getType ().toString () == "bar");
                  // find ancestor named "foo" relative to baz.
                  cello::Object foo { "^foo", fooBarBaz };
                  expect (foo.getType ().toString () == "foo");
              });

        test ("change notify tree",
              [&] ()
              {
                  cello::Object root ("root", nullptr);
                  Vec2 pt ("point", root);
                  // init from the root tree
                  Vec2 pt2 ("point", root);
                  float x {};
                  float y {};
                  pt2.onPropertyChange ("point",
                                        [&] (juce::Identifier)
                                        {
                                            x = pt2.x;
                                            y = pt2.y;
                                        });
                  expectWithinAbsoluteError<float> (pt2.x, 0.f, 0.001f);
                  expectWithinAbsoluteError<float> (pt2.y, 0.f, 0.001f);

                  // replace the entire tree with a new one (by copying its values)
                  pt = Vec2 ("point", 101.1f, -33.2f);
                  expectWithinAbsoluteError<float> (x, 101.1f, 0.001f);
                  expectWithinAbsoluteError<float> (y, -33.2f, 0.001f);
                  expectWithinAbsoluteError<float> (pt2.x, 101.1f, 0.001f);
                  expectWithinAbsoluteError<float> (pt2.y, -33.2f, 0.001f);
              });

        test ("set property lambda",
              [&] ()
              {
                  cello::Object root ("root", nullptr);
                  Vec2 pt ("point", root);
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
                  Rect box ("box", 100, -100, 200, 250);
                  Vec2 origin { "origin", &box };
                  expectWithinAbsoluteError<float> (origin.x, 100.f, 0.001f);
                  expectWithinAbsoluteError<float> (origin.y, -100.f, 0.001f);
                  Vec2 size { "size", &box };
                  expectWithinAbsoluteError<float> (size.x, 200.f, 0.001f);
                  expectWithinAbsoluteError<float> (size.y, 250.f, 0.001f);

                  // check a default initialized box as a child of another object.
                  cello::Object root ("root", nullptr);
                  Rect rect ("rect", &root);
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
                  juce::String missingDefault { "MISSING" };
                  juce::String missing { o.getattr (foo, missingDefault) };
                  expect (missing == missingDefault);
              });
        test ("lists of objects",
              [&] ()
              {
                  cello::Object list { "objectList", nullptr };
                  expectEquals (list.getNumChildren (), 0);

                  int newChildIndex { -1 };
                  int oldChildIndex { -1 };

                  Object::ChildUpdateFn callback = [&] (juce::ValueTree&, int oldIndex, int newIndex)
                  {
                      oldChildIndex = oldIndex;
                      newChildIndex = newIndex;
                  };

                  list.onChildAdded = callback;

                  // create some objects and add them as children
                  for (int i { 0 }; i < 10; ++i)
                  {
                      Vec2 point { "point", (float) i, (float) i * 2 };
                      list.append (&point);
                      expectEquals (newChildIndex, i);
                      expectEquals (oldChildIndex, -1);
                  }
                  expectEquals (list.getNumChildren (), 10);
                  // check the values
                  for (int i { 0 }; i < list.getNumChildren (); ++i)
                  {
                      auto child { list[i] };
                      expectEquals (child.getType (), juce::Identifier ("point"));
                      Vec2 pt { "point", child };
                      expectWithinAbsoluteError<float> (pt.x, i, 0.001f);
                      expectWithinAbsoluteError<float> (pt.y, i * 2, 0.001f);
                  }

                  // move around
                  list.onChildMoved = callback;
                  list.move (0, 9);
                  expectEquals (newChildIndex, 9);
                  expectEquals (oldChildIndex, 0);
                  list.move (1, 0);
                  expectEquals (newChildIndex, 0);
                  expectEquals (oldChildIndex, 1);

                  // delete
                  list.onChildRemoved = callback;
                  list.remove (9);
                  expectEquals (oldChildIndex, 9);
                  expectEquals (newChildIndex, -1);
                  list.remove (4);
                  expectEquals (oldChildIndex, 4);
                  expectEquals (newChildIndex, -1);
              });

        test ("save/load XML",
              [&] ()
              {
                  // loop through the formats that we support and verify
                  // store / re-load behavior
                  for (auto format : { cello::Object::FileFormat::xml, cello::Object::FileFormat::binary,
                                       cello::Object::FileFormat::zipped })
                  {
                      cello::Object root ("root", nullptr);
                      Vec2 pt1 ("pt1", root);
                      pt1.x = 5;
                      pt1.y = 10;
                      Vec2 pt2 ("pt2", root);
                      pt1.x = 6;
                      pt1.y = 11;
                      Vec2 pt3 ("pt3", root);
                      pt1.x = 7;
                      pt1.y = 12;

                      juce::TemporaryFile tempFile;

                      const juce::String fileName { tempFile.getFile ().getFullPathName () };
                      expect (root.save (fileName, format));

                      cello::Object recoveredRoot ("root", fileName, format);
                      juce::ValueTree rootTree { root };
                      juce::ValueTree recoveredRootTree { recoveredRoot };
                      expect (rootTree.isEquivalentTo (recoveredRootTree));
                      juce::File f { fileName };
                      expect (f.deleteFile ());
                  }
              });

        test ("set/get attr",
              [&] ()
              {
                  cello::Object root ("root", nullptr);
                  root.setattr ("intVal", 11);
                  expectEquals (root.getattr<int> ("intVal", 0), 11);

                  // chain set operations together -- we can use CTAD, so there's
                  // no need to be explicit here about the template type.
                  root.setattr ("floatVal", 45.7f)
                      .setattr ("stringVal", juce::String ("this is a string"))
                      .setattr ("anotherInt", 10101);

                  expect (root.hasattr ("anotherInt"));
                  expectEquals (root.getattr<int> ("anotherInt", {}), 10101);
                  root.delattr ("anotherInt");
                  expect (!root.hasattr ("anotherInt"));
                  expect (!root.hasattr ("nonexistent"));
                  expectEquals (root.getattr<int> ("nonexistent", {}), 0);
              });

        test ("parentage change",
              [&] ()
              {
                  cello::Object parent ("root", nullptr);
                  bool childAdded { false };
                  parent.onChildAdded = [&] (juce::ValueTree&, int, int) { childAdded = true; };

                  cello::Object child ("c1", nullptr);
                  bool parentChanged { false };
                  child.onParentChanged = [&] () { parentChanged = true; };

                  parent.append (&child);
                  expect (parentChanged);
                  expect (childAdded);

                  // make sure the parent really has it -- instantiate another
                  // Object from the child tree and verify how it was created.
                  cello::Object child2 ("c1", &parent);
                  expect (child2.getCreationType () == cello::Object::CreationType::wrapped);
              });

        test ("sort children",
              [&] ()
              {
                  cello::Object parent ("root", nullptr);
                  const int childCount { 10 };

                  // create list 9, 8, 7, ...
                  for (int i { 0 }; i < childCount; ++i)
                  {
                      OneValue v { childCount - i - 1 };
                      parent.append (&v);
                  }

                  struct ValSort
                  {
                      int compareElements (const juce::ValueTree& first, const juce::ValueTree& second)
                      {
                          // convert the trees back into the Object type
                          OneValue lhs (first);
                          OneValue rhs (second);
                          if (lhs.getValue () < rhs.getValue ())
                              return -1;
                          if (lhs.getValue () > rhs.getValue ())
                              return 1;
                          return 0;
                      }
                  };

                  ValSort comp;
                  parent.sort (comp, false);

                  // check the sorting.
                  for (int i { 0 }; i < childCount; ++i)
                  {
                      OneValue val { parent[i] };
                      expectEquals (val.getValue (), i);
                  }
              });
    }

private:
    // !!! test class member vars here...
};
static Test_cello_object testcello_object;
} // namespace cello

/// @endcond TEST
