# GTlab-IntelliGraph

## Based on QtNodes
*IntelliGraph is based on [QtNodes by paceholder et al.](https://github.com/paceholder/nodeeditor) (version 2.2.2). QtNodes was adapted and extended in various ways and deeply integrated into the GTlab Framework.*

Following features were added/changes were made:

- Added ability to group nodes.
- A dedicated graph execution model handles the correct execution of nodes and manages the transfer of data btween nodes.
- Each node can have its own painter and geometry object and thus be styled individually.
- Connections can be renderes in various shapes (Cubic, Rectangle and Straight).
- Nodes and their graphical representation are loosly coupled, allowing IntelliGraphs to be executed without a graphical user interface.
- Graphs are serialized using GTlab's Memento System instead of exporting the data to JSON.