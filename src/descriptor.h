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

#include <node.h>
#include <nan.h>

#include <google/protobuf/descriptor.h>

namespace node {
namespace protobuf {

NAN_METHOD(Protobuf);

class Schema;
class Descriptor : public node::ObjectWrap {
public:
  static void Init (v8::Handle<v8::Object> exports);
  static v8::Local<v8::Object> NewInstance (
    v8::Local<v8::Object> handle,
    const google::protobuf::Descriptor *descriptor
  );

  Descriptor (
    v8::Local<v8::Object> handle,
    const Schema *schema,
    const google::protobuf::Descriptor *descriptor
  );

  ~Descriptor ();

private:
  const Schema *schema_;
  const google::protobuf::Descriptor *descriptor_;
  v8::Persistent<v8::Object> persistentHandle;

  static NAN_METHOD(New);
  static NAN_METHOD(Parse);
  static NAN_METHOD(Serialize);
  static NAN_METHOD(Fields);
  static NAN_METHOD(ToString);

  google::protobuf::Message *NewMessage ();

  const Descriptor *DescriptorFor (
    const google::protobuf::FieldDescriptor *field
  ) const;

  v8::Local<v8::Value> ProtoToJS (
    v8::Local<v8::Function> converter_,
    const google::protobuf::Message &message
  ) const;

  v8::Local<v8::Value> ProtoToJS (
    v8::Local<v8::Function> converter_,
    const google::protobuf::Message &message,
    const google::protobuf::Reflection *reflection,
    const google::protobuf::FieldDescriptor *field,
    const Descriptor *descriptor,
    const int index
  ) const;

  const char *JSToProto (
    v8::Local<v8::Function> converter_,
    google::protobuf::Message *message,
    v8::Local<v8::Object> src
  ) const;

  const char *JSToProto (
    v8::Local<v8::Function> converter_,
    google::protobuf::Message *message,
    const google::protobuf::FieldDescriptor *field,
    v8::Local<v8::Value> value,
    const Descriptor *descriptor,
    const bool repeated
  ) const;
};

} // namespace protobuf
} // namespace node
