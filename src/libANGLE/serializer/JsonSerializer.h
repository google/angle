//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// JsonSerializer.h: Implementation of a JSON based serializer
//

#ifndef LIBANGLE_JSONSERIALIZER_H_
#define LIBANGLE_JSONSERIALIZER_H_

#include "common/angleutils.h"

#if !defined(ANGLE_HAS_RAPIDJSON)
#    error RapidJSON must be available to build this file.
#endif  // !defined(ANGLE_HAS_RAPIDJSON)

#include <rapidjson/document.h>

#include <map>
#include <memory>
#include <sstream>
#include <stack>
#include <type_traits>

namespace angle
{

// Rapidjson has problems picking the right AddMember template for long
// integer types, so let's just make these values use 64 bit variants
template <typename T>
struct StoreAs
{
    using Type = T;
};

template <>
struct StoreAs<unsigned long>
{
    using Type = uint64_t;
};

template <>
struct StoreAs<signed long>
{
    using Type = int64_t;
};

class JsonSerializer : public angle::NonCopyable
{
  public:
    JsonSerializer();
    ~JsonSerializer();

    void startDocument(const std::string &name);
    void endDocument();

    void addCString(const std::string &name, const char *value);

    void addString(const std::string &name, const std::string &value);

    void addBlob(const std::string &name, const uint8_t *value, size_t length);

    void startGroup(const std::string &name);

    void endGroup();

    const char *data() const;

    std::vector<uint8_t> getData() const;

    size_t length() const;

    template <typename T>
    void addScalar(const std::string &name, T value)
    {
        typename StoreAs<T>::Type v = value;
        mGroupValueStack.top().insert(std::make_pair(name, rapidjson::Value(v)));
    }

    template <typename T>
    void addVector(const std::string &name, const std::vector<T> &value)
    {
        rapidjson::Value array(rapidjson::kArrayType);
        array.SetArray();

        for (typename StoreAs<T>::Type v : value)
            array.PushBack(v, mAllocator);

        mGroupValueStack.top().insert(std::make_pair(name, std::move(array)));
    }

    template <typename T>
    void addVectorAsHash(const std::string &name, const std::vector<T> &value)
    {
        addBlob(name, reinterpret_cast<const uint8_t *>(&value[0]), value.size() * sizeof(T));
    }

    void addVectorOfStrings(const std::string &name, const std::vector<std::string> &value);

  private:
    using SortedValueGroup = std::multimap<std::string, rapidjson::Value>;

    rapidjson::Value makeValueGroup(SortedValueGroup &group);

    using ValuePointer = std::unique_ptr<rapidjson::Value>;

    rapidjson::Document mDoc;
    rapidjson::Document::AllocatorType &mAllocator;
    std::stack<std::string> mGroupNameStack;
    std::stack<SortedValueGroup> mGroupValueStack;
    std::string mResult;
};

}  // namespace angle

#endif  // JSONSERIALIZER_H
