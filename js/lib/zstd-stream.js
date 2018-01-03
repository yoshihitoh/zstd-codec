const stream = require('stream');
const zstd = require('./zstd-codec-binding.js')();
const constants = require('./constants.js');


const getClassName = (obj) => {
    if (!obj || typeof obj != 'object') return null;

    // Object.prototype.toString returns '[object ClassName]',
    // remove prefix '[object ' and suffix ']'
    return Object.prototype.toString.call(obj).slice('[object '.length, -1);
};


const isUint8Array = (obj) => {
    return getClassName(obj) == 'Uint8Array';
};


const isString = (obj) => {
    return typeof obj == 'string' || getClassName(obj) == 'String';
};


const toTypedArray = (chunk, encoding, string_decoder) => {
    if (isString(chunk)) {
        chunk = string_decoder(encoding);
    }

    if (isUint8Array(chunk)) {
        // NOTE: Buffer is recognized as Uint8Array object.
        return chunk;
    }
    else if (getClassName(chunk) == 'ArrayBuffer') {
        return new Uint8Array(chunk);
    }
    else if (Array.isArray(chunk)) {
        return new Uint8Array(chunk);
    }

    return null;
};


const fromTypedArrayToBuffer = (typedArray) => {
    return Buffer.from(typedArray.buffer);
};


class ZstdCompressTransform extends stream.Transform {
    constructor(compression_level, string_decoder, option) {
        super(option || {});

        this.string_decoder = string_decoder;
        this.binding = new zstd.ZstdCompressStreamBinding();
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

        this.binding = new zstd.ZstdDecompressStreamBinding();
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

exports.ZstdCompressTransform = ZstdCompressTransform;
exports.ZstdDecompressTransform = ZstdDecompressTransform;
