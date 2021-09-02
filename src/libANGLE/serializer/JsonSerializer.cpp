//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// JsonSerializer.cpp: Implementation of a JSON based serializer
// Note that for binary blob data only a checksum is stored so that
// a lossless  deserialization is not supported.

#include "JsonSerializer.h"

#include "common/debug.h"

#include <anglebase/sha1.h>
#include <rapidjson/document.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>

namespace angle
{

namespace js = rapidjson;

JsonSerializer::JsonSerializer() : mDoc(js::kObjectType), mAllocator(mDoc.GetAllocator()) {}

JsonSerializer::~JsonSerializer() {}

void JsonSerializer::startDocument(const std::string &name)
{
    startGroup(name);
}

void JsonSerializer::startGroup(const std::string &name)
{
    mGroupValueStack.push(SortedValueGroup());
    mGroupNameStack.push(name);
}

void JsonSerializer::endGroup()
{
    ASSERT(mGroupValueStack.size() >= 2);
    ASSERT(!mGroupNameStack.empty());

    SortedValueGroup &group = mGroupValueStack.top();
    std::string &name       = mGroupNameStack.top();

    SortedValueGroup::value_type new_entry = std::make_pair(name, makeValueGroup(group));

    mGroupValueStack.pop();
    mGroupNameStack.pop();

    mGroupValueStack.top().insert(std::move(new_entry));
}

void JsonSerializer::addBlob(const std::string &name, const uint8_t *blob, size_t length)
{
    unsigned char hash[angle::base::kSHA1Length];
    angle::base::SHA1HashBytes(blob, length, hash);
    std::ostringstream os;

    // Since we don't want to de-serialize the data we just store a checksum of the blob
    os << "SHA1:";
    static constexpr char kASCII[] = "0123456789ABCDEF";
    for (size_t i = 0; i < angle::base::kSHA1Length; ++i)
    {
        os << kASCII[hash[i] & 0xf] << kASCII[hash[i] >> 4];
    }

    std::ostringstream hashName;
    hashName << name << "-hash";
    addString(hashName.str(), os.str());

    std::vector<uint8_t> data((length < 16) ? length : static_cast<size_t>(16));
    std::copy(blob, blob + data.size(), data.begin());

    std::ostringstream rawName;
    rawName << name << "-raw[0-" << data.size() - 1 << ']';
    addVector(rawName.str(), data);
}

void JsonSerializer::addCString(const std::string &name, const char *value)
{
    rapidjson::Value tag(name.c_str(), mAllocator);
    rapidjson::Value val(value, mAllocator);
    mGroupValueStack.top().insert(std::make_pair(name, std::move(val)));
}

void JsonSerializer::addString(const std::string &name, const std::string &value)
{
    addCString(name, value.c_str());
}

void JsonSerializer::addVectorOfStrings(const std::string &name,
                                        const std::vector<std::string> &value)
{
    rapidjson::Value arrayValue(rapidjson::kArrayType);
    arrayValue.SetArray();

    for (const std::string &v : value)
    {
        rapidjson::Value str(v.c_str(), mAllocator);
        arrayValue.PushBack(str, mAllocator);
    }

    mGroupValueStack.top().insert(std::make_pair(name, std::move(arrayValue)));
}

void JsonSerializer::addBool(const std::string &name, bool value)
{
    rapidjson::Value boolValue(value);
    mGroupValueStack.top().insert(std::make_pair(name, std::move(boolValue)));
}

const char *JsonSerializer::data() const
{
    return mResult.c_str();
}

std::vector<uint8_t> JsonSerializer::getData() const
{
    return std::vector<uint8_t>(mResult.begin(), mResult.end());
}

void JsonSerializer::endDocument()
{
    // finalize last group
    ASSERT(!mGroupValueStack.empty());
    ASSERT(!mGroupNameStack.empty());

    rapidjson::Value nameValue(mGroupNameStack.top().c_str(), mAllocator);
    mDoc.AddMember(nameValue, makeValueGroup(mGroupValueStack.top()), mAllocator);

    mGroupValueStack.pop();
    mGroupNameStack.pop();
    ASSERT(mGroupValueStack.empty());
    ASSERT(mGroupNameStack.empty());

    std::stringstream os;
    js::OStreamWrapper osw(os);
    js::PrettyWriter<js::OStreamWrapper> prettyOs(osw);
    mDoc.Accept(prettyOs);
    mResult = os.str();
}

size_t JsonSerializer::length() const
{
    return mResult.length();
}

rapidjson::Value JsonSerializer::makeValueGroup(SortedValueGroup &group)
{
    rapidjson::Value valueGroup(js::kObjectType);
    for (auto &it : group)
    {
        rapidjson::Value tag(it.first.c_str(), mAllocator);
        valueGroup.AddMember(tag, it.second, mAllocator);
    }
    return valueGroup;
}

}  // namespace angle
