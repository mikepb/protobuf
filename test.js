var fs = require('fs');

var protobuf = require('./index');
console.log('CODE', protobuf.toString());

var desc = fs.readFileSync('test/unittest.desc');
console.log('READ', desc);

var schema = protobuf(desc);
console.log('LOADED', schema);

var golden = fs.readFileSync('test/golden_message');
var descriptor = schema['protobuf_unittest.TestAllTypes'];

var message = descriptor.parse(golden);

console.log('MESSAGE', message);

console.log('DONE');
