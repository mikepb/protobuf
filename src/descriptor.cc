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

#include <assert.h>

#include <string>
#include <vector>
#include <iostream>
#include <sstream>

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>

#include "schema.h"

using google::protobuf::Descriptor;
using google::protobuf::DescriptorPool;
using google::protobuf::FieldDescriptor;
using google::protobuf::Message;
using google::protobuf::MethodDescriptor;
using google::protobuf::Reflection;

using std::map;
using std::string;
using std::vector;
using std::cerr;
using std::endl;

namespace node {
namespace protobuf {

static v8::Persistent<v8::FunctionTemplate> descriptor_constructor;

const char E_NO_ARRAY[] = "Not an array";
const char E_NO_OBJECT[] = "Not an object";
const char E_UNKNOWN_ENUM[] = "Unknown enum value";

Descriptor::Descriptor (
  v8::Local<v8::Object> handle,
  const Schema *schema,
  const google::protobuf::Descriptor *descriptor
) : schema_(schema), descriptor_(descriptor) {
  assert(schema_ != NULL);
  assert(descriptor_ != NULL);
  NanAssignPersistent(persistentHandle, handle);
}

Descriptor::~Descriptor () {
  NanDisposePersistent(persistentHandle);
}

google::protobuf::Message *Descriptor::NewMessage () {
  return const_cast<Schema *>(schema_)->NewMessage(descriptor_);
}

const Descriptor *Descriptor::DescriptorFor (
  const google::protobuf::FieldDescriptor *field
) const {
  return schema_->DescriptorFor(field);
}

/* V8 exposed functions *****************************/

void Descriptor::Init (v8::Handle<v8::Object> exports) {
  v8::Local<v8::FunctionTemplate> t = NanNew<v8::FunctionTemplate>(Descriptor::New);
  NanAssignPersistent(descriptor_constructor, t);
  t->SetClassName(NanSymbol("Descriptor"));
  t->InstanceTemplate()->SetInternalFieldCount(1);
  NODE_SET_PROTOTYPE_METHOD(t, "parse", Parse);
  NODE_SET_PROTOTYPE_METHOD(t, "serialize", Serialize);
  NODE_SET_PROTOTYPE_METHOD(t, "fields", Fields);
  NODE_SET_PROTOTYPE_METHOD(t, "toString", ToString);
  exports->Set(NanSymbol("Descriptor"), t->GetFunction());
}

v8::Local<v8::Object> Descriptor::NewInstance (
  v8::Local<v8::Object> handle,
  const google::protobuf::Descriptor *descriptor
) {
  v8::Local<v8::FunctionTemplate> constructorHandle =
    NanNew(descriptor_constructor);

  assert(!constructorHandle.IsEmpty());

  v8::Local<v8::Value> argv[] = {
    handle,
    NanNew<v8::External>((void *)descriptor)
  };

  return constructorHandle->GetFunction()->NewInstance(2, argv);
}

NAN_METHOD(Descriptor::New) {
  NanScope();

  assert(args.Length() == 2);

  v8::Local<v8::Object> handle = args[0]->ToObject();
  Schema *schema = node::ObjectWrap::Unwrap<Schema>(handle);
  v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(args[1]);
  const google::protobuf::Descriptor *pdesc =
    static_cast<const google::protobuf::Descriptor *>(wrap->Value());

  Descriptor *descriptor = new Descriptor(handle, schema, pdesc);
  descriptor->Wrap(args.This());

  NanReturnValue(args.This());
}

NAN_METHOD(Descriptor::Parse) {
  NanScope();

  if (args.Length() != 1) {
    return NanThrowError("Expected single argument");
  } else if (!Buffer::HasInstance(args[0])) {
    return NanThrowError("Expected argument to be a Buffer");
  }

  Descriptor *descriptor = node::ObjectWrap::Unwrap<Descriptor>(args.This());
  v8::Local<v8::Object> buf = args[0]->ToObject();

  google::protobuf::Message *message = descriptor->NewMessage();
  bool success =
    message->ParseFromArray(node::Buffer::Data(buf), node::Buffer::Length(buf));

  v8::Local<v8::Value> result;

  if (success) {
    v8::Local<v8::Function> converter_ =
      args.This()->Get(NanSymbol("_arrayAsObject")).As<v8::Function>();
    assert(!converter_.IsEmpty());
    result = descriptor->ProtoToJS(converter_, *message);
  }

  delete message;

  if (!success) {
    return NanThrowError("Malformed message");
  }

  NanReturnValue(result);
}

NAN_METHOD(Descriptor::Serialize) {
  NanScope();

  if (args.Length() != 1) {
    return NanThrowError("Expected single argument");
  } else if (!args[0]->IsObject()) {
    return NanThrowError("Expected argument to be an Object");
  }

  Descriptor *descriptor = node::ObjectWrap::Unwrap<Descriptor>(args.This());

  v8::Local<v8::Function> converter_ =
    args.This()->Get(NanSymbol("_objectAsArray")).As<v8::Function>();
  assert(!converter_.IsEmpty());

  google::protobuf::Message *message = descriptor->NewMessage();
  const char *error =
    descriptor->JSToProto(converter_, message, args[0]->ToObject());

  v8::Local<v8::Object> buf;

  if (!error) {
    buf = NanNewBufferHandle(message->ByteSize());
    message->SerializeWithCachedSizesToArray(
      reinterpret_cast<google::protobuf::uint8 *>(node::Buffer::Data(buf)));
  }

  delete message;

  if (error) {
    return NanThrowError(error);
  }

  NanReturnValue(buf);
}

NAN_METHOD(Descriptor::Fields) {
  Descriptor *descriptor = node::ObjectWrap::Unwrap<Descriptor>(args.This());
  const google::protobuf::Descriptor *descriptor_ = descriptor->descriptor_;

  v8::Local<v8::Array> fields = NanNew<v8::Array>(descriptor_->field_count());

  for (int i = 0; i < descriptor_->field_count(); i++) {
    const google::protobuf::FieldDescriptor *field = descriptor_->field(i);
    fields->Set(i, NanNew<v8::String>(field->name().c_str()));
  }

  NanReturnValue(fields);
}

NAN_METHOD(Descriptor::ToString) {
  Descriptor *descriptor = node::ObjectWrap::Unwrap<Descriptor>(args.This());
  NanReturnValue(NanNew<v8::String>(descriptor->descriptor_->full_name().c_str()));
}

v8::Local<v8::Value> Descriptor::ProtoToJS(
  v8::Local<v8::Function> converter_,
  const google::protobuf::Message &message
) const {
  const google::protobuf::Reflection *reflection = message.GetReflection();
  const google::protobuf::Descriptor *descriptor = message.GetDescriptor();

  v8::Local<v8::Array> properties = NanNew<v8::Array>(descriptor->field_count());

  for (int i = 0; i < descriptor->field_count(); i++) {
    const google::protobuf::FieldDescriptor *field = descriptor->field(i);

    bool repeated = field->is_repeated();
    if (repeated && !reflection->FieldSize(message, field)) continue;
    if (!repeated && !reflection->HasField(message, field)) continue;

    const Descriptor *child = DescriptorFor(field);

    v8::Local<v8::Value> value;

    if (field->is_repeated()) {
      int size = reflection->FieldSize(message, field);
      v8::Local<v8::Array> array = NanNew<v8::Array>(size);
      for (int j = 0; j < size; j++) {
        array->Set(j, ProtoToJS(converter_, message, reflection, field, child, j));
      }
      value = array;
    } else {
      value = ProtoToJS(converter_, message, reflection, field, child, -1);
    }

    assert(!value.IsEmpty());
    properties->Set(i, value);
  }

  assert(!converter_.IsEmpty());
  return converter_->Call(properties, 0, NULL);
}

#define GET(TYPE) (                                                  \
index >= 0 ?                                                         \
 reflection->GetRepeated##TYPE(message, field, index) :              \
 reflection->Get##TYPE(message, field))

v8::Local<v8::Value> Descriptor::ProtoToJS(
  v8::Local<v8::Function> converter_,
  const google::protobuf::Message &message,
  const google::protobuf::Reflection *reflection,
  const google::protobuf::FieldDescriptor *field,
  const Descriptor *descriptor,
  const int index
) const {
  switch (field->cpp_type()) {
  case FieldDescriptor::CPPTYPE_MESSAGE:
    assert(descriptor != NULL);
    return descriptor->ProtoToJS(converter_, GET(Message));
  case FieldDescriptor::CPPTYPE_STRING: {
    const string &value = GET(String);
    if (field->type() == FieldDescriptor::TYPE_BYTES) {
      return NanNewBufferHandle(
        const_cast<char *>(value.data()), value.length());
    } else {
      return NanNew<v8::String>(value.data(), value.length());
    }
  }
  case FieldDescriptor::CPPTYPE_INT32:
    return NanNew<v8::Int32>(GET(Int32));
  case FieldDescriptor::CPPTYPE_UINT32:
    return NanNew<v8::Uint32>(GET(UInt32));
  case FieldDescriptor::CPPTYPE_INT64: {
    std::ostringstream ss;
    ss << GET(Int64);
    string s = ss.str();
    return NanNew<v8::String>(s.data(), s.length());
  }
  case FieldDescriptor::CPPTYPE_UINT64: {
    std::ostringstream ss;
    ss << GET(UInt64);
    string s = ss.str();
    return NanNew<v8::String>(s.data(), s.length());
  }
  case FieldDescriptor::CPPTYPE_FLOAT:
    return NanNew<v8::Number>(GET(Float));
  case FieldDescriptor::CPPTYPE_DOUBLE:
    return NanNew<v8::Number>(GET(Double));
  case FieldDescriptor::CPPTYPE_BOOL:
    return GET(Bool) ? NanTrue() : NanFalse();
  case FieldDescriptor::CPPTYPE_ENUM:
    return NanNew<v8::String>(GET(Enum)->name().c_str());
  }

  assert(false);  // NOTREACHED
}
#undef GET

const char *Descriptor::JSToProto (
  v8::Local<v8::Function> converter_,
  google::protobuf::Message *message,
  v8::Local<v8::Object> src
) const {
  assert(!converter_.IsEmpty());
  v8::Local<v8::Array> properties = converter_->Call(src, 0, NULL).As<v8::Array>();

  const char *error = NULL;

  for (int i = 0; error == NULL && i < descriptor_->field_count(); i++) {
    v8::Local<v8::Value> value = properties->Get(i);

    if (value->IsUndefined() || value->IsNull()) continue;

    const google::protobuf::FieldDescriptor *field = descriptor_->field(i);
    const Descriptor *child = DescriptorFor(field);

    if (field->is_repeated()) {
      if (!value->IsArray()) {
        return E_NO_ARRAY;
      }

      v8::Local<v8::Array> array = value.As<v8::Array>();

      int length = array->Length();
      for (int j = 0; j < length; j++) {
        error = JSToProto(converter_, message, field, array->Get(j), child, true);
      }
    } else {
      error = JSToProto(converter_, message, field, value, child, false);
    }
  }

  return error;
}

#define SET(TYPE, EXPR)                                              \
  if (repeated) reflection->Add##TYPE(message, field, EXPR);         \
  else reflection->Set##TYPE(message, field, EXPR)

const char *Descriptor::JSToProto(
  v8::Local<v8::Function> converter_,
  google::protobuf::Message *message,
  const google::protobuf::FieldDescriptor *field,
  v8::Local<v8::Value> value,
  const Descriptor *descriptor,
  const bool repeated
) const {
  const Reflection *reflection = message->GetReflection();

  switch (field->cpp_type()) {
  case FieldDescriptor::CPPTYPE_MESSAGE:
    if (!value->IsObject()) {
      return E_NO_OBJECT;
    }
    descriptor->JSToProto(converter_,
      repeated ?
      reflection->AddMessage(message, field) :
      reflection->MutableMessage(message, field),
      value->ToObject()
    );
    break;
  case FieldDescriptor::CPPTYPE_STRING: {
    if (node::Buffer::HasInstance(value)) {
      v8::Local<v8::Object> buf = value->ToObject();
      SET(String, std::string(node::Buffer::Data(buf), node::Buffer::Length(buf)));
    } else {
      v8::String::Utf8Value utf8(value);
      SET(String, std::string(*utf8, utf8.length()));
    }
    break;
  }
  case FieldDescriptor::CPPTYPE_INT32:
    SET(Int32, value->Int32Value());
    break;
  case FieldDescriptor::CPPTYPE_UINT32:
    SET(UInt32, value->Uint32Value());
    break;
  case FieldDescriptor::CPPTYPE_INT64:
    if (value->IsString()) {
      google::protobuf::int64 ll;
      std::istringstream(*v8::String::Utf8Value(value)) >> ll;
      SET(Int64, ll);
    } else {
      SET(Int64, value->NumberValue());
    }
    break;
  case FieldDescriptor::CPPTYPE_UINT64:
    if (value->IsString()) {
      google::protobuf::uint64 ull;
      std::istringstream(*v8::String::Utf8Value(value)) >> ull;
      SET(UInt64, ull);
    } else {
      SET(UInt64, value->NumberValue());
    }
    break;
  case FieldDescriptor::CPPTYPE_FLOAT:
    SET(Float, value->NumberValue());
    break;
  case FieldDescriptor::CPPTYPE_DOUBLE:
    SET(Double, value->NumberValue());
    break;
  case FieldDescriptor::CPPTYPE_BOOL:
    SET(Bool, value->BooleanValue());
    break;
  case FieldDescriptor::CPPTYPE_ENUM: {
    char *cstr = NULL;

    if (value->IsString()) {
      size_t len;
      cstr = NanCString(value, &len);
    }

    const google::protobuf::EnumValueDescriptor *enum_value =
      value->IsNumber() ?
      field->enum_type()->FindValueByNumber(value->Int32Value()) :
      field->enum_type()->FindValueByName(cstr);

    if (cstr) {
      delete[] cstr;
    }

    if (!enum_value) {
      return E_UNKNOWN_ENUM;
    }

    SET(Enum, enum_value);

    break;
  }
  }

  return NULL;
}
#undef SET

} // namespace protobuf
} // namespace node
