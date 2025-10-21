/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/graphuservariables.h>
#include <intelli/private/utils.h>
#include <intelli/property/uint.h>

#include <gt_coreapplication.h>
#include <gt_propertystructcontainer.h>
#include <gt_structproperty.h>
#include <gt_variantproperty.h>
#include <gt_variantconvert.h>
#include <gt_mpl.h>

#include <QVariant>

using namespace intelli;

struct GraphUserVariables::Impl
{
    GtPropertyStructContainer variables{
        "userVars", tr("User Variables")
    };
};

QString const& S_TYPE   = QStringLiteral("Entry");
QString const& S_MEMBER_VALUE = QStringLiteral("value");
QString const& S_MEMBER_ID = QStringLiteral("id");

GraphUserVariables::GraphUserVariables(GtObject* parent) :
    GtObject(parent),
    pimpl(std::make_unique<Impl>())
{
    setObjectName(QStringLiteral("__user_variables"));

    setFlag(UserRenamable, false);
    setFlag(UserDeletable, false);
    setFlag(UserHidden, true);

    GtPropertyStructDefinition def{S_TYPE};
    // value of key
    def.defineMember(S_MEMBER_VALUE, gt::makeVariantProperty());
    // for some applications a numerical "unique" id is needed
    def.defineMember(S_MEMBER_ID, intelli::makeUIntProperty(0));

    pimpl->variables.registerAllowedType(def);
    registerPropertyStructContainer(pimpl->variables);
}

GraphUserVariables::~GraphUserVariables() = default;

namespace
{

/// Operator that returns template argument as is
struct identity
{
    template <typename T>
    T&& operator()(T&& t) const { return t; }
};

/// Helper function that checks if variant `v` is compatible to any `T`.
/// Prints error message if not.
template<typename ...T>
bool checkCompatibility(QVariant const& v)
{
    std::initializer_list<bool> convertible{gt::canConvert<T>(v)...};

    if (std::none_of(convertible.begin(), convertible.end(), identity{}))
    {
        gt::log::Stream s;
        s << QObject::tr("GraphUserVariables: "
                         "Unsupported variant type, must be one of: '");

        // iterate over types and print names
        std::tuple<T...> tuple{};
        gt::mpl::static_foreach(tuple, [&s](auto type){
            s << gt::log::nospace
              << '"' << gt::mpl::type_name<decltype(type)>::get() << "\" ";
        });
        s << gt::log::space << "',"
          << QObject::tr("got: '%1'").arg(toString(v));

        gtWarning() << s.str();
        return false;
    }

    return true;
}

} // namespace

bool
GraphUserVariables::setValue(QString const& key, QVariant const& value)
{
    if (value.isNull() ||
        !checkCompatibility<
            bool,
            int,
            unsigned,
            double,
            QString
            >(value))
    {
        return false;
    }

    GtPropertyStructInstance* instance = nullptr;

    auto iter = pimpl->variables.findEntry(key);
    if (iter != pimpl->variables.end())
    {
        // overwrite value
        instance = &*iter;
    }
    else
    {
        // add new value
        instance = &pimpl->variables.newEntry(S_TYPE, key);
    }

    // set value
    instance->setMemberVal(S_MEMBER_VALUE, value);

    // update id (if its invalid)
    bool ok = true;
    if (instance->getMemberVal<ID>(S_MEMBER_ID, &ok) == 0 && !ok)
    {
        unsigned newId = pimpl->variables.size();
        for (auto& entry : pimpl->variables)
        {
            bool ok = true;
            auto id = entry.getMemberVal<ID>(S_MEMBER_ID, &ok);
            if (!ok) id = 0;

            newId = std::max(newId, id + 1);
        }

        instance->setMemberVal(S_MEMBER_ID, qulonglong{newId});
    }

    return true;
}

bool
GraphUserVariables::remove(QString const& key)
{
    auto iter = pimpl->variables.findEntry(key);
    if (iter == pimpl->variables.end()) return false;
    pimpl->variables.removeEntry(iter);
    return true;
}

void
GraphUserVariables::clear()
{
    pimpl->variables.clear();
}

bool
GraphUserVariables::hasValue(QString const& key) const
{
    return pimpl->variables.findEntry(key) != pimpl->variables.end();
}

QVariant
GraphUserVariables::value(QString const& key) const
{
    auto iter = pimpl->variables.findEntry(key);
    if (iter == pimpl->variables.end()) return {};

    return iter->getMemberValToVariant(S_MEMBER_VALUE);
}

GraphUserVariables::ID
GraphUserVariables::id(QString const& key) const
{
    auto iter = pimpl->variables.findEntry(key);
    if (iter == pimpl->variables.end()) return {};

    bool ok = true;
    ID value = iter->getMemberVal<ID>(S_MEMBER_ID, &ok);

    return ok ? value : 0;
}

QStringList
GraphUserVariables::keys() const
{
    QStringList list;
    list.reserve(size());
    visit([&](QString const& key, QVariant const&){
        list.push_back(key);
    });
    return list;
}

size_t
GraphUserVariables::size() const
{
    return pimpl->variables.size();
}

bool
GraphUserVariables::empty() const
{
    return size() == 0;
}

void
GraphUserVariables::visit(std::function<void (const QString&, const QVariant&)> f) const
{
    for (GtPropertyStructInstance& entry : pimpl->variables)
    {
        f(entry.ident(), entry.getMemberValToVariant(S_MEMBER_VALUE));
    }
}

void
GraphUserVariables::mergeWith(GraphUserVariables& other)
{
    if (empty()) return;

    other.visit([this](QString const& key, QVariant const& value){
        if (hasValue(key))
        {
            gtWarning().verbose()
                << QObject::tr("GraphUserVariables: "
                               "Entry '%1' already exists, overwriting!").arg(key)
                << gt::log::nospace
                << "(current: " << this->value(key)
                << " vs. new "  << value << ")";
        }
        setValue(key, value);
    });

    other.clear();
}

void
GraphUserVariables::onObjectDataMerged()
{
    GtObject::onObjectDataMerged();

    emit variablesUpdated();
}
