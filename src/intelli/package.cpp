/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

/*
 * generated 1.2.0
 */
 
#include "intelli/package.h"
#include "intelli/graph.h"
#include "intelli/graphcategory.h"

#include <gt_objectmemento.h>
#include <gt_objectfactory.h>
#include <gt_coreapplication.h>
#include <gt_project.h>
#include <gt_qtutilities.h>

#include <gt_logging.h>

#include <QDir>
#include <QDirIterator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

using namespace intelli;

QString const& Package::MODULE_DIR = QStringLiteral("intelli");

QString const& Package::FILE_SUFFIX = QStringLiteral(".gtflow");

QString const& Package::INDEX_FILE = QStringLiteral(".gtflow.index");

struct Package::Impl
{
    /**
     * @brief Deletes all graph file of the category dir `dir`
     * @param dir Category dir
     */
    static void deleteGraphFiles(QDir& dir)
    {
        QDirIterator fileIter{
            dir.path(),
            QStringList{'*' + FILE_SUFFIX},
            QDir::Files,
            QDirIterator::NoIteratorFlags
        };

        while (fileIter.hasNext())
        {
            dir.remove(fileIter.next());
        }
    }

    /**
     * @brief Deletes all category dirs in `dir` that are not in `whitelist`
     * @param dir Module dir
     */
    static void deleteCategoryDirs(QDir& dir, QStringList const& whitelist)
    {
        QDirIterator catDirIter{
            dir.path(),
            QStringList{QStringLiteral("*")},
            QDir::Dirs | QDir::NoDotAndDotDot,
            QDirIterator::NoIteratorFlags
        };

        while (catDirIter.hasNext())
        {
            QDir catDir = catDirIter.next();

            QString const& catName = catDirIter.fileName();

            if (whitelist.contains(catName)) continue;

            catDir.remove(INDEX_FILE);

            Impl::deleteGraphFiles(catDir);

            if (!dir.rmdir(catName))
            {
                gtWarning() << tr("Failed to remove category '%1' "
                                  "(directory is not empty)!").arg(catName);
            }
        }
    }

    /**
     * @brief Creates an index file in `dir` with the contents of `jDoc`. The
     * index file denotes the order of the objects in `dir`.
     * @param jDoc Document to write
     * @param dir Dir to create index file in
     * @param makeError
     * @return success
     */
    template <typename Lambda>
    static bool createIndexFile(GtObject const& source, QJsonDocument const& jDoc, QDir const& dir, Lambda const& makeError)
    {
        QFile indexFile(dir.absoluteFilePath(INDEX_FILE));

        if (!indexFile.open(QIODevice::Truncate | QIODevice::WriteOnly))
        {
            gtWarning() << makeError()
                        << tr("Failed to create index file for '%1'. Continuing...").arg(source.objectName());
            return false;
        }

        if (!indexFile.write(jDoc.toJson(QJsonDocument::Indented)))
        {
            gtWarning() << makeError()
                        << tr("Failed to write index file for '%1'. Continuing...").arg(source.objectName());
            return false;
        }

        return true;
    }

    /**
     * @brief Attempts to read the index file located in `dir`
     * @param dir Directory in which the index file exists
     * @return Json document. Null if reading failed
     */
    static QJsonDocument readIndexFile(QDir const& dir)
    {
        QFile indexFile(dir.absoluteFilePath(INDEX_FILE));

        if (!indexFile.exists() || !indexFile.open(QIODevice::ReadOnly))
        {
            return {};
        }

        return QJsonDocument::fromJson(indexFile.readAll());
    }

    /**
     * @brief Applies the order in `jIndex["order"]` to the direct children of
     * `obj`
     * @param obj Object to order children of
     * @param jIndex Json object that contains the `order` object
     */
    static void applyIndex(GtObject& obj, QJsonObject jIndex)
    {
        auto const& order = jIndex[QStringLiteral("order")].toArray();
        auto const& children = obj.findDirectChildren();

        int idx = 0;

        for (auto const& jObj : order)
        {
            auto iter = std::find_if(children.begin(), children.end(),
                                     [uuid = jObj.toString()](GtObject* obj){
                return obj->uuid() == uuid;
            });
            if (iter == children.end()) continue;

            auto onFailure = gt::finally([iter](){
                delete *iter;
            });

            (*iter)->disconnectFromParent();
            if (obj.insertChild(idx++, *iter)) onFailure.clear();
        }
    }

    /**
     * @brief Saves `graph` to `dir`.
     * @param graph Graph object to save
     * @param dir Dir to save object in
     * @param makeError
     * @return success
     */
    template <typename Lambda>
    static bool saveGraph(Graph const* graph, QDir dir, Lambda const& makeError)
    {
        assert(graph);

        auto const& fileName = graph->uuid() + FILE_SUFFIX;

        QFile file(dir.absoluteFilePath(fileName));

        if (!file.open(QIODevice::Truncate | QIODevice::WriteOnly))
        {
            gtError() << makeError()
                      << tr("(graph flow '%1' (%2) could not be created!)")
                             .arg(fileName, graph->caption());
            return false;
        }

        auto data = graph->toMemento().toByteArray();

        if (file.write(data) != data.size())
        {
            gtError() << makeError()
                      << tr("(graph flow '%1' (%2) could not be saved!)")
                             .arg(fileName, graph->caption());
            return false;
        }

        return true;
    }

    /**
     * @brief Saves the category object `cat` to `dir`.
     * @param cat Category object to save
     * @param dir Dir to save object in
     * @param makeError
     * @return success
     */
    template <typename Lambda>
    static bool saveCategory(GraphCategory const* cat, QDir dir, Lambda const& makeError)
    {
        assert(cat);

        auto const& name = cat->objectName();

        dir.mkdir(name);
        if (!dir.cd(name))
        {
            gtError() << makeError()
                      << tr("(category dir '%1' could not be created)").arg(name);
            return false;
        }

        deleteGraphFiles(dir);

        // order of graphs
        QJsonArray jGraphs;

        auto const& graphs = cat->findDirectChildren<Graph*>();

        bool success = true;

        for (auto const* graph : graphs)
        {
            if (!Impl::saveGraph(graph, dir, makeError))
            {
                success = false;
                continue;
            }

            jGraphs.append(graph->uuid());
        }

        // contents of index file
        QJsonObject jIndex;
        jIndex[QStringLiteral("uuid")] = cat->uuid();
        jIndex[QStringLiteral("order")] = std::move(jGraphs);

        Impl::createIndexFile(*cat, QJsonDocument(jIndex), dir, makeError);

        return success;
    }

    /**
     * @brief Attempts to read the graph denoted by `fileName` relative to `dir`.
     * @param cat Category to append graph to
     * @param fileName File to open
     * @param dir Directory to open file in
     * @param makeError
     * @return success
     */
    template <typename Lambda>
    static bool readGraph(GraphCategory& cat, QString const& fileName, QDir const& dir, Lambda const& makeError)
    {
        assert(gtObjectFactory);

        QFile file(dir.absoluteFilePath(fileName));

        if (!file.open(QIODevice::ReadOnly))
        {
            gtError() << makeError()
                      << tr("(graph flow '%1' could not be opend!)")
                             .arg(fileName);
            return false;
        }

        GtObjectMemento memento(file.readAll());

        auto graph = gt::unique_qobject_cast<Graph>(memento.toObject(*gtObjectFactory));

        if (!graph || !cat.appendChild(graph.get()))
        {
            gtError() << makeError()
                      << tr("(graph flow '%1' could not be opend!)")
                             .arg(fileName);
            return false;
        }

        graph.release();

        return true;
    }

    /**
     * @brief Attempts to read the category denoted by `name` relative to `dir`.
     * @param package Package to append category to
     * @param name Category name to append
     * @param dir Directory to read category from
     * @param makeError
     * @return success
     */
    template <typename Lambda>
    static bool readCategory(GtPackage& package, QString const& name, QDir dir, Lambda const& makeError)
    {
        if (!dir.cd(name))
        {
            gtError() << makeError()
                      << tr("(category dir '%1' could not be accessed)").arg(name);
            return false;
        }

        auto cat = std::make_unique<GraphCategory>();
        cat->setObjectName(name);

        if (!package.appendChild(cat.get()))
        {
            gtError() << makeError()
                      << tr("(category '%1' could not be loaded)").arg(name);
            return false;
        }

        QDirIterator fileIter{
            dir.path(),
            QStringList{'*' + FILE_SUFFIX},
            QDir::Files,
            QDirIterator::NoIteratorFlags
        };

        bool success = true;

        while (fileIter.hasNext())
        {
            if (!Impl::readGraph(*cat, fileIter.next(), dir, makeError))
            {
                success = false;
            }
        }

        auto cleanup = gt::finally([&cat](){
            cat.release();
        });

        // parse index file
        QJsonDocument jDoc = Impl::readIndexFile(dir);
        if (jDoc.isNull() || !jDoc.isObject())
        {
            gtWarning() << makeError()
                        << tr("Index file of '%1' could not be parsed. Continuing...").arg(name);
            return success;
        }

        // apply uuid
        QJsonObject jIndex = jDoc.object();
        auto uuid = jIndex[QStringLiteral("uuid")].toString();

        if (uuid.size() == cat->uuid().size()) cat->setUuid(uuid);

        // apply order
        applyIndex(*cat, jIndex);

        return success;
    }
};

Package::Package()
{
    setObjectName("IntelliGraphs");
}

bool
Package::readData(const QDomElement& root)
{
#if GT_VERSION < GT_VERSION_CHECK(2, 1, 0)
    assert(gtApp);

    // Workaround. GTlab < 2.1.0 does not allow to access the project dir,
    // hence we need to get it this way.
    auto* project = gtApp->currentProject();

    if (!project)
    {
        gtError() << tr("Failed to read package data!")
                  << tr("(project could not be accessed)");
        return GtPackage::readData(root);
    }

    if (!readMiscData(project->path()))
    {
        gtError() << tr("The GTlab version is most likely incompatible with the module version of '%1'! "
                        "Reading from module file instead...").arg(MODULE_DIR);
        return GtPackage::readData(root);
    }
#endif
    return true;
}

bool
Package::saveData(QDomElement& root, QDomDocument& doc)
{
#if GT_VERSION < GT_VERSION_CHECK(2, 1, 0)
    assert(gtApp);

    // Workaround. GTlab < 2.1.0 does not allow to access the project dir,
    // hence we need to get it this way.
    auto* project = gtApp->currentProject();

    if (!project)
    {
        gtError() << tr("Failed to save package data!")
                  << tr("(project could not be accessed)");
        GtPackage::saveData(root, doc);
        return false;
    }

    if (!saveMiscData(project->path()))
    {
        GtPackage::saveData(root, doc);
        return false;
    }

#endif
    return true;
}

bool
Package::saveMiscData(QDir const& projectDir)
{
    auto const makeError = [](){
        return tr("Failed to save package data");
    };

    QDir dir(projectDir);
    dir.mkdir(MODULE_DIR);
    if (!dir.cd(MODULE_DIR))
    {
        gtError() << makeError()
                  << tr("(module dir '%1' could not be created)").arg(MODULE_DIR);
        return false;
    }

    auto const& categories = findDirectChildren<GraphCategory*>();

    Impl::deleteCategoryDirs(dir, gt::objectNames(categories));

    // order of categories
    QJsonArray jCats;

    bool success = true;

    for (auto const* cat : categories)
    {
        if (!Impl::saveCategory(cat, dir, makeError))
        {
            success = false;
            continue;
        }

        jCats.append(cat->uuid());
    }

    // contents of index file
    QJsonObject jIndex;
    jIndex[QStringLiteral("order")] = std::move(jCats);

    Impl::createIndexFile(*this, QJsonDocument(jIndex), dir, makeError);

    return success;
}

bool
Package::readMiscData(QDir const& projectDir)
{
    auto const makeError = [](){
        return tr("Failed to read package data");
    };

    QDir dir(projectDir);

    if (!dir.cd(MODULE_DIR) || !dir.exists())
    {
        gtWarning().verbose()
            << tr("Module dir '%1' does not exists.").arg(MODULE_DIR);
        return true;
    }

    if (dir.isEmpty(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        gtWarning().verbose()
            << tr("Empty module '%1' dir").arg(MODULE_DIR);
        return true;
    }

    QDirIterator iter{
        dir.path(),
        QStringList{QStringLiteral("*")},
        QDir::Dirs | QDir::NoDotAndDotDot,
        QDirIterator::NoIteratorFlags
    };

    bool success = true;

    while (iter.hasNext())
    {
        iter.next();

        if (!Impl::readCategory(*this, iter.fileName(), dir, makeError))
        {
            success = false;
        }
    }

    // parse index file
    QJsonDocument jDoc = Impl::readIndexFile(dir);
    if (jDoc.isNull() || !jDoc.isObject())
    {
        gtWarning() << makeError()
                    << tr("Package index file could not be parsed. Continuing...");
        return success;
    }

    // apply
    Impl::applyIndex(*this, jDoc.object());

    return success;
}
