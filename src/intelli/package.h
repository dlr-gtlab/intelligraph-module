/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_PACKAGE_H
#define GT_INTELLI_PACKAGE_H

#include <intelli/exports.h>

#include <gt_package.h>
#include <gt_version.h>

#include <QDir>

/// Helper macro do add "override" for GTlab 2.1.x
#if GT_VERSION >= GT_VERSION_CHECK(2, 1, 0)
    #define ADD_OVERRIDE_2_1_X override
#else
    #define ADD_OVERRIDE_2_1_X
#endif

namespace intelli
{


/**
 * @generated 1.2.0
 * @brief The GtIgPackage class
 */
class Package : public GtPackage
{
    Q_OBJECT

public:

    static QString const& MODULE_DIR;
    static QString const& FILE_SUFFIX;
    static QString const& INDEX_FILE;

    Q_INVOKABLE Package();

protected:

    /**
     * @brief Module specific data read method.
     * @param Root data.
     * @return Returns true if data was successfully read.
     */
    bool readData(const QDomElement& root) override;

    /**
     * @brief Module specific data save method.
     * @param Root data.
     * @return Returns true if data was successfully read.
     */
    bool saveData(QDomElement& root, QDomDocument& doc) override;

    /**
     * @brief Reads the intelli graphs and their categories from the project dir
     * @param projectDir Directory to read data from
     * @return success
     */
    bool readMiscData(QDir const& projectDir) ADD_OVERRIDE_2_1_X;

    /**
     * @brief Saves the intelli graphs and their categories to separate files
     * @param projectDir Directory to save data to
     * @return success
     */
    bool saveMiscData(QDir const& projectDir) ADD_OVERRIDE_2_1_X;

private:

    struct Impl;
};

} // namespace intelli

#endif // GT_INTELLI_PACKAGE_H
