/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_MODULE_H
#define GT_INTELLI_MODULE_H

#include <gt_moduleinterface.h>
#include <gt_datamodelinterface.h>
#include <gt_processinterface.h>
#include <gt_mdiinterface.h>
#include <gt_propertyinterface.h>

/**
 * @generated 1.2.0
 * @brief The GtIntelliGraphModule class
 */
class GtIntelliGraphModule : public QObject,
        public GtModuleInterface,
        public GtDatamodelInterface,
        public GtProcessInterface,
        public GtMdiInterface,
        public GtPropertyInterface
{
    Q_OBJECT

    GT_MODULE("module.json")

    Q_INTERFACES(GtModuleInterface)
    Q_INTERFACES(GtDatamodelInterface)
    Q_INTERFACES(GtProcessInterface)
    Q_INTERFACES(GtMdiInterface)
    Q_INTERFACES(GtPropertyInterface)
    
public:

    /**
     * @brief Returns current version number of module
     * @return version number
     */
    GtVersionNumber version() override;

    /**
     * @brief Returns module description
     * @return description
     */
    QString description() const override;

    /**
     * @brief Initializes module. Called on application startup.
     */
    void init() override;

    /**
     * @brief Passes additional module information to the framework.
     * @return module meta information.
     */
    MetaInformation metaInformation() const override;

    /**
     * @brief Upgrade routines
     * @return List of all upgrade routines of the module.
     */
    QList<gt::VersionUpgradeRoutine> upgradeRoutines() const override;

    /**
     * @brief Shared functions
     * @return List of all shared functions of the module.
     */
    QList<gt::SharedFunction> sharedFunctions() const override;

    /**
     * @brief Returns static meta objects of datamodel package.
     * @return package meta object
     */
    QMetaObject package() override;

    /**
     * @brief Returns static meta objects of datamodel classes.
     * @return list including meta objects
     */
    QList<QMetaObject> data() override;

    /**
     * @brief Returns true if module is a standalone module with own data
     * model structure. Otherwise module only extends the overall application
     * with additional functionalities like classes, calculators or 
     * graphical user interfaces.
     * @return Standalone indicator.
     */
    bool standAlone() override;

    /**
     * @brief Returns static meta objects of calculator classes.
     * @return list of meta objects.
     */
    QList<GtCalculatorData> calculators() override;

    /**
     * @brief Returns static meta objects of task classes.
     * @return list of meta objects.
     */
    QList<GtTaskData> tasks() override;

    /**
     * @brief Returns static meta objects of mdi item classes.
     * @return list including meta objects
     */
    QList<QMetaObject> mdiItems() override;

    /**
     * @brief Returns static meta objects of dockwidget classes.
     * @return list including meta objects
     */
    QList<QMetaObject> dockWidgets() override;

    /**
     * @brief uiItems
     * @return data class names mapped to ui item objects
     */
    QMap<const char*, QMetaObject> uiItems() override;

    /**
     * @brief postItems
     * @return
     */
    QList<QMetaObject> postItems() override;

    /**
     * @brief postPlots
     * @return
     */
    QList<QMetaObject> postPlots() override;

    /**
     * @brief Returns static meta objects of property item classes.
     * @return list of meta objects of property item classes.
     */
    QMap<const char*, QMetaObject> propertyItems() override;
};

#endif // GT_INTELLI_MODULE_H
