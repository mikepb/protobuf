var assert = require('assert'),
    puts = require('util').puts,
    read = require('fs').readFileSync,
    Schema = require('../').Schema;

/* hack to make the tests pass with node v0.3.0's new Buffer model */
/* copied from http://github.com/bnoordhuis/node-iconv/blob/master/test.js */
assert.bufferEqual = function(a, b, c) {
	assert.equal(
		a.inspect().replace(/^<SlowBuffer/, '<Buffer'),
		b.inspect().replace(/^<SlowBuffer/, '<Buffer'),
                c);
};

describe('protobuf', function () {

  before(function () {
    this.source = read(__dirname + '/unittest.desc');
    this.golden = read(__dirname + '/golden_message');
  });

  it('should load schema', function () {
    this.schema = new Schema(this.source);
    this.descriptor = this.schema['protobuf_unittest.TestAllTypes'];
    assert(this.descriptor);
  });

  it('should parse message', function () {
    this.message = this.descriptor.parse(this.golden);
    assert(this.message);  // currently rather crashes
  });

});

/*

assert.bufferEqual(T.serialize(message), golden, 'roundtrip');

message.ignored = 42;
assert.bufferEqual(T.serialize(message), golden, 'ignored field');

assert.throws(function() {
  T.parse(new Buffer('invalid'));
}, Error, 'Should not parse');

assert.strictEqual(T.parse(
  T.serialize({
    optionalInt32: '3'
  })
).optionalInt32, 3, 'Number conversion');

assert.strictEqual(T.parse(
  T.serialize({
    optionalInt32: ''
  })
).optionalInt32, 0, 'Number conversion');

assert.strictEqual(T.parse(
  T.serialize({
    optionalInt32: 'foo'
  })
).optionalInt32, 0, 'Number conversion');

assert.strictEqual(T.parse(
  T.serialize({
    optionalInt32: {}
  })
).optionalInt32, 0, 'Number conversion');

assert.strictEqual(T.parse(
  T.serialize({
    optionalInt32: null
  })
).optionalInt32, undefined, 'null');

assert.throws(function() {
  T.serialize({
    optionalNestedEnum: 'foo'
  });
}, Error, 'Unknown enum');

assert.throws(function() {
  T.serialize({
    optionalNestedMessage: 3
  });
}, Error, 'Not an object');

assert.throws(function() {
  T.serialize({
    repeatedNestedMessage: ''
  });
}, Error, 'Not an array');

assert.bufferEqual(T.parse(
  T.serialize({
   optionalBytes: new Buffer('foo')
  })
).optionalBytes, new Buffer('foo'));

assert.bufferEqual(T.parse(
  T.serialize({
   optionalBytes: 'foo'
  })
).optionalBytes, new Buffer('foo'));

assert.bufferEqual(T.parse(
  T.serialize({
   optionalBytes: '\u20ac'
  })
).optionalBytes, new Buffer('\u00e2\u0082\u00ac', 'binary'));

assert.bufferEqual(T.parse(
  T.serialize({
   optionalBytes: '\u0000'
  })
).optionalBytes, new Buffer('\u0000', 'binary'));

assert.equal(T.parse(
  T.serialize({
   optionalString: new Buffer('f\u0000o')
  })
).optionalString, 'f\u0000o');

puts('Success');
*/
