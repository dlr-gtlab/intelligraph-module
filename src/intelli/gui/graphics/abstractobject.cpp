/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 12.3.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include <intelli/gui/graphics/abstractobject.h>
#include <intelli/gui/style.h>

using namespace intelli;

AbstractGraphicsObject::AbstractGraphicsObject(Graph& graph_,
                                           GraphStyleInstance& style_,
                                           QGraphicsObject* parent) :
    QGraphicsObject(parent),
    m_graph(&graph_),
    m_style(&style_)
{

}

