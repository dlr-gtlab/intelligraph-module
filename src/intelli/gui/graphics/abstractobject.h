/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 12.3.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_ABSTRACTGRAPHICSOBJECT_H
#define GT_INTELLI_ABSTRACTGRAPHICSOBJECT_H

#include <intelli/exports.h>

#include <QGraphicsObject>

namespace intelli
{

class Graph;
class GraphStyleInstance;

class GT_INTELLI_EXPORT AbstractGraphicsObject : public QGraphicsObject
{
    Q_OBJECT

public:
    
    AbstractGraphicsObject(Graph& graph_,
                         GraphStyleInstance& style_,
                         QGraphicsObject* parent = nullptr);

    Graph& graph() { return *m_graph; }
    Graph const& graph() const { return *m_graph; }

    GraphStyleInstance& style() { return *m_style; }
    GraphStyleInstance const& style() const { return *m_style; }

private:

    Graph* m_graph;
    GraphStyleInstance* m_style;
};

} // namespace intelli

#endif // GT_INTELLI_ABSTRACTGRAPHICSOBJECT_H
