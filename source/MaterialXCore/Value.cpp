//
// TM & (c) 2017 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#include <MaterialXCore/Value.h>

#include <MaterialXCore/Util.h>

#include <sstream>
#include <type_traits>

namespace MaterialX
{

Value::CreatorMap Value::_creatorMap;

namespace {

template <class T> using enable_if_mx_vector_t = typename std::enable_if<std::is_base_of<VectorBase, T>::value, T>::type;

template <class T> class is_std_vector : public std::false_type { };
template <class T, class Alloc> class is_std_vector< vector<T, Alloc> > : public std::true_type { };
template <class T> using enable_if_std_vector_t = typename std::enable_if<is_std_vector<T>::value, T>::type;

template <class T> void stringToData(const string& str, T& data);
template <class T> void dataToString(const T& data, string& str);

template <> void stringToData(const string& str, bool& data)
{
    if (str == VALUE_STRING_TRUE)
        data = true;
    else if (str == VALUE_STRING_FALSE)
        data = false;
    else
        throw ExceptionTypeError("Type mismatch in boolean stringToData: " + str);
}

template <> void stringToData(const string& str, string& data)
{
    data = str;
}

template <class T> void stringToData(const string& str, enable_if_mx_vector_t<T>& data)
{
    vector<string> tokens = splitString(str, ARRAY_VALID_SEPARATORS);
    if (tokens.size() != data.length())
    {
        throw ExceptionTypeError("Type mismatch in vector stringToData: " + str);
    }
    for (size_t i = 0; i < data.length(); i++)
    {
        stringToData(tokens[i], data[i]);
    }
}

template <class T> void stringToData(const string& str, enable_if_std_vector_t<T>& data)
{
    for (const string& token : splitString(str, ARRAY_VALID_SEPARATORS))
    {
        typename T::value_type val;
        stringToData(token, val);
        data.push_back(val);
    }
}

template <class T> void stringToData(const string& value, T& data)
{
    std::stringstream ss(value);
    if (!(ss >> data))
    {
        throw ExceptionTypeError("Type mismatch in generic stringToData: " + value);
    }
}

template <> void dataToString(const bool& data, string& str)
{
    str = data ? VALUE_STRING_TRUE : VALUE_STRING_FALSE;
}

template <> void dataToString(const string& data, string& str)
{
    str = data;
}

template <class T> void dataToString(const enable_if_mx_vector_t<T>& data, string& str)
{
    for (size_t i = 0; i < data.length(); i++)
    {
        string token;
        dataToString(data[i], token);
        str += token;
        if (i + 1 < data.length())
        {
            str += ARRAY_PREFERRED_SEPARATOR;
        }
    }
}

template <class T> void dataToString(const enable_if_std_vector_t<T>& data, string& str)
{
    for (size_t i = 0; i < data.size(); i++)
    {
        string token;
        dataToString<typename T::value_type>(data[i], token);
        str += token;
        if (i + 1 < data.size())
        {
            str += ARRAY_PREFERRED_SEPARATOR;
        }
    }
}

template <class T> void dataToString(const T& data, string& str)
{
    std::stringstream ss;
    ss << data;
    str = ss.str();
}

} // anonymous namespace

//
// Global functions
//

template<class T> const string& getTypeString()
{
    return TypedValue<T>::TYPE;
}

template <class T> string toValueString(const T& data)
{
    string value;
    dataToString<T>(data, value);
    return value;
}

template <class T> T fromValueString(const string& value)
{
    T data;
    stringToData<T>(value, data);
    return data;
}

//
// TypedValue methods
//

template <class T> const string& TypedValue<T>::getTypeString() const
{
    return TYPE;
}

template <class T> string TypedValue<T>::getValueString() const
{
    return toValueString<T>(_data);
}

template <class T> ValuePtr TypedValue<T>::createFromString(const string& value)
{
    try
    {
        return Value::createValue<T>(fromValueString<T>(value));
    }
    catch (ExceptionTypeError&)
    {
    }
    return ValuePtr();
}

//
// Value methods
//

ValuePtr Value::createValueFromStrings(const string& value, const string& type)
{
    CreatorMap::iterator it = _creatorMap.find(type);
    if (it != _creatorMap.end())
        return it->second(value);

    return TypedValue<string>::createFromString(value);
}

template<class T> bool Value::isA() const
{
    return dynamic_cast<const TypedValue<T>*>(this) != nullptr;
}

template<class T> T Value::asA() const
{
    const TypedValue<T>* typedVal = dynamic_cast<const TypedValue<T>*>(this);
    if (!typedVal)
    {
        throw ExceptionTypeError("Incorrect type specified for value");
    }
    return typedVal->getData();
}

//
// Value registry class
//

template <class T> class ValueRegistry
{
  public:
    ValueRegistry()
    {
        if (!Value::_creatorMap.count(TypedValue<T>::TYPE))
        {
            Value::_creatorMap[TypedValue<T>::TYPE] = TypedValue<T>::createFromString;
        }
    }
    ~ValueRegistry() { }
};

//
// Template instantiations
//

#define INSTANTIATE_TYPE(T, name)                       \
template <> const string TypedValue<T>::TYPE = name;    \
template bool Value::isA<T>() const;                    \
template T Value::asA<T>() const;                       \
template const string& getTypeString<T>();              \
ValueRegistry<T> registry##T;

// Base types
INSTANTIATE_TYPE(int, "integer")
INSTANTIATE_TYPE(bool, "boolean")
INSTANTIATE_TYPE(float, "float")
INSTANTIATE_TYPE(Color2, "color2")
INSTANTIATE_TYPE(Color3, "color3")
INSTANTIATE_TYPE(Color4, "color4")
INSTANTIATE_TYPE(Vector2, "vector2")
INSTANTIATE_TYPE(Vector3, "vector3")
INSTANTIATE_TYPE(Vector4, "vector4")
INSTANTIATE_TYPE(Matrix3x3, "matrix33")
INSTANTIATE_TYPE(Matrix4x4, "matrix44")
INSTANTIATE_TYPE(string, "string")

// Array types
INSTANTIATE_TYPE(IntVec, "integerarray")
INSTANTIATE_TYPE(BoolVec, "booleanarray")
INSTANTIATE_TYPE(FloatVec, "floatarray")
INSTANTIATE_TYPE(StringVec, "stringarray")

// Alias types
INSTANTIATE_TYPE(long, "integer")
INSTANTIATE_TYPE(double, "float")

} // namespace MaterialX
