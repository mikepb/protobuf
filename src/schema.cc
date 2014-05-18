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

#include <node.h>
#include <nan.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>

#include "schema.h"
#include "descriptor.h"

namespace node {
namespace protobuf {

static v8::Persistent<v8::FunctionTemplate> schema_constructor;

Schema::Schema (const google::protobuf::DescriptorPool *pool) : pool_(pool) {
  assert(pool_ != NULL);
  v8::Local<v8::Object> array = NanNew<v8::Array>();
  NanAssignPersistent(persistentHandle, array);
  factory_.SetDelegateToGeneratedFactory(true);
}

Schema::~Schema () {
  NanDisposePersistent(persistentHandle);
  if (pool_ != google::protobuf::DescriptorPool::generated_pool()) {
    delete pool_;
  }
}

google::protobuf::Message *Schema::NewMessage (
  const google::protobuf::Descriptor *descriptor
) {
  return factory_.GetPrototype(descriptor)->New();
}

const Descriptor *Schema::DescriptorFor (
  const google::protobuf::FieldDescriptor *field
) const {
  if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
    descriptor_map_type::const_iterator it =
      descriptors_.find(field->message_type());
    return it == descriptors_.end() ? NULL : it->second;
  }
  return NULL;
}

/* V8 exposed functions *****************************/

void Schema::Init (v8::Handle<v8::Object> exports) {
  v8::Local<v8::FunctionTemplate> t = NanNew<v8::FunctionTemplate>(Schema::New);
  NanAssignPersistent(schema_constructor, t);
  t->SetClassName(NanSymbol("Schema"));
  t->InstanceTemplate()->SetInternalFieldCount(1);
  exports->Set(NanSymbol("Schema"), t->GetFunction());
}

v8::Local<v8::Object> Schema::NewInstance (v8::Local<v8::Value> buf) {
  v8::Local<v8::Object> instance;
  v8::Local<v8::FunctionTemplate> constructorHandle =
    NanNew(schema_constructor);

  assert(!constructorHandle.IsEmpty());

  if (buf.IsEmpty()) {
    instance = constructorHandle->GetFunction()->NewInstance(0, NULL);
  } else {
    v8::Local<v8::Value> argv[] = { buf };
    instance = constructorHandle->GetFunction()->NewInstance(1, argv);
  }

  return instance;
}


NAN_METHOD(Schema::New) {
  NanScope();

  Schema *schema;

  if (!args.Length()) {

    const google::protobuf::DescriptorPool *pool =
      google::protobuf::DescriptorPool::generated_pool();

    schema = new Schema(pool);
    schema->Wrap(args.This());

  } else {

    assert(node::Buffer::HasInstance(args[0]));

    v8::Local<v8::Object> buf = args[0]->ToObject();
    char *data = node::Buffer::Data(buf);
    size_t size = node::Buffer::Length(buf);

    google::protobuf::FileDescriptorSet descriptors;
    if (!descriptors.ParseFromArray(data, size)) {
      return NanThrowError("Malformed descriptor");
    }

    const google::protobuf::DescriptorPool *pool =
      new google::protobuf::DescriptorPool;

    schema = new Schema(pool);
    schema->Wrap(args.This());

    for (int i = 0; i < descriptors.file_size(); i++) {
      const google::protobuf::FileDescriptor *fileDescriptor =
        const_cast<google::protobuf::DescriptorPool *>(pool)
          ->BuildFile(descriptors.file(i));
      schema->BuildDescriptors(fileDescriptor);
    }

  }

  NanReturnValue(args.This());
}

void Schema::BuildDescriptors (
  const google::protobuf::FileDescriptor *fileDescriptor
) {
  for (int i = 0; i < fileDescriptor->message_type_count(); i++) {
    const google::protobuf::Descriptor *pdesc =
      fileDescriptor->message_type(i);
    std::string pname = fileDescriptor->package() + "." + pdesc->name();
    v8::Local<v8::String> name =
      NanNew<v8::String>(pname.c_str(), pname.size());

    v8::Local<v8::Object> handle = NanObjectWrapHandle(this);
    assert(!handle.IsEmpty());
    v8::Local<v8::Object> wrap = Descriptor::NewInstance(handle, pdesc);
    assert(!wrap.IsEmpty());

    Descriptor *descriptor = node::ObjectWrap::Unwrap<Descriptor>(wrap);
    assert(descriptor != NULL);
    descriptors_[pdesc] = descriptor;

    handle->Set(name, wrap,
      static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8:: DontDelete));
  }
}

} // namespace protobuf
} // namespace node
