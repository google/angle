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

#include <rapidjson/document.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>

#include <anglebase/sha1.h>

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
    auto group = std::make_unique<js::Value>(js::kObjectType);
    mGroupValueStack.push(std::move(group));
    mGroupNameStack.push(name);
}

void JsonSerializer::endGroup()
{
    ValuePointer group = std::move(mGroupValueStack.top());
    std::string name   = std::move(mGroupNameStack.top());
    mGroupValueStack.pop();
    mGroupNameStack.pop();
    rapidjson::Value name_value(name.c_str(), mAllocator);
    mGroupValueStack.top()->AddMember(name_value, *group, mAllocator);
}

void JsonSerializer::addBlob(const std::string &name, const uint8_t *blob, size_t length)
{
    rapidjson::Value tag(name.c_str(), mAllocator);
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
    addString(name, os.str());
}

void JsonSerializer::addCString(const std::string &name, const char *value)
{
    rapidjson::Value tag(name.c_str(), mAllocator);
    rapidjson::Value val(value, mAllocator);
    mGroupValueStack.top()->AddMember(tag, val, mAllocator);
}

void JsonSerializer::addString(const std::string &name, const std::string &value)
{
    addCString(name, value.c_str());
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
    mDoc.AddMember(name_value, *mGroupValueStack.top(), mAllocator);

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

}  // namespace angle
