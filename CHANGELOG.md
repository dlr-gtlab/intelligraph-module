# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
- Caption may be renamed using the properties dock widget
- Added group nodes, that allows for nested intelli graphs. Group nodes can be created using the context menu of a node selection and may be opened using a double click. - #5
- Nodes can be unique effectively existing only once within an intelli graph. Unique nodes will be highlighted - #18

### Changed
- Lots of internal refactoring, better stability and ease of use - #8

### Deprecated
### Removed
### Fixed
- Nodes that are not deletable in the explorer, can no longer be deleted using the editor - #4
- Adding and editing a port caption at runtime will resize the node properly - #14

### Security

## [0.1.0] - 2023-06-06
### Added
 - First release
