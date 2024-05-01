# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## 1.4.0 * 2024-05-01

- Added new `Object::findOne()` method to search for and return a single child tree from an object. 
- Added new argument `returnFirstFound` to `Query::search`, used to implement the above. 
- 

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