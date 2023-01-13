const stream = require('stream');
const binding = require('./module.js').Binding;
const constants = require('./constants.js');
const helpers = require('./helpers.js');

const getClassName = helpers.getClassName;
const toTypedArray = helpers.toTypedArray;
const fromTypedArrayToBuffer = helpers.fromTypedArrayToBuffer;

const onReady = (binding) => {
    class ZstdCompressTransform extends stream.Transform {
        constructor(compression_level, string_decoder, option) {
            super(option || {});

            this.string_decoder = string_decoder;
            this.binding = new binding.ZstdCompressStreamBinding();
            this.binding.begin(compression_level || constants.DEFAULT_COMPRESSION_LEVEL);
            this.callback = (compressed) => {
                this.push(fromTypedArrayToBuffer(compressed), 'buffer');
            };
        }

        _transform(chunk, encoding, callback) {
            const chunkBytes = toTypedArray(chunk, encoding, this.string_decoder);
            if (!chunkBytes) {
                const type_name = getClassName(chunk) || typeof chunk;
                callback(new Error(`unsupported chunk type: ${type_name}`));
                return;
            }

            if (this.binding.transform(chunkBytes, this.callback)) {
                callback();
            }
            else {
                callback(new Error('ZstdDecompressTransform: Error on _transform'));
            }
        }

        _flush(callback) {
            if (this.binding.flush(this.callback)) {
                callback();
            }
            else {
                callback(new Error('ZstdDecompressTransform: Error on _flush'));
            }
        }

        _final(callback) {
            if (this.binding.end(this.callback)) {
                callback();
            }
            else {
                callback(new Error('ZstdDecompressTransform: Error on _final'));
            }
        }
    }


    class ZstdDecompressTransform extends stream.Transform {
        constructor(option) {
            super(option || {});

            this.binding = new binding.ZstdDecompressStreamBinding();
            this.binding.begin();
            this.callback = (decompressed) => {
                this.push(fromTypedArrayToBuffer(decompressed), 'buffer');
            };
        }

        _transform(chunk, encoding, callback) {
            const chunkBytes = toTypedArray(chunk, encoding, this.string_decoder);
            if (!chunkBytes) {
                const type_name = getClassName(chunk) || typeof chunk;
                callback(new Error(`unsupported chunk type: ${type_name}`));
                return;
            }

            if (this.binding.transform(chunk, this.callback)) {
                callback();
            }
            else {
                callback(new Error('ZstdDecompressTransform: Error on _transform'));
            }
        }

        _flush(callback) {
            if (this.binding.flush(this.callback)) {
                callback();
            }
            else {
                callback(new Error('ZstdDecompressTransform: Error on _flush'));
            }
        }

        _final(callback) {
            if (this.binding.end(this.callback)) {
                callback();
            }
            else {
                callback(new Error('ZstdDecompressTransform: Error on _final'));
            }
        }
    }

    const streams = {};
    streams.ZstdCompressTransform = ZstdCompressTransform;
    streams.ZstdDecompressTransform = ZstdDecompressTransform;
    return streams;
};

exports.run = (f) => {
    return require('./module.js').run((binding) => {
        const streams = onReady(binding);
        f(streams);
    });
};
