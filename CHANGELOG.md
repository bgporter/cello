# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Unreleased 

### Added 

- new ctor that accepts another `Object` by reference.

### Fixed

- `creationType` bug on re-wrapping a ValueTree.

## 1.1.3 * 2023-03-18 
### Fixed 

- template error in `Object:getattr`

## 1.1.2 * 2023-03-16 
### Fixed

- `Object::save` now ensures that its file is created befor attempting to save.
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