# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
- Added the option to snap nodes to the grid. - #44

### Changed
- Updated rendering of the grid to show minor and major grid lines. - #95

### Fixed
- Fixed pop-up window when starting GTlab. - #93

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
