// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you
// may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.

#pragma once

#include <tr1/unordered_map>

#include <node.h>
#include <nan.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>

#include "descriptor.h"

namespace node {
namespace protobuf {

NAN_METHOD(Protobuf);

class Schema : public node::ObjectWrap {
  friend class Descriptor;

public:
  static void Init (v8::Handle<v8::Object> exports);
  static v8::Local<v8::Object> NewInstance (v8::Local<v8::Value> buf);

  Schema (const google::protobuf::DescriptorPool *pool);
  ~Schema ();

private:
  typedef std::tr1::unordered_map<
    const google::protobuf::Descriptor *,
    const Descriptor *
  > descriptor_map_type;

  const google::protobuf::DescriptorPool *pool_;
  google::protobuf::DynamicMessageFactory factory_;
  v8::Persistent<v8::Object> persistentHandle;
  descriptor_map_type descriptors_;

  static NAN_METHOD(New);
  // static NAN_METHOD(DescriptorGetter);

  void BuildDescriptors (
    const google::protobuf::FileDescriptor *fileDescriptor
  );

  google::protobuf::Message *NewMessage (
    const google::protobuf::Descriptor *descriptor
  );

  const Descriptor *DescriptorFor (
    const google::protobuf::FieldDescriptor *field
  ) const;
};

} // namespace protobuf
} // namespace node
