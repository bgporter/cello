# Cello

Classes for working with juce ValueTree objects. 

**Fall 2022 * Brett g Porter**

brett@bgporter.net

## Motivation and Overview

### Confessions of a `ValueTree` Skeptic

I've been using the JUCE framework for over a decade now, but there's a major component of JUCE that never clicked for me as a developer &mdash; ValueTrees. This wasn't a problem for me until I changed jobs and started needing to work on a mature codebase that made significant use of them. This code makes efforts to hide some of the more cumbersome or repetitive aspects of integrating ValueTrees into an application, but that `ValueTreeWrapper` class still seemed like it required too much effort to work with; where I'm used to thinking in terms of objects that contain values, any time I needed to get near data that's stored in a ValueTree, it was impossible to avoid the awareness that I was always working through an API to perform operations on data that should just be directly manipulable, and while the wrapper class approach mitigated this to some extent, there was still more boilerplate code to write than seems good to me, as well as other places where the gaps around the abstraction were more obvious than I like. 

I've always found that the only way for me to work through these issues when I encounter them is to sit down with a blank document in an editor and start enumerating the problems that I see with a system and use that as a guide to start thinking about ways that I can engineer around the parts that aren' tmy favorite, and sometimes how I can reframe my thinking to start seeing superpowers where I thought there were deficiencies. 

One of my current teammates has expressed confusion that I wasn't immediately on board with ValueTrees, and his defense of them was key to my eventually starting this re-analysis. They give you: 

- A really easy way to capture and pass around the entire state of your application's data at run time
- A rich mechanism to watch that data at a fine degree of granularity
- Trivially easy persistence of application state

...but at the cost (in comparison to using native POD or class/struct variables) of being:
- slower
- less convenient to use
- less type-safe, since all values are stored in the JUCE `var` variant type. 

As I started listing the tradeoffs, I considered ways to work around the convenience and type-safety issues. I also reflected on the years in my career when I wrote far more Python code than I did C++, and many of these same charges can be filed against that language, which I love. 

At one level, you can look at Python as being nothing but a bunch of associative arrays (or in python, `dict`s) with the ability to be manipulated dynamically by code. Once I started thinking in those terms, the project became much more interesting. 

As frequently happens with me, these thoughts sat collecting dust in a document until I hit upon a name for the project -- `cello`, short for 'cellophane' (since the code is wrapping a ValueTree)

### `cello`

In short, my goal was: create a set of C++ classes that I can derive my own classes from where member variables are stored transparently in JUCE ValueTrees instead of directly in those object instances, combining the comfort and simplicity of working with normal-looking C++ code with the benefits and tradeoffs of ValueTrees. 

Something similar to:

```cpp

// define a struct with two members and then create an instance
struct CelloDemo : public cello::Object 
{
    // we'll figure this type out shortly...
    cello::Value<int> x;
    cello::Value<float> y;
};

CelloDemo demoObject;

// give that object a lambda to call whenever the value of `x` changes. 
demoObject.onPropertyChange(demoObject.x, [&demoObject] () 
{ 
    std::cout << "x changed to " << demoObject.x << "\n"; 
});

// after executing this line, stdout should print: "x changed to 100"
demoObject.x = 100;

```
## Values

- actually, a proxy to a value. We store a `juce::Identifier` and a reference to a ValueTree that provides the actual storage; storing or retrieving the value through its variable needs to do so through the ValueTree API, but that's all kept out of sight. 
- can be set to always update their listeners when the value is set, even if the underlying value wasn't changed. 
- can be given validator functions that will be called when the value is set or retrieved.
- arithmetic types have all of the in-place operators (`++`, `--`, `+=`, `-=`, `*=`, `/=`) defined.
- can be used to access any C++ value data type for which a `juce::VariantConverter` struct has been defined. 

`cello::Value` objects only make sense as members of a class derived from `cello::Object` (below). The signature of the Value constuctor is:

```cpp
template <typename T>
Value::Value (Object& object, const juce::Identifier& id, T initVal);
```

...so at creation time, a value knows:
1. the Object holding a ValueTree where its storage is located
2. the identifier of this piece of data in the value tree
3. how to initialize that data if needed
4. The data type to use outside of the ValueTree. Because we use the `VariantConverter` facility in JUCE, almost any type of data can be converted to/from the `var` variant type. 

So, declaring an instance of this type templated on `int` as a member of a `cello::Object` object would look like 

```cpp
cello::Value<int> x { *this, "x", {} };
```

We pass a reference to the owning object, the ID to use, and its default initial value. By convention, we use the same name for the member variable as for its Identifier in the ValueTree. 

We define a macro in `cello_value.h` that's less cumbersome and less potentially error-prone to do the same thing: 

```cpp
#define MAKE_VALUE_MEMBER(type, name, init) \
    cello::Value<type> name { *this, #name, init };
```

...so the above declaration would be `MAKE_VALUE_MEMBER (int, x, {});`. Once a `cello::Object` containing this declaration is instantiated, you can manipulate that value almost exactly the same as if it were an actual instance of the underlying type ("almost exactly" here covers edge cases like `sizeof` giving different results, and probably others that I haven't considered yet):

```cpp
// after each of these lines, any property change callbacks watching 
// `x` will be called. 
myObj.x = 20;
--myObj.x;
myObj.x *= -3;
```
### `VariantConverter`s

By defining a template specialization of the `juce::VariantConverter` struct, you can store more complex value types by cleverly packing them inside one of the more interesting `var` variants that exist -- in this example from the `cello` unit tests, we use the fact that an `Array` of `var`s is a `var`:

```cpp
namespace juce
{
/**
 * @brief A variant converter template specialization for
 * std::complex<float> <--> & juce::var.
 * VariantConverter structs need to be in the juce namespace for
 * them to work properly.
 */
template <> struct VariantConverter<std::complex<float>>
{
    static std::complex<float> fromVar (const var& v)
    {
        if (const auto* array = v.getArray (); array != nullptr && array->size () == 2)
            return { array->getUnchecked (0), array->getUnchecked (1) };
        jassertfalse;
        return {};
    }

    static var toVar (const std::complex<float>& val)
    {
        Array<var> array;
        array.set (0, val.real ());
        array.set (1, val.imag ());
        return { std::move (array) };
    }
};

} // namespace juce
```

Then we define a class that has a single public Value member that contains a `std::complex<float>` -- there's no additional work required to perform the conversions:

```cpp
class ObjectWithConvertibleObject : public cello::Object
{
public:
    ObjectWithConvertibleObject ()
    : cello::Object ("convertible", nullptr)
    {
    }
    // verify the automatic use of variant converters.
    MAKE_VALUE_MEMBER (std::complex<float>, complexVal, {});
};
```

Your code is then free to work with that value directly: 

```cpp
ObjectWithConvertibleObject o;
std::complex<float> orig { 2.f, 3.f };
o.complexVal = orig;

std::complex<float> retrieved { o.complexVal };
expectWithinAbsoluteError<float> (orig.real (), retrieved.real (), 0.001f);
expectWithinAbsoluteError<float> (orig.imag (), retrieved.imag (), 0.001f);
```
### Validator Functions

If we're taking some inspiration from Python here, it's worth remembering that Python developers are in the practice of leaving all their class member variables public instead of hiding them behind a wall or privacy and forcing the usage of `getVariable()`/`setVariable()` methods to ensure the separation of interface from implementation -- much of the time, there's no reason to require those accessor/mutator methods, and when there is an actual reason (for example, to ensure the maintenance of a class invariant), it's easy to switch over to using a property to manage access to the underlying data. Bertrand Meyer, creator of the Eiffel programming language refers to this as the "Uniform Access Principle," that "...all services offered by a module should be available through a uniform notation, which does not betray whether they are implemented through storage or through computation."

Each `cello::Value` object may have `ValidatePropertyFn` lambdas assigned to it (where that lambda accepts a const reference to `T` and returns a `T` by value) that are (`onSet`) called before that value is stored into the underlying ValueTree or (`onGet`) called after retrieving the property from the ValueTree but before returning the value to calling code. 

Your application can use this facility to modify the value (e.g. to keep it within a valid range), create an entirely new value, make changes to other properties of the ValueTree, create log entries, or anything else that you need to happen at these juncture points. 


### Forcing Update Callbacks

The normal behavior of ValueTrees is to only notify callback listeners of property changes when a value actually *changes*. In practice, it's frequently useful to ensure that any attempt to set a property results in notifications being sent even if setting it to its current value. This can be controlled on a per-value basis by calling that value's `forceUpdate (bool shouldForceUpdate)` method. 

To simplify the common case where this behavior is only meant to be in force for a single update, we provide a utility class `ScopedForceUpdater` that sets the value to force sending updates when the class is constructed, and then clears the update logic when that updater object goes out of scope. 

### Excluding Listeners

It's also common to want to send update callbacks to all listeners except one -- for example, if I have a bit of code that's setting a value and that code is also listening to the value, there's no need to receive a callback; the code already knows what the new value is. The `cello::Value` class provides a method `void excludeListener (juce::ValueTree::Listener* listener)` for this purpose. 

## Objects

### Creation Patterns: Creating vs Wrapping

Since our objects rely on separate ValueTree objects for their storage, we need to support two different mechanisms for creating instances: 

1. If the ValueTree doesn't exist yet, we need to create one and initialize it for use. 
2. When the value tree already exists, we need to 'wrap' it to access its storage and capabilities. 

The constructors of `cello::Object` handle both these cases for us, using the logic outlined below:

* `Object (juce::Identifier type, Object* state);` (preferred)
* `Object (juce::Identifier type, juce::ValueTree tree);`

1. If the `state` or `tree` argument is of type `type`, wrap that inside the object being created. 
2. If the `state` or `tree` arguments has a child of type `type`, wrap that child inside the object beng created. 
3. Else, we create a new ValueTree of type `type` and initialize it as appropriate. If the `state` arg was not null (or the `tree` is valid), add this new tree as a child. 

It is sometimes useful to know whether a new Object was created or wrapped -- for example, it might be an error in your application if a child that's expected to be present isn't. 

You can test this at runtime using the method `Object::getCreationType()`, which will return either:
* `Object::CreationType::initialized`
* `Object::CreationType::wrapped`

### Working with Children

ValueTrees can contain other ValueTrees as children, and it's important to keep in mind that there are two different modes for this containment:

* *Heterogeneous* The parent tree is a data structure that contains other (tree) data structures. Access the children by specifying their type. The children are stored in a list, but the sequence is not significant.
* *Homogeneous* The parent tree contains a list of child trees, typically but not necessarily of the same type. Access the children by their index or iterating through them. 

There's no mechanism to enforce this distinction -- if a list of different types makes sense in your application, there's a little more logic you'll need to write, but that's all. 

#### Adding Children

`void append (Object* object);` adds the child to the end of this object's child list. 

`void insert (Object* object, int index);` adds the child at a specific index in the list; if `index` is out of range (less than zero or greater than the current number of children), the child will be appended to the list. 


#### Removing Children

To remove a child that's already wrapped in an Object, use 

```cpp
 Object* remove (Object* object);
 ```

 On success, this will return the same pointer you passed it; if that Object was not actually a child of this object, will return `nullptr`. 

 To remove a child by its index: 

 ```cpp
juce::ValueTree remove (int index);
```

This will return the raw ValueTree used by that child on success, or an invalid ValueTree on failure. 

#### Finding Children

We provide an `operator[]` to access children by their index:

```cpp
juce::ValueTree operator[] (int index) const;
```

You can also iterate through an Object's children:

```cpp
for (auto childTree: someObject)
{
    // work with the raw ValueTree here, maybe using it to instantiate an object...
}
```

#### Moving / Sorting Children

You can change the position of an individual child using the method

```cpp
void move (int fromIndex, int toIndex);
```

...and sort all the children with the method: 

```cpp
template <typename Comparator> 
void sort (Comparator& comp, bool stableSort);
```

where `Comparator` is an object that contains a method
```cpp
int compareElements (const ValueTree& first, const ValueTree& second);
```

that returns 
* a value of < 0 if the first comes before the second
* a value of 0 if the two objects are equivalent
* a value of > 0 if the second comes before the first

The `stableSort` argument specifies whether the sort algorithm should guarantee that equivalent children remain in their original order after the sort. 
  
### Undo/Redo

Most ValueTree operations accept a pointer to a `juce::UndoManager` object as an argument to make those operations undoable/redoable. `cello::Object`s can maintain this manager for you: pass a pointer to `UndoManager` to a `cello::Object` using its `setUndoManager` method, and that object and any child/descendant objects that are added to it will become undoable. 

The following undo/redo methods are available directly from `cello::Object`: 

* `bool canUndo () const;`
* `bool undo ();`
* `bool canRedo () const;`
* `bool redo ();`
* `void clearUndoHistory ();`

You can also retrieve a pointer to the UndoManager (using `juce::UndoManager* getUndoManager()`) for any of its other operations that we don't expose directly.


### Change Callbacks

The `cello::Object` class defines a set of `std::function`s that can be installed as callbacks to be executed when properties or children of an object are changed:

#### Property Changes

`PropertyUpdateFn` signature: `std::function<void(juce::Identifier)>`

You can register a callback for each named property of an object that will be executed when the value of that property is changed. You can also register a wildcard callback using the identifier of the object itself that will be called when an attribute changes but there was no specific handler for it. 

There are two Object methods to register these callbacks:

* `void onPropertyChange (juce::Identifier id, PropertyUpdateFn callback)` -- pass in the identifier of the attribute to watch
* `void onPropertyChange (const ValueBase& val, PropertyUpdateFn callback);` -- pass in a reference to the `cello::Value` or `cello::Object` to watch. 


#### Child Changes


Changes to children are broadcast using a `ChildUpdateFn` callback that has the signature `std::function<void (juce::ValueTree& child, int oldIndex, int newIndex)>;`

* `onChildAdded` -- `oldIndex` will be -1, `newIndex` will be the index of the new child. 
* `onChildRemoved` -- `oldIndex` will be the index of the child that was removed, `newIndex` will be -1.
* `onChildMoved` -- `oldIndex` and `newIndex` are self-explanatory. 


#### Tree Changes

A `SelfUpdateFn` callback with the signature `std::function<void (void)>` will be called when:

* `onParentChanged` -- this object has been adopted by a different parent tree.
* `onTreeRedirected` -- the underlying value tree used by this object was replaced with a different one. 

### "Pythonesque" access

Not everything can or should be done with the kind of compile-time API `cello` was written to support. These methods take their names and inspriation from similar methods in the Python object model.

These methods do provide some level of type-safety and type-coercion using `VariantConverter`s that our `Value` types have.

* `bool hasattr (const juce::Identifier& attr) const` tests an object to see if it has an attribute/property of the specified type (enabling what the Python world would call 'Look Before You Leap' programming)
* `template <typename T> Object& setattr (const juce::Identifier& attr, const T& attrVal);` sets the value of the specified attribute in the object. We return a reference to the current Object so that multiple calls to this method can be chained together. 
* `template <typename T> T getattr (const juce::Identifier& attr, const T& defaultVal) const` either returns the current value of the specified attribute, or a default value if it's not present. 


### Persistence

`cello::Object` instances can be persisted to or from disk in any of the three formats that ValueTrees support:
* text/XML
* (JUCE proprietary) binary
* binary, compressed with GZIP. 

To save a file, use the `bool save (juce::File file, FileFormat format = FileFormat::xml) const` method, which will write out that tree and all its descendants into the specified file. 

Loading a file is a little more complex; we use a static method `static juce::ValueTree load (juce::File file, FileFormat format = FileFormat::xml)` that attempts to load and return a ValueTree from the specified file; you should then pass that ValueTree (if valid) to the constructor of your application's root Object type and verify that the constructor was able to wrap the tree it was given, code like:

```cpp

const juce::File filePath { "/path/to/my/file.xml" };
auto loadedTree { cello::Object::load(filePath) };

if (! loadedTree.isValid ())
{
    // failed to load the expected file. Maybe this is an error in your app?
    // if so, handle it! 
}

MyRootObject root { loadedTree };
if (root.getCreationType () == cello::Object::CreationType::initialized)
{
    // we didn't load successfully -- if this is an error in your app, 
    // handle it. 
}
// else, we've re-loaded -- carry on! 
```

## Missing Pieces

There are parts of the `juce::ValueTree` API that are not available through the `cello` API; these may be added later, or you can use them directly by accessing the `ValueTree` object that an `Object` already owns. 

