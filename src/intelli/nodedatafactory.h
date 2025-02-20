/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_DATAFACTORY_H
#define GT_INTELLI_DATAFACTORY_H

#include <intelli/exports.h>
#include <intelli/globals.h>

#include <gt_abstractobjectfactory.h>
#include <gt_object.h>

/// Helper macro for registering a node class. The node class does should not be
/// registered additionally as a "data" object of your module
#define GT_INTELLI_REGISTER_DATA(CLASS) \
    intelli::NodeDataFactory::registerData<CLASS>();

/// Helper macro to register a conversion between two types. `FUNC` parameter
/// takes the converted NodeDataPtr `FROM` as an argument and should return a
/// NodeDataPtr of type `TO`.
#define GT_INTELLI_REGISTER_CONVERSION(FROM, TO, FUNC) \
    intelli::NodeDataFactory::instance().registerConversion(GT_CLASSNAME(FROM), GT_CLASSNAME(TO), \
        [](intelli::NodeDataPtr const& data_) -> intelli::NodeDataPtr { \
            assert(data_);\
            return std::static_pointer_cast<TO const>(FUNC( \
                std::static_pointer_cast<FROM const>(data_))); \
        });

/// Helper macro to register a simple conversion between two types
#define GT_INTELLI_REGISTER_INLINE_CONVERSION(FROM, TO, HOW) \
    GT_INTELLI_REGISTER_CONVERSION(FROM, TO, [](auto const& data){ \
        return std::make_shared<TO const>(HOW); });

namespace intelli
{

/// Conversion function. The parameter is never null.
using ConversionFunction = std::function<NodeDataPtr(NodeDataPtr const&)>;

/// Struct to store a conversion between two types.
struct Conversion
{
    QString targetTypeId;
    ConversionFunction convert = nullptr;
};

class NodeData;
class GT_INTELLI_EXPORT NodeDataFactory : public GtAbstractObjectFactory
{

public:

    ~NodeDataFactory();

    /**
     * @brief instance
     * @return
     */
    static NodeDataFactory& instance();

    /**
     * @brief Registers the meta object in the data factory. This is necessary
     * to create a data type object dynamically or to retrieve the type id/
     * type name of the registered data types at runtime.
     * @param meta Meta object of the data type
     * @return success
     */
    bool registerData(QMetaObject const& meta) noexcept;

    /**
     * @brief Overload, convenience function. Registers the data type `T` in
     * the factory. `T` must be derived fo the common data type class.
     * @return success
     */
    template <typename T,
             std::enable_if_t<std::is_base_of<NodeData, T>::value, bool> = true>
    static bool registerData()
    {
        return instance().registerData(T::staticMetaObject);
    }

    /**
     * @brief Registers a conversion function between two type ids.
     * @param from Type Id of the source data type
     * @param to Type Id of the target data type.
     * @param conversion Conversion function
     * @return success
     */
    bool registerConversion(TypeId const& from,
                            TypeId const& to,
                            ConversionFunction conversion) noexcept;

    /**
     * @brief Returns a list of all registered type ids
     * @return List of registered type ids
     */
    TypeIdList registeredTypeIds() const { return knownClasses(); };

    /**
     * @brief Returns the type name of the type given by type id.
     * @param typeId Type id to retrieve the type name from
     * @return Type name. Empty if type id was not found
     */
    TypeName const& typeName(TypeId const& typeId) const noexcept;

    /**
     * @brief Returns whether a conversion function exists between two types.
     * Some conversions may only be allowed in one directional.
     * @param from Source type
     * @param to Target type
     * @return Whether there is an conversion function from type a to type b.
     */
    bool canConvert(TypeId const& from, TypeId const& to) const;

    /**
     * @brief Overload. Checks whether type `a` and `b` are compatible depending
     * on the direction.
     * @param a First type id
     * @param b Second type id
     * @param direction Direction of the final connection. In case of `IN`: from
     * b to a will be checked, else from a to b
     * @return success
     */
    bool canConvert(TypeId const& a, TypeId const& b, PortType direction) const;

    /**
     * @brief Performs a conversion between of the given data type instance to
     * the specified target data type. It is not required to check beforehand
     * whether a conversion exists.
     * @param data Data to convert
     * @param to Target type id
     * @return Shared pointer to the converted data type. Will be null if data
     * is null, no conversion exists or the conversion fails.
     */
    NodeDataPtr convert(NodeDataPtr const& data, TypeId const& to) const;

    /**
     * @brief Instantiates a new node of type className.
     * @param className Class to instantiate
     * @return Object pointer (may be null)
     */
    NodeDataPtr makeData(TypeId const& typeId) const noexcept;

private:

    struct Impl;
    std::unique_ptr<Impl> pimpl;

    // hide some functions
    using GtAbstractObjectFactory::newObject;
    using GtAbstractObjectFactory::registerClass;

    /// private constructor
    NodeDataFactory();
};

} // namespace intelli

#endif // GT_INTELLI_DATAFACTORY_H
