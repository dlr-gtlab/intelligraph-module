/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GRAPHVIEW_H
#define GT_INTELLI_GRAPHVIEW_H

#include <gt_graphicsview.h>

class QMenu;
class QPushButton;

namespace intelli
{

class Graph;
class GraphScene;
class GraphSceneSelector;

class GraphView : public GtGraphicsView
{
    Q_OBJECT

public:

    struct ScaleRange
    {
        double minimum = 0;
        double maximum = 0;
    };
    
    GraphView(QWidget* parent = nullptr);

    void setScene(GraphScene& scene);

    /// @brief max=0/min=0 indicates infinite zoom in/out
    void setScaleRange(double minimum = 0, double maximum = 0);

    void setScaleRange(ScaleRange range);

    /// Returns the current major grid size
    int minorGridSize() const;
    /// Returns the current major grid size
    int majorGridSize() const;

    /// returns the current scale
    double scale() const;
    /// returns the current graph scene
    GraphScene* nodeScene();

public slots:

    void centerScene();

    void scaleUp();

    void scaleDown();

    void setScale(double scale);

    void printToPDF();

signals:

    void scaleChanged(double scale, QPrivateSignal);

protected:

    void contextMenuEvent(QContextMenuEvent* event) override;

    void wheelEvent(QWheelEvent* event) override;

    void keyPressEvent(QKeyEvent* event) override;

    void keyReleaseEvent(QKeyEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;

    void mouseMoveEvent(QMouseEvent* event) override;

private:

    struct Impl;

    ScaleRange m_scaleRange;
    QPointF m_panPosition;
};

} // namespace intelli

#endif // GT_INTELLI_VIEW_H
