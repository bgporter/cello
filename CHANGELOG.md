# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Unreleased 

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

## Release 1.1.0 * 19 Feb 2023
- Added `cello::Query` class and updates to `cello::Object` to perform database-like queries and in-place updating of child objects. See the "Database / Query" section of this README document. 

## Release 1.0.1 * 05 Feb 2023

- added MIT license text to all source files. 
- added [Doxygen Awesome](https://github.com/jothepro/doxygen-awesome-css) CSS/etc to document generation.

## Release 1.0.0 * 01 Jan 2023

Original release. 