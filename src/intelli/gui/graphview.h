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
class GtObject;

namespace intelli
{

class Graph;
class GraphScene;
class GraphSceneSelector;

class GraphView : public QGraphicsView
{
    Q_OBJECT

public:

    struct ScaleRange
    {
        double minimum = 0;
        double maximum = 0;
    };
    
    GraphView(QWidget* parent = nullptr);
    ~GraphView();

    /**
     * @brief Sets the current scene
     * @param scene Scene
     */
    void setScene(GraphScene& scene);
    /**
     * @brief Clears the current scene
     */
    void clearScene();

    /// @brief max=0/min=0 indicates infinite zoom in/out
    void setScaleRange(double minimum = 0, double maximum = 0);

    void setScaleRange(ScaleRange range);

    /// Returns the current major grid size
    int minorGridSize() const;
    /// Returns the current major grid size
    int majorGridSize() const;

    /// Returns whether the grid is visible
    bool isGridVisible() const;
    /// Sets whether the grid should be visible or not
    void showGrid(bool show = true);

    /// Returns the current scale
    double scale() const;

    /// Returns the current graph scene
    GraphScene* nodeScene();

public slots:

    void centerScene();

    void scaleUp();

    void scaleDown();

    void setScale(double scale);

    void printToPDF();

signals:

    void scaleChanged(double scale, QPrivateSignal);

    void sceneChanged(GraphScene* scene);

    void gridVisibilityChanged();

protected:

    void contextMenuEvent(QContextMenuEvent* event) override;

    void wheelEvent(QWheelEvent* event) override;

    void keyPressEvent(QKeyEvent* event) override;

    void keyReleaseEvent(QKeyEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;

    void mouseMoveEvent(QMouseEvent* event) override;

    bool event(QEvent* event) override;

    bool gestureEvent(QGestureEvent* event);

    void panView(QPointF delta);

private:

    /// hide grid API as it provides a poor interface
    // using QGraphicsView::grid;

    struct Impl;

    /// scale range
    ScaleRange m_scaleRange;
    /// last pan position
    QPointF m_panPosition;
    /// accessing grid visibility through grid API is not possible
    bool m_gridVisible = true;
    bool m_handleMouseMovement = true;
};

} // namespace intelli

#endif // GT_INTELLI_VIEW_H
