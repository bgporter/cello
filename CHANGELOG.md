# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Unreleased 

### Added 

- `Object::remove(const cello::Query&)` will remove any children that are filtered by the query argument, returning the number of children that were removed as a result. 
- new `ComputedValue<T>` class; provides a `Value`-like API to functions that are not backed in the ValueTree as such (useful for conversions, etc.)

### Changed

### Fixed 

## 1.6.0 * 2025-08-21

### Added

- the `MAKE_VALUE_MEMBER` macro now creates a static `juce::Identifer` that's used by all instances of the class to instantiate that member instead of re-converting from a string each time. Should be faster, and use the Identifier objects as they're intended to be used. 
- Added `CACHED_VALUE(name, value)` macro to simplify creating cached value members for `cello::Value` objects.
- Added `get()` method to `Value<T>::Cached` for easier access to the cached value (e.g., you can use it to initialize an `auto` variable).
- Added support for bidirectional synchronization between `cello::Object`s in different threads via the new `cello::SyncController` class, which manages a pair of `Sync` objects and prevents feedback loops during updates.
- Added new methods to `SyncController` for performing updates: `performNextUpdate` and `performAllUpdates` for a given thread.
- Added new tests for bidirectional sync and feedback prevention.

### Changed 

- changed the signature of the `Value<T>::onSet()` callable. This now returns `std::optional<T>`; if this callable returns `std::nullopt`, the Value will not be modified, so you can ignore set calls that are invalid, or always return `nullopt` from your set validator to treat the Value as if it were const. 

### Fixed

- type of first argument to a `PropertyChangeFn` changed from `juce::Identifier` to `const juce::Identifier&`, which it always should have been.
- cleaned up `Value` arithmetic operators by replacing noisy static_cast calls with the recently added `Value::get()` method that accomplishes the same thing.
- Improved feedback prevention in synchronization logic to avoid infinite update loops when syncing between objects.
- Updated `IpcClient` to implement `startUpdate` and `endUpdate` for better state management during IPC sync.
- Minor documentation improvements and added missing comments for clarity.

## 1.5.0 * 2025-02-09 

### Added 

- add `get()` method to value, simplfying accessing the underlying data of a `Value` in its correct compile-time type. 
- fix behavior of listeners on an Object's type to return the ID of the property that has changed, not the ID of the tree itself, which isn't that useful. 
- add `Object::wasWrapped()` and `Object::wasInitialized()` methods. 
- add `Object::getTypeName()` for convenience. 
- add `Object::toXmlString()` for convenience. 

## 1.4.0 * 2024-05-01

- Added new `Object::findOne()` method to search for and return a single child tree from an object. 
- Added new argument `returnFirstFound` to `Query::search`, used to implement the above. 
- Fixed some compiler warnings. 

## 1.3.1 * 2024-03-29

- Cleaned up some buggy unit tests


## 1.3.0 * 2024-03-02

### Added

- `cello::Sync`: Perform thread-safe Object updates using the juce::ValueTreeSynchroniser class. The basic approach is that we rely on a pair of `cello::Object`s, each of which is only accessed by a single thread. A separate `cello::Sync` object (or pair of them if bidirectional sync is needed) watches for changes and communicates those deltas across the thread boundary where they're applied in a thread-safe manner to the other object/tree. 
- `cello::IpcClient` (and related): Connect cello::Objects in separate processes using TCP or Named Pipe-based interprocess communications. This uses a similar approach, but communicates the deltas across process boundaries using the JUCE interprocess communication API. 

## 1.2.0 * 2023-11-12

### Added 

- new ctor that accepts another `Object` by reference.
- "Path" functionality that permits finding/constructing trees/Object by using an absolute or relative path. An impact of this change is that the `type` parameter passed to `cello::Object` constructors has been changed from `juce::Identifier&` to `juce::String&`.
- `PropertyUpdateFn` alias moved from the `Object` class into the top-level `Cello` namespace (defined in the `cello_update_source.h` header)
- added `Value::onPropertyChange()` to simplify subscribing to value objects that are public; instead of doing e.g. `myObject.onPropertyChange (myVal, callback)` you can just call `myVal.onPropertyChange(callback)`. 
- added `cello::Value<T>::Cached` class to store the last updated state of a `Value` object to make it usable without requiring a ValueTree lookup and validation each time it's needed. 

### Fixed

- `creationType` bug on re-wrapping a ValueTree.

## 1.1.3 * 2023-03-18 
### Fixed 

- template error in `Object:getattr`

## 1.1.2 * 2023-03-16 
### Fixed

- `Object::save` now ensures that its file is created before attempting to save.
- `Object::save` returns a `juce::Result` instead of bool, and will indicate the reason for a failure in that return value. 

## 1.1.1 * 2023-03-14 
### Fixed 
- some template errors. 

## 1.1.0 * 2023-02-19 
### Added 

- `cello::Query` class and updates to `cello::Object` to perform database-like queries and in-place updating of child objects. See the "Database / Query" section of the README document. 

## 1.0.1 * 2023-02-05 
### Added
- MIT license text to all source files. 
- [Doxygen Awesome](https://github.com/jothepro/doxygen-awesome-css) CSS/etc to document generation.

## 1.0.0 * 2023-01-01 

Original release. 