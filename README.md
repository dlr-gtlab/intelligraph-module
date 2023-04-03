# GTlab-IntelliGraph

## Next Step
- [ ] QtNodes als neues Projekt in GTlab-CoreDev-Sandbox (Fork)
- [ ] Einbindung des QtNodes Repo als Submodule im GTlab-IntelliGraph (statisch gelinkt)
- [ ] Aufbau GTlab Klassenstruktur (GtIntelliGraph; GtNodeCollection; GtIntelliGraphNode; GtIgObjectSourceNode...)
- [ ] Aufbau entsprechender NodeDelegateModels (z.B. NdsObjectLoaderModel (GtIgObjectSourceNode))
- [ ] Implementierung Konverter Memento->JSON; JSON->Memento
- [ ] Erstellung IntelliGraph-View aus JSON

## Fahrplan
- [ ] Nutzung eines IntelligGraph-Memento in einer IntelliGraphNode
- [ ] Ausführung der Nodes in separaten Threads (Eigener Thread-Pool; Abstrakte NodeDelegateModel-Klasse für GTlab-NodeKlassen - compute() funktion)
- [ ] Erstellung einer Task-Node; Dynamischer Scope (Object-DataFlow)
- [ ] Implementierung einer DataFlow-Datenstruktur
- [ ] Filter-Funktionalitäten (Pandas/DataFrame)
