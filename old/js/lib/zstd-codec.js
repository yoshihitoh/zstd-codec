const ArrayBufferHelper = require('./helpers.js').ArrayBufferHelper;
const constants = require('./constants.js');

const onReady = (binding) => {
    const codec = new binding.ZstdCodec();

    const withBindingInstance = (instance, callback) => {
        try {
            return callback(instance);
        }
        finally {
            instance.delete();
        }
    };

    const withCppVector = (callback) => {
        const vector = new binding.VectorU8();
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
                binding.cloneToVector(src, compressed_bytes);
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
                    binding.cloneToVector(src, content_bytes);
                    dest.resize(compressBound, 0);

                    var rc = codec.compress(dest, src, compression_level);
                    if (rc < 0) return null;    // `rc` is compressed size

                    dest.resize(rc, 0);
                    return binding.cloneAsTypedArray(dest);
                });
            });
        }

        decompress(compressed_bytes) {
            // use streaming-api, to support data without `frameContentSize`.
            return withCppVector((src) => {
                return withCppVector((dest) => {
                    binding.cloneToVector(src, compressed_bytes);

                    const contentSize = contentSizeImpl(src);
                    if (!contentSize) return null;

                    dest.resize(contentSize, 0);

                    var rc = codec.decompress(dest, src);
                    if (rc < 0 || rc != contentSize) return null;    // `rc` is compressed size

                    return binding.cloneAsTypedArray(dest);
                });
            });
        }

        compressUsingDict(content_bytes, cdict) {
            // use basic-api `compress`, to embed `frameContentSize`.

            const compressBound = compressBoundImpl(content_bytes.length);
            if (!compressBound) return null;

            return withCppVector((src) => {
                return withCppVector((dest) => {
                    binding.cloneToVector(src, content_bytes);
                    dest.resize(compressBound, 0);

                    var rc = codec.compressUsingDict(dest, src, cdict.get());
                    if (rc < 0) return null;    // `rc` is original content size

                    dest.resize(rc, 0);
                    return binding.cloneAsTypedArray(dest);
                });
            });
        }

        decompressUsingDict(compressed_bytes, ddict) {
            // use streaming-api, to support data without `frameContentSize`.
            return withCppVector((src) => {
                return withCppVector((dest) => {
                    binding.cloneToVector(src, compressed_bytes);

                    const contentSize = contentSizeImpl(src);
                    if (!contentSize) return null;

                    dest.resize(contentSize, 0);

                    var rc = codec.decompressUsingDict(dest, src, ddict.get());
                    if (rc < 0 || rc != contentSize) return null;    // `rc` is compressed size

                    return binding.cloneAsTypedArray(dest);
                });
            });
        }
    }

    class Streaming {
        compress(content_bytes, compression_level) {
            return withBindingInstance(new binding.ZstdCompressStreamBinding(), (stream) => {
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
            return withBindingInstance(new binding.ZstdCompressStreamBinding(), (stream) => {
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

        compressUsingDict(content_bytes, cdict) {
            return withBindingInstance(new binding.ZstdCompressStreamBinding(), (stream) => {
                const initial_size = compressBoundImpl(content_bytes.length);
                const sink = new ArrayBufferSink(initial_size);
                const callback = (compressed) => {
                    sink.concat(compressed);
                };

                if (!stream.beginUsingDict(cdict.get())) return null;
                if (!stream.transform(content_bytes, callback)) return null;
                if (!stream.end(callback)) return null;

                return sink.array();
            });
        }

        compressChunksUsingDict(chunks, size_hint, cdict) {
            return withBindingInstance(new binding.ZstdCompressStreamBinding(), (stream) => {
                const initial_size = size_hint || constants.STREAMING_DEFAULT_BUFFER_SIZE;
                const sink = new ArrayBufferSink(initial_size);
                const callback = (compressed) => {
                    sink.concat(compressed);
                };

                if (!stream.beginUsingDict(cdict.get())) return null;
                for (const chunk of chunks) {
                    if (!stream.transform(chunk, callback)) return null;
                }
                if (!stream.end(callback)) return null;

                return sink.array();
            });
        }

        decompress(compressed_bytes, size_hint) {
            return withBindingInstance(new binding.ZstdDecompressStreamBinding(), (stream) => {
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
            return withBindingInstance(new binding.ZstdDecompressStreamBinding(), (stream) => {
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

        decompressUsingDict(compressed_bytes, size_hint, ddict) {
            return withBindingInstance(new binding.ZstdDecompressStreamBinding(), (stream) => {
                const initial_size = size_hint || this._estimateContentSize(compressed_bytes);
                const sink = new ArrayBufferSink(initial_size);
                const callback = (decompressed) => {
                    sink.concat(decompressed);
                };

                if (!stream.beginUsingDict(ddict.get())) return null;
                if (!stream.transform(compressed_bytes, callback)) return null;
                if (!stream.end(callback)) return null;

                return sink.array();
            });
        }

        decompressChunksUsingDict(chunks, size_hint, ddict) {
            return withBindingInstance(new binding.ZstdDecompressStreamBinding(), (stream) => {
                const initial_size = size_hint || constants.STREAMING_DEFAULT_BUFFER_SIZE;
                const sink = new ArrayBufferSink(initial_size);
                const callback = (decompressed) => {
                    sink.concat(decompressed);
                };

                if (!stream.beginUsingDict(ddict.get())) return null;
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

    class ZstdCompressionDict {
        constructor(dict_bytes, compression_level) {
            this.binding = binding.createCompressionDict(dict_bytes, compression_level);
        }

        get() {
            return this.binding;
        }

        close() {
            if (this.binding) {
                this.binding.delete();
            }
        }

        delete() {
            this.close();
        }
    }

    class ZstdDecompressionDict {
        constructor(dict_bytes) {
            this.binding = new binding.createDecompressionDict(dict_bytes);
        }

        get() {
            return this.binding;
        }

        close() {
            if (this.binding) {
                this.binding.delete();
            }
        }

        delete() {
            this.close();
        }
    }

    const zstd = {};
    zstd.Generic = Generic;
    zstd.Simple = Simple;
    zstd.Streaming = Streaming;

    zstd.Dict = {};
    zstd.Dict.Compression = ZstdCompressionDict;
    zstd.Dict.Decompression = ZstdDecompressionDict;

    return zstd;
};

exports.run = (f) => {
    return require('./module.js').run((binding) => {
        const zstd = onReady(binding);
        f(zstd);
    });
};
