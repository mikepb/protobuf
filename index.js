var binding;

try {
  binding = require('./build/Release/binding.node');
} catch (err) {
  binding = require('./build/Debug/binding.node');
}

exports = module.exports = function load (buf) {
  return new exports.Schema(buf);
}

exports.Schema = Schema;
exports.Descriptor = Descriptor;

function Schema (source) {
  var schema = new binding.Schema(source);
  for (var key in schema) {
    this[key] = new exports.Descriptor(this, schema[key]);
  }
};

function Descriptor (schema, descriptor) {
  var fields = descriptor.fields();

  Object.defineProperties(descriptor, {
    _objectAsArray: {
      get: function () {
        var result = [];
        for (var i = 0; i < fields.length; i++) {
          result[i] = this[fields[i]];
        }
        return result;
      }
    },
    _arrayAsObject: {
      get: function () {
        var result = {};
        for (var i = 0; i < fields.length; i++) {
          result[fields[i]] = this[i];
        }
        return result;
      }
    }
  });

  return descriptor;
};
