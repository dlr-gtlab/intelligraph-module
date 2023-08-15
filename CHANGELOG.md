# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
- Caption may be renamed using the properties dock widget
- Added group nodes, that allows for nested intelli graphs. Group nodes can be created using the context menu of a node selection and may be opened using a double click. - #5
- Nodes can be unique effectively existing only once within an intelli graph. Unique nodes will be highlighted - #18
- Nodes will now remember and resotre their size - #11
- Nodes will now have the same context menu as in the explorer. Additional actions for grouping oder deleting selected nodes have been added. In addition ports can have their own context menus - #19
- Node UI actions may be triggered using the registered shortcuts inside the graph view - #24
- `intelli::DynamicNode` that allows the user to add/remove ports at runtime and store added ports persistently - #21
- Nodes can be toggled between automatic and manual evaluation - #27

### Changed
- Lots of internal refactoring, better stability and ease of use - #8
- By default each node will be executed in parallel - #21
- The widgets of nodes can now recieve mouse wheel events - #13

### Deprecated
### Removed
### Fixed
- Nodes that are not deletable in the explorer, can no longer be deleted using the editor - #4
- Adding and editing a port caption at runtime will resize the node properly - #14
- Undo/Redo will are now merged and no longer devided into many undo/redo commands -16

### Security

## [0.1.0] - 2023-06-06
### Added
 - First release
