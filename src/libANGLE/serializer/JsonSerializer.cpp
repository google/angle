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
    SortedValueGroup group = std::move(mGroupValueStack.top());
    std::string name       = std::move(mGroupNameStack.top());
    mGroupValueStack.pop();
    mGroupNameStack.pop();

    mGroupValueStack.top().insert(std::make_pair(name, makeValueGroup(group)));
}

void JsonSerializer::addBlob(const std::string &name, const uint8_t *blob, size_t length)
{
    unsigned char hash[angle::base::kSHA1Length];
    angle::base::SHA1HashBytes(blob, length, hash);
    std::ostringstream os;

    // Since we don't want to de-serialize the data we just store a checksume
    // of the blob
    os << "SHA1:";
    static constexpr char kASCII[] = "0123456789ABCDEF";
    for (size_t i = 0; i < angle::base::kSHA1Length; ++i)
    {
        os << kASCII[hash[i] & 0xf] << kASCII[hash[i] >> 4];
    }

    std::ostringstream hash_name;
    hash_name << name << "-hash";
    addString(hash_name.str(), os.str());

    std::vector<uint8_t> data(length < 16 ? length : (size_t)16);
    std::copy(blob, blob + data.size(), data.begin());

    std::ostringstream raw_name;
    raw_name << name << "-raw[0-" << data.size() - 1 << ']';
    addVector(raw_name.str(), data);
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
    rapidjson::Value array(rapidjson::kArrayType);
    array.SetArray();

    for (const std::string &v : value)
    {
        rapidjson::Value str(v.c_str(), mAllocator);
        array.PushBack(str, mAllocator);
    }

    mGroupValueStack.top().insert(std::make_pair(name, std::move(array)));
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

    rapidjson::Value name_value(mGroupNameStack.top().c_str(), mAllocator);
    mDoc.AddMember(name_value, makeValueGroup(mGroupValueStack.top()), mAllocator);

    mGroupValueStack.pop();
    mGroupNameStack.pop();
    ASSERT(mGroupValueStack.empty());
    ASSERT(mGroupNameStack.empty());

    std::stringstream os;
    js::OStreamWrapper osw(os);
    js::PrettyWriter<js::OStreamWrapper> pretty_os(osw);
    mDoc.Accept(pretty_os);
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
