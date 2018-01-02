const ArrayBufferHelper = require('./helpers.js').ArrayBufferHelper;
const zstd = require('./zstd-codec-binding.js')();
const constants = require('./constants.js');
const codec = new zstd.ZstdCodec();


const withBindingInstance = (instance, callback) => {
    try {
        return callback(instance);
    }
    finally {
        instance.delete();
    }
};


const withCppVector = (callback) => {
    const vector = new zstd.VectorU8();
    return withBindingInstance(vector, callback);
};


const correctCompressionLevel = (compression_level) => {
    return compression_level || constants.DEFAULT_COMPRESSION_LEVEL;
};


const compressBoundImpl = (content_size) => {
    const rc = codec.compressBound(content_size);
    return rc >= 0 ? rc : null;
};


const contentSizeImpl = (src_vec) => {
    const rc = codec.contentSize(src_vec);
    return rc >= 0 ? rc : null;
};


class ArrayBufferSink {
    constructor(initial_size) {
        this._buffer = new ArrayBuffer(initial_size);
        this._array = new Uint8Array(this._buffer);
        this._offset = 0;
    }

    concat(array) {
        if (array.length + this._offset > this._array.length) {
            this._grow(Math.max(this._array.length, array.length) * 2);
        }

        this._array.set(array, this._offset);
        this._offset += array.length;
    }

    array() {
        // NOTE: clone buffer to shrink to fit
        const buffer = ArrayBufferHelper.transfer(this._buffer, this._offset);
        return new Uint8Array(buffer);
    }

    _grow(new_size) {
        this._buffer = ArrayBufferHelper.transfer(this._buffer, new_size);
        this._array = new Uint8Array(this._buffer);
    }
}


class Generic {
    compressBound(content_bytes) {
        return compressBoundImpl(content_bytes.length);
    }

    contentSize(compressed_bytes) {
        return withCppVector((src) => {
            zstd.cloneToVector(src, compressed_bytes);
            return contentSizeImpl(src);
        });
    }
}


class Simple {
    compress(content_bytes, compression_level) {
        // use basic-api `compress`, to embed `frameContentSize`.

        const compressBound = compressBoundImpl(content_bytes.length);
        if (!compressBound) return null;

        compression_level = correctCompressionLevel(compression_level);

        return withCppVector((src) => {
            return withCppVector((dest) => {
                zstd.cloneToVector(src, content_bytes);
                dest.resize(compressBound, 0);

                var rc = codec.compress(dest, src, compression_level);
                if (rc < 0) return null;    // `rc` is compressed size

                dest.resize(rc, 0);
                return zstd.cloneAsTypedArray(dest);
            });
        });
    }

    decompress(compressed_bytes) {
        // use streaming-api, to support data without `frameContentSize`.
        return withCppVector((src) => {
            return withCppVector((dest) => {
                zstd.cloneToVector(src, compressed_bytes);

                const contentSize = contentSizeImpl(src);
                if (!contentSize) return null;

                dest.resize(contentSize, 0);

                var rc = codec.decompress(dest, src);
                if (rc < 0 || rc != contentSize) return null;    // `rc` is compressed size

                return zstd.cloneAsTypedArray(dest);
            });
        });
    }
}


class Streaming {
    compress(content_bytes, compression_level) {
        return withBindingInstance(new zstd.ZstdCompressStreamBinding(), (stream) => {
            const initial_size = compressBoundImpl(content_bytes.length);
            const sink = new ArrayBufferSink(initial_size);
            const callback = (compressed) => {
                sink.concat(compressed);
            };

            const level = correctCompressionLevel(compression_level);

            if (!stream.begin(level)) return null;
            if (!stream.transform(content_bytes, callback)) return null;
            if (!stream.end(callback)) return null;

            return sink.array();
        });
    }

    compressChunks(chunks, size_hint, compression_level) {
        return withBindingInstance(new zstd.ZstdCompressStreamBinding(), (stream) => {
            const initial_size = size_hint || constants.STREAMING_DEFAULT_BUFFER_SIZE;
            const sink = new ArrayBufferSink(initial_size);
            const callback = (compressed) => {
                sink.concat(compressed);
            };

            const level = correctCompressionLevel(compression_level);

            if (!stream.begin(level)) return null;
            for (const chunk of chunks) {
                if (!stream.transform(chunk, callback)) return null;
            }
            if (!stream.end(callback)) return null;

            return sink.array();
        });
    }

    decompress(compressed_bytes, size_hint) {
        return withBindingInstance(new zstd.ZstdDecompressStreamBinding(), (stream) => {
            const initial_size = size_hint || this._estimateContentSize(compressed_bytes);
            const sink = new ArrayBufferSink(initial_size);
            const callback = (decompressed) => {
                sink.concat(decompressed);
            };

            if (!stream.begin()) return null;
            if (!stream.transform(compressed_bytes, callback)) return null;
            if (!stream.end(callback)) return null;

            return sink.array();
        });
    }

    decompressChunks(chunks, size_hint) {
        return withBindingInstance(new zstd.ZstdDecompressStreamBinding(), (stream) => {
            const initial_size = size_hint || constants.STREAMING_DEFAULT_BUFFER_SIZE;
            const sink = new ArrayBufferSink(initial_size);
            const callback = (decompressed) => {
                sink.concat(decompressed);
            };

            if (!stream.begin()) return null;
            for (const chunk of chunks) {
                if (!stream.transform(chunk, callback)) return null;
            }
            if (!stream.end(callback)) return null;

            return sink.array();
        });
    }

    _estimateContentSize(compressed_bytes) {
        // REF: https://code.facebook.com/posts/1658392934479273/smaller-and-faster-data-compression-with-zstandard/
        // with lzbench, ratio=3.11 .. 3.14. round up to integer
        return compressed_bytes.length * 4;
    }
}


exports.ZstdCodec = {};
exports.ZstdCodec.Generic = Generic;
exports.ZstdCodec.Simple = Simple;
exports.ZstdCodec.Streaming = Streaming;
