
#include <intelli/gui/graphviewoverlay.h>

#include <intelli/graph.h>
#include <intelli/graphexecmodel.h>
#include <intelli/gui/graphview.h>
#include <intelli/gui/graphscene.h>
#include <intelli/gui/graphsceneselector.h>

#include <gt_application.h>
#include <gt_objectuiaction.h>
#include <gt_icons.h>
#include <gt_colors.h>
#include <gt_guiutilities.h>
#include <gt_grid.h>
#include <gt_state.h>
#include <gt_statehandler.h>

#include <QMenu>
#include <QMenuBar>
#include <QPushButton>

Q_DECLARE_METATYPE(intelli::ConnectionShape)

using namespace intelli;

struct GraphViewOverlay::Impl
{

struct GridStateChanged
{
    void operator()(QVariant const& enable)
    {
        assert(view);
        assert(overlay);
        assert(overlay->m_snapToGridBtn);

        auto* btn = overlay->m_snapToGridBtn;
        auto* grid = view->grid();
        auto* scene = view->nodeScene();
        if (!grid)
        {
            btn->setVisible(false);
            if (scene) scene->setSnapToGrid(false);
            return;
        }

        bool enabled = enable.toBool();

        view->resetCachedContent();
        grid->showGrid(enabled);
        btn->setVisible(enabled);
        if (scene) scene->setSnapToGrid(enabled && btn->isChecked());
    }

    GraphViewOverlay* overlay{};
    GraphView* view{};
};

struct ToggleStateValue
{
    void operator()()
    {
        assert(state);
        // triggers state update
        state->setValue(!state->getValue().toBool());
    }

    GtState* state{};
};

struct SnapToGridStateChanged
{
    void operator()(QVariant const& enable)
    {
        assert(view);
        assert(overlay);
        assert(overlay->m_snapToGridBtn);

        bool enabled = enable.toBool();

        auto* btn = overlay->m_snapToGridBtn;
        btn->setChecked(enabled);

        auto* scene = view->nodeScene();
        if (scene) scene->setSnapToGrid(enabled);
    }

    GraphViewOverlay* overlay{};
    GraphView* view{};
};

struct ConnectionShapeStateChanged
{
    void operator()(QVariant const& shape)
    {
        assert(view);
        assert(overlay);

        auto scene = view->nodeScene();
        if (scene) scene->setConnectionShape(shape.value<ConnectionShape>());
    }

    GraphViewOverlay* overlay{};
    GraphView* view{};
};

struct ConnectionShapeValueChanged
{
    void operator()()
    {
        assert(conShapeState);
        auto value = conShapeState->getValue().value<ConnectionShape>();
        switch (value)
        {
        case ConnectionShape::Cubic:
            value = ConnectionShape::Rectangle;
            break;
        case ConnectionShape::Rectangle:
            value = ConnectionShape::Straight;
            break;
        case ConnectionShape::Straight:
            value = ConnectionShape::Cubic;
        }
        conShapeState->setValue(QVariant::fromValue(value));
    }

    GtState* conShapeState{};
};

struct AutoEvaluationStateChanged
{
    void operator()(QVariant const& enable)
    {
        assert(view);
        assert(overlay);

        auto scene = view->nodeScene();
        if (!scene) return;

        auto& graph = scene->graph();

        auto* model = GraphExecutionModel::accessExecModel(graph);
        if (!model) return;

        bool doAutoEvaluate = enable.toBool();
        if (doAutoEvaluate)
        {
            model->autoEvaluateGraph(graph);
        }
        else
        {
            model->stopAutoEvaluatingGraph(graph);
        }

        overlay->m_startAutoEvalBtn->setVisible(!doAutoEvaluate);
        overlay->m_stopAutoEvalBtn->setVisible(doAutoEvaluate);
    }

    GraphViewOverlay* overlay{};
    GraphView* view{};
};

/// helper function to create a state and update it accordingly when signals are
/// fired.
template<typename OnStateChanged,
         typename OnValueChanged,
         typename Value,
         typename Signal,
         typename Sender = GraphView>
static GtState* setupState(GraphViewOverlay& overlay,
                           GtObject& guardian,
                           Graph& graph,
                           QString const& stateId,
                           Value defaultValue,
                           Sender* sender,
                           Signal signal)
{
    /// grid change state
    auto* state = gtStateHandler->initializeState(
        // group id
        GT_CLASSNAME(GraphView),
        // state id
        stateId,
        // entry for this graph
        graph.uuid() + QChar(';') + stateId.toLower().replace(' ', '_'),
        // default value
        defaultValue,
        // guardian object
        &guardian);

    connect(state, qOverload<QVariant const&>(&GtState::valueChanged),
            &overlay, OnStateChanged{&overlay, overlay.m_view});
    connect(sender, signal,
            state, OnValueChanged{state});

    // trigger grid update
    emit state->valueChanged(state->getValue());
    return state;
}

};

GraphViewOverlay::GraphViewOverlay(GraphView& view) :
    QHBoxLayout(&view)
{
    m_menuBar = new QMenuBar;

    auto const makeSeparator = [](){
        return GtObjectUIAction();
    };

    // helper function to call member function of current scene on activation
    auto sceneAction = [this](auto mfunc){
        return [this, binding = std::bind(mfunc, std::placeholders::_1)](GtObject*){
            assert(m_view);
            auto* scene = m_view->nodeScene();
            if (scene) binding(scene);
        };
    };

    auto setupBtn = [](){
        auto* btn = new QPushButton();
        btn->setVisible(false);
        btn->setEnabled(false);
        auto height = btn->sizeHint().height();
        btn->setFixedSize(QSize(height, height));
        return btn;
    };

    m_startAutoEvalBtn = setupBtn();
    m_stopAutoEvalBtn = setupBtn();
    m_snapToGridBtn = setupBtn();

    /* SCENE MENU */
    m_sceneMenu = m_menuBar->addMenu(tr("Scene"));
    m_sceneMenu->setEnabled(false);

    auto resetScaleAction =
        gt::gui::makeAction(tr("Reset scale"),
                            std::bind(&GraphView::setScale, &view, 1))
            .setIcon(gt::gui::icon::revert());

    auto centerSceneAction =
        gt::gui::makeAction(tr("Center scene"),
                            std::bind(&GraphView::centerScene, &view))
            .setIcon(gt::gui::icon::select());

    auto changeGrid =
        gt::gui::makeAction(tr("Toggle Grid"),
                            std::bind(&GraphViewOverlay::gridChanged, this, QPrivateSignal()))
            .setIcon(gt::gui::icon::grid());

    auto changeConShape =
        gt::gui::makeAction(tr("Toggle Connection Shape"),
                            std::bind(&GraphViewOverlay::connectionShapeChanged,this, QPrivateSignal()))
            .setIcon(gt::gui::icon::vectorBezier2());

    auto print =
        gt::gui::makeAction(tr("Print to PDF"),
                            std::bind(&GraphView::printToPDF, &view))
            .setIcon(gt::gui::icon::pdf());

    gt::gui::addToMenu(resetScaleAction, *m_sceneMenu, nullptr);
    gt::gui::addToMenu(centerSceneAction, *m_sceneMenu, nullptr);
    gt::gui::addToMenu(changeConShape, *m_sceneMenu, nullptr);
    gt::gui::addToMenu(changeGrid, *m_sceneMenu, nullptr);
    gt::gui::addToMenu(makeSeparator(), *m_sceneMenu, nullptr);
    gt::gui::addToMenu(print, *m_sceneMenu, nullptr);

    /* EDIT MENU */
    m_editMenu = m_menuBar->addMenu(tr("Edit"));
    m_editMenu->setEnabled(false);

    auto alignAction =
        gt::gui::makeAction(tr("Align Selection to Grid"),
                            sceneAction(&GraphScene::alignObjectsToGrid))
            .setIcon(gt::gui::icon::gridSnap())
            .setVerificationMethod([this](GtObject*){
                // grid->isVisible or similar does not exist
                return m_snapToGridBtn->isEnabled();
            });

    auto copyAction =
        gt::gui::makeAction(tr("Copy Selection"),
                            sceneAction(&GraphScene::copySelectedObjects))
            .setIcon(gt::gui::icon::copy())
            .setShortCut(gtApp->getShortCutSequence("copy"));

    auto pasteAction =
        gt::gui::makeAction(tr("Paste Selection"),
                            sceneAction(&GraphScene::pasteObjects))
            .setIcon(gt::gui::icon::paste())
            .setShortCut(gtApp->getShortCutSequence("paste"));

    auto duplicateAction =
        gt::gui::makeAction(tr("Duplicate Selection"),
                            sceneAction(&GraphScene::duplicateSelectedObjects))
            .setIcon(gt::gui::icon::duplicate())
            .setShortCut(QKeySequence(Qt::CTRL | Qt::Key_D));

    auto deleteAction =
        gt::gui::makeAction(tr("Delete Selection"),
                            sceneAction(&GraphScene::deleteSelectedObjects))
            .setIcon(gt::gui::icon::duplicate())
            .setShortCut(gtApp->getShortCutSequence("delete"));

    auto clearSelectionAction =
        gt::gui::makeAction(tr("Clear Selection"),
                            sceneAction(&GraphScene::deleteSelectedObjects))
            .setIcon(gt::gui::icon::duplicate())
            .setShortCut(Qt::Key_Escape);

    gt::gui::addToMenu(alignAction, *m_editMenu, nullptr);
    gt::gui::addToMenu(clearSelectionAction, *m_editMenu, nullptr);
    gt::gui::addToMenu(makeSeparator(), *m_sceneMenu, nullptr);
    gt::gui::addToMenu(copyAction, *m_editMenu, nullptr);
    gt::gui::addToMenu(duplicateAction, *m_editMenu, nullptr);
    gt::gui::addToMenu(pasteAction, *m_editMenu, nullptr);

    /* AUTO EVAL */
    m_startAutoEvalBtn->setVisible(true);
    m_startAutoEvalBtn->setToolTip(tr("Enable automatic graph evaluation"));
    m_startAutoEvalBtn->setIcon(gt::gui::icon::play());

    m_stopAutoEvalBtn->setToolTip(tr("Stop automatic graph evaluation"));
    m_stopAutoEvalBtn->setIcon(gt::gui::icon::stop());

    connect(m_startAutoEvalBtn, &QPushButton::clicked,
            this, std::bind(&GraphViewOverlay::autoEvaluationChanged, this, QPrivateSignal()));
    connect(m_stopAutoEvalBtn, &QPushButton::clicked,
            this, std::bind(&GraphViewOverlay::autoEvaluationChanged, this, QPrivateSignal()));

    /* SNAP TO GRID */
    m_snapToGridBtn->setCheckable(true);
    m_snapToGridBtn->setToolTip(tr("Toggle snap to grid"));
    m_snapToGridBtn->setVisible(true);
    m_snapToGridBtn->setEnabled(false);

    {
        using gt::gui::color::lighten;
        using gt::gui::color::disabled;
        using gt::gui::color::text;
        using gt::gui::colorize; // use custom colors for icon

        auto* button = m_snapToGridBtn;
        // checked button do not use On/Off Icons, thus we have to update the
        // icon ourselfes (adapted from `GtOutputDock`)
        auto const updateIconColor = [b = QPointer<QPushButton>(button)](){
            assert (b);
            return b->isChecked() ? text() : lighten(disabled(), 15);
        };
        button->setIcon(colorize(gt::gui::icon::gridSnap(),
                                 gt::gui::SvgColorData{ updateIconColor }));
    }

    connect(m_snapToGridBtn, &QPushButton::clicked, this, [this](){
        if (auto* scene = m_view->nodeScene())
        {
            scene->setSnapToGrid(m_snapToGridBtn->isChecked());
        }
    });

    /* SCENE-LINK */
    m_sceneSelector = new GraphSceneSelector;

    /* OVERLAY */
    setContentsMargins(5, 5, 5, 0);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    addWidget(m_menuBar);
    addWidget(m_startAutoEvalBtn);
    addWidget(m_stopAutoEvalBtn);
    addWidget(m_snapToGridBtn);
    addWidget(m_sceneSelector);
    addStretch();

    auto size = m_menuBar->sizeHint();
    size.setWidth(size.width() + 10);
    m_menuBar->setFixedSize(size);

    /* VIEW ACTIONS */
    auto actions = m_editMenu->actions();
    std::for_each(actions.begin(), actions.end(), [](QAction* action){
        action->setShortcutContext(Qt::ShortcutContext::WidgetShortcut);
    });
    view.addActions(std::move(actions));
}

void
GraphViewOverlay::onSceneRegistered(GraphScene& scene)
{
    auto& graph = scene.graph();

    /// accessing grid size thorugh grid API not possible
    scene.setGridSize(m_view->minorGridSize());

    m_sceneSelector->setCurrentGraph(graph);

    m_startAutoEvalBtn->setEnabled(true);
    m_stopAutoEvalBtn->setEnabled(true);
    m_snapToGridBtn->setEnabled(true);

    /* SETUP STATES */
    auto* guardian = new GtObject();
    guardian->setParent(&scene);

    /// grid change state
    auto* gridState =
        Impl::setupState<Impl::GridStateChanged,
                         Impl::ToggleStateValue>(
        *this, *guardian, graph, tr("Show Grid"), true,
        this, &GraphViewOverlay::gridChanged
    );

    /// snap to grid state
    Impl::setupState<Impl::SnapToGridStateChanged,
                     Impl::ToggleStateValue>(
        *this, *guardian, graph, tr("Snap to Grid"), true,
        m_snapToGridBtn, &QPushButton::clicked
    );

    /// connection style state
    Impl::setupState<Impl::ConnectionShapeStateChanged,
                     Impl::ConnectionShapeValueChanged>(
        *this, *guardian, graph, tr("Connection Shape"),
        QVariant::fromValue(ConnectionShape::DefaultShape),
        this, &GraphViewOverlay::connectionShapeChanged
    );

    /// auto evaluate state
    Impl::setupState<Impl::AutoEvaluationStateChanged,
                     Impl::ToggleStateValue>(
        *this, *guardian, graph, tr("Auto Evaluation"),
        QVariant::fromValue(false),
        this, &GraphViewOverlay::autoEvaluationChanged
    );

    bool gridEnabled = gridState->getValue().toBool();
    if (!gridEnabled) scene.setSnapToGrid(false);
}


GraphViewOverlay*
intelli::makeOverlay(GraphView& view)
{
    return new GraphViewOverlay(view);
}
