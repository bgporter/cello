# Cello

Classes for working with juce ValueTree objects. 

**Fall 2022 * Brett g Porter**

brett@bgporter.net

## Motivation and Overview

### `cello`

In short, my goal was: create a set of C++ classes that I can derive my own classes from where member variables are stored transparently in JUCE ValueTrees instead of directly in those object instances, combining the comfort and simplicity of working with normal-looking C++ code with the benefits and tradeoffs of ValueTrees. 

Something like:

```cpp

struct CelloDemo : public cello::Object 
{
    // we'll figure this type out shortly...
    SomeMagicType<int> x {};
    SomeMagicType<float> y {};
};

CelloDemo demoObject;
demoObject.onPropertyChange(demoObject.x, [&demoObject]() 
{ 
    std::cout << "x changed to " << demoObject.x; 
});

demoObject.x = 100;

// stdout should print: "x changed to 100"
```
## Values

t/k

- 
## Objects

t/k
