# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Unreleased
*This release is not ABI compatible with `0.14.0` and introduces small API changes*

### Added
- Nodes can now be collapsed using the node's context menu. Collapsed nodes only consists of the node's header, reducing visual clutter. - #282
- Comments can now be embedded into a graph scene. Comments may either be standalone or may be linked to nodes using the comment's context menu. Comments support markdown. - #65
- Shortcut `CTRL+A` can now be used to select all objects in a scene - #292
- A graph can now be duplicated within the explorer using its context menu or using the shortcut `CTRL+D`. - #291
- Introduced so-called User Variables which behave like environment variables per graph hierarchy. These can be edited using the `Edit` menu in the graph view. - #294
- Added drop shadow effect to nodes. - #304
- Added Undo/Redo commands for pausing a node, renaming a node, and adding dynamic ports. - #322
- *Internal:* Added support for scope object, that allows executing graphs with a custom source datatree. - #32

### Changed
- Copying nodes will now always copy connections inbetween the selected nodes, regardless of
  whether the connections were selected or not.
- `CTRL`-Clicking on a node that is already selected now unselects the node, thus making selections behave more intuitively. - #295
- Resizing a node or comment now creates and undo command. - #290, #306
- *Internal:* Moved utility functions from `intelli/globals.h` to `intelli/utilities.h`.
- *Internal:* Implemented copy, move, and group functions in `intelli/graphutilities.h`.
- *Internal:* Major refactoring of graph scene and related graphics objects.

### Fixed
- Fixed a bug where small mouse movements would not cause the widget to resize. - #288
- Fixed theme not updating Graph View, Scene, and Nodes c orrectly. - #301
- Fixed a bug where deleting ports of input/output providers would result in an undefined/unstable internal node state. - #323
- The graph editor now updates its title when the associated graph changes its object name. - #322
- *Internal:* `GraphBuilder` now also allows creating connections between ports with different but compatible type ids.

## [0.14.0] - 2025-04-04
*This release is considered ABI compatible with `0.13.0`*

### Added
- Added dummy nodes that are created for missing node classes (dummy objects). Dummy nodes allow to view graphs with unknown nodes and allow the deletion of these nodes. - #107

### Changed
- Moving a node now creates an undo command. - #41

### Fixed
- Fixed `Node::evaluated` being triggered twice per evaluation. - #278
- Fixed instantiation of outgoing connections when grouping nodes. - #277
- Fixed slider input mode for number input nodes not committing value. - #272
- The overlay buttons in a graph scene no longer create undo/redo commands. - #271
- Nodes that have the `UserDeletable` flag set, can no longer be deleted. A popup is displayed to notify the user in such a case. Custom delete actions are used, if the shortcut of an UI action matches the delete shortcut. - #270
- Connections to unknown nodes are no longer deleted upon loading a project with missing modules/unknown nodes. - #107

## [0.13.0] - 2025-03-19
*This release is not ABI compatible with `0.12.0` and may break API*

### Added
- Subgraphs can now also be expanded, thus allowing to "ungroup" subgraphs previously created. - #64
- Allowed nodes to be marked as deprecated. Deprecated nodes should be replaced as soon as possible as a future release may remove the node entirely. - #225
- Nodes can now have a "display icon" that is drawn in the node header. - #236

### Changed
- Major refactoring of the graph execution model. A single execution model is used to manage and evaluate the entire graph hierarchy, improving the handling of nested graphs and fixing previously unhandled edge cases. Additionally a helper object returned by the execution model when executin nodes/graphs is now more capable, allowing the registration of async callback functions. - #112
- IntelliGraph specific compile flags were renamed; node's size and position properties are now always hidden unless the associated compiler flag is set. - #205
- Most inputs nodes have been refactored and streamlined. Duplicate nodes have been removed. - #224

### Fixed
- Fixed an error when creating and loading a project with an empty IntelliGraph package in GTlab 2.1.0 - #193
- Fixed potential crashes when deleting instances of `ObjectData` that were created in a separate thread. - #112
- Fixed port captions spanning multiple words being truncated. - #212
- The graph view should now remember more consistently whether a graph should be auto-evaluated. - #112
- If a node fails to evaluate all successor nodes are invalidated and marked as failed as well. - #221
- Fixed menus of embedded widgets not beeing displayed outside of the node's bounding rect.

## [0.12.0] - 2024-09-24
### Added
- Allowed nodes and ports to have custom tooltips. - #110

### Changed
- The PortId-generation of output- and input-providers had to be reworked, thus nested graphs imported from old version of this module are missing connections - #111 
- API: Refactored the "connection model" and provided leaner and more flexible access to connection data of nodes. This change may improve general performance of large graphs. - #111
- API: One can now access a "global connection model" as well, in which an entire graph hierarchy is flattened. To access nodes one has to use `uuid`- property of nodes. - #111

### Fixed
- Fixed hidden ports being rendered incorretly. Hidden ports are still accessible through a port id and port index. - #111
- API: Fixed `portConnected` and `portDisconnected` being emitted incorrectly. - #114

### Removed

## [0.11.0] - 2024-07-22
### Added
- Added the option to snap nodes to the grid. - #44
- Added node to execute calculators. The calcualtors must be registered beforehand as not all calculators may work properly. - #92
- Added nodes for obtaining a file handle that can be used to read from or write to a file. - #101
- Added project info node to access project specific properties. - #101
- Added generic node to display text. Supports multiple synatx highlighters. - #101

### Changed
- Updated rendering of the grid to show minor and major grid lines. - #95
- Updated highlighting of nodes, connections, and ports when creating a draft connection to further emphasize compatibilities. - #94
- Allowed nodes be resized horizontally only if desired. - #101
- Changed the memento node to output a memento instead of displaying an objects memento. - #101
- It is now possible to register conversions for node data types. This allows to connect different node data types. - #77

### Fixed
- Fixed pop-up window when starting GTlab. - #93
- Fixed compatible ports sometimes not beeing highlighted correctly when creating a draft connection. - #94
- Updating the type of a port will now check if associated connections are still valid and remove incompatible connections. - #78

### Removed
- Removed `StringListData` and its node. - #80

## [0.10.4] - 2024-06-18
### Fixed
- Fixed bad logging behaviour 

## [0.10.3] - 2024-04-09
### Fixed
- Fixed a crash when grouping nodes and reverting the action, while the subgraph is opened. - #87
- Clicking the widget of a node will now also select the object application wide and thus update the properties dock. - #88

## [0.10.2] - 2024-04-08
### Changed
- Reduced and updated logging output. - #85

### Removed
- Removed `IntegerData` in favor of `IntData`. This might lead to breaking changes in existing graphs. - #86

## [0.10.1] - 2024-04-08
### Changed
- Updated visuals of nodes and connections - #67
- Grid state will now be saved per graph instead globally - #67
- Clicking a widget will now select the node - #67
- Connections that share the same out node and out port are merged when grouping nodes - #67

### Added
- Added entry to the scene menu to center the scene - #67
- Added entry to the scene menu to cycle through multiple connection styles (straight, cubic, rectangle). The connection shape is saved persistently per graph. - #67
- Added tooltips to Node Ports and EvalState-Visualizer. - #67

### Fixed
- Fixed imprecise port hit registration - #67
- Fixed Node Size and Position now beeing hidden in non-dev mode - #79

## [0.9.0] - 2024-03-27
### Changed
- Position and Size Properties are now hidden for Nodes in non-dev-mode - #54
- Graphs are no longer auto-evaluating by default and will remember the last state they where in. The property `Is Node Active` of the Graph can be used to enable/disable auto-evaluation. #61

### Added 
- Integrated a traffic light system, to denote the state of the node: Green = Evaluated and Valid Outputs; Yellow = Not Evaluated; Red = Invalid data at output (Execution not successful). - #56
- A dedicated button has been added to Graph View, which can be used to enable/disable auto-evaluation. - #61

### Fixed
- Fixed random crashes when deleting a node with widgets - #76
- Fixed evaluation of graphs when input provider is not connected to output provider - #75

## [0.8.0] - 2024-02-19
### Added
- Add some basic data types and their source nodes - #57
- Add invokable functions to make node data accessable in python scripts - #58
- DynamicNodes can now be assigned whitelists, allowing to add ports of certain types - #59

## [0.7.2] - 2023-11-23
### Changed
- Update of the cmake configuration to use code in other projects - #50

## [0.7.1] - 2023-11-22

First official release

### Added
- Caption may be renamed using the properties dock widget
- Added group nodes, that allows for nested intelli graphs. Group nodes can be created using the context menu of a node selection and may be opened using a double click. - #5
- Nodes can be unique effectively existing only once within an intelli graph. Unique nodes will be highlighted - #18
- Nodes will now remember and restore their size - #11
- Nodes will now have the same context menu as in the explorer. Additional actions for grouping oder deleting selected nodes have been added. In addition ports can have their own context menus - #19
- Node UI actions may be triggered using the registered shortcuts inside the graph view - #24
- `intelli::DynamicNode` that allows the user to add/remove ports at runtime and store added ports persistently - #21
- Nodes can be toggled between automatic and manual evaluation - #27
- Nodes will be saved in separate project files (`<project_dir>/intelli`) - # 22

### Changed
- Lots of internal refactoring, better stability and ease of use - #8
- By default each node will be executed in parallel - #21
- The widgets of nodes can now recieve mouse wheel events - #13
- Sorted entries in node scene menu - #46
- Implemented own execution model to handle evaluations of nodes. Nodes will now only be evaluated once all inputs are valid, reduces redundant/multiple evaluations of a single node - #28

### Fixed
- Nodes that are not deletable in the explorer, can no longer be deleted using the editor - #4
- Adding and editing a port caption at runtime will resize the node properly - #14
- Undo/Redo will are now merged and no longer devided into many undo/redo commands -16

## [0.1.0] - 2023-06-06
### Added
 - First release
