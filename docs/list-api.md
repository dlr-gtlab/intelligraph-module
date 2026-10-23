# List API Documentation

Every registered node data type automatically gets an associated **list type**.
This list type may either be generic or a specialized list type that stores the underlying data more optimally.

## Generic List Type

The generic list node data type `intelli::GernericListData` simply stores a list of `intelli::NodeDataPtr` internally.
While it may store any instance of `intelli::NodeData`, only a specific node data type is allowed, which is either inferred during construction or when appending a data element.

The data appended is automatically converted to the node data type the list is set to.
If the conversion fails or the data element is null, the append does not succeed and no data element is appended.

## Specialized List Types

Specialized list types may be beneficial for node data types that are used frequently in a list.
Using a specialized list type may improve the performance of accessing elements and reduces memory consumption.

A specialized list type must be derived of `intelli::ListData` and must implement the following functions:

- `QString typeId() const` - which should yield the list type id of this class. You may use `intelli::typeId<LIST>()` with `LIST` denoting your list class.
- `size_t getSize() const noexcept` - which should yield the size of the list.
- `NodeDataPtr getAt(size_t idx) const` - which should yield the data element at the given position `idx`.
- `bool append(NodeDataPtr const& data) const` - which should append the data element `data` if it can be stored in the list. It should yield whether appending was successful. Null data elements should not be appended.

To implement a specialized list type, you must use the preprocessor macro `GT_INTELLI_DECLARE_LIST_TYPE(T, LIST)` with `T` denoting the type of the data element and `LIST` your specialized list type.

## TypeId API

> [!IMPORTANT]
> Include `intelli/list.h`, if you want to access a list-type of an existing node data type

```cpp
intelli::list<T>::type                    // registered list type for T (by default GernericListData)
intelli::inner_type_t<intelli::list<T>>   // yields T
intelli::inner_type_t<GernericListData>   // compile error, cannot infer inner type of GernericListData
intelli::is_list_type<T>::value           // yields true if T is a list type

// type ids
intelli::typeId<intelli::list<IntData>>() // same as
intelli::listTypeId<IntData>()            // "#list#intelli::IntData"
```
Note, `intelli::listTypeId<T>` may produce list types that are not yet registered

When working with string-based type ids, use the `intelli::NodeDataFactory` to access the associated list type or inner type respectively:

```cpp
auto& factory = intelli::NodeDataFactory::instance();
factory.isListType(id);       // returns whether 'id' is a list type
factory.hasListType(id);      // returns whether 'id' has a list type
factory.listType(id);         // scalar type id -> list type id
factory.innerType(id);        // list type id -> scalar type id
factory.makeData(factory.listType(id)); // same as
factory.makeListData(id);     // instantiate list data (accepts scalar or list id)
```

## Working with List Types

If you know the specialized list type, its best to use it:

```cpp
auto listData = nodeData<intelli::StringListData>(myPortId);
```

If you do not know the actual list type, use the common base class `intelli::ListData` to access size and elements of the list.

```cpp
auto listData = nodeData<intelli::ListData>(myPortId);
if (!listData) return evalFailed();

listData->size();                          // number of entries
listData->at(0);                           // get item
for (auto const& data : *listData) { ... } // iterate over items
```

If you want to instantiate list data, always use the `intelli::NodeDataFactory`:

```cpp
auto myTypeId = intelli::typeId<IntData>();
intelli::Ptr<intelli::ListData> listData = NodeDataFactory::instance().makeListData(myTypeId);
if (!listData) return evalFailed();

// add entry only works on non const node data
listData->append(intelli::makeNodeData<IntData>(42)); 
```
