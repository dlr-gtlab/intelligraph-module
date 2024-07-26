/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_VIEW_H
#define GT_INTELLI_VIEW_H

#include <intelli/exports.h>

#include <gt_graphicsview.h>

class QMenu;
class QPushButton;
class GtGrid;

namespace intelli
{

class GraphScene;

class GT_INTELLI_EXPORT GraphView : public GtGraphicsView
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

    double scale() const;
    
    GraphScene* nodeScene();

public slots:

    void centerScene();

    void scaleUp();

    void scaleDown();

    void setScale(double scale);

private slots:

    void printPDF();

signals:

    void scaleChanged(double scale);

    void gridChanged(QPrivateSignal);

    void connectionShapeChanged(QPrivateSignal);

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

    QMenu* m_sceneMenu = nullptr;
    QMenu* m_editMenu = nullptr;

    QPushButton* m_startAutoEvalBtn = nullptr;
    QPushButton* m_stopAutoEvalBtn = nullptr;
    QPushButton* m_snapToGridBtn = nullptr;
};

} // namespace intelli

#endif // GT_INTELLI_VIEW_H
