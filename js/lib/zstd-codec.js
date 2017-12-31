const ArrayBufferHelper = require('./helpers.js').ArrayBufferHelper;
const zstd = require('./zstd-codec-binding.js')();
const codec = new zstd.ZstdCodec();


const withCppVector = (callback) => {
    const vector = new zstd.VectorU8();
    try {
        return callback(vector);
    }
    finally {
        vector.delete();
    }
};


const compressBoundImpl = (content_size) => {
    const rc = codec.compressBound(content_size);
    return rc >= 0 ? rc : null;
};


const contentSizeImpl = (src_vec) => {
    const rc = codec.contentSize(src_vec);
    return rc >= 0 ? rc : null;
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

        const DEFAULT_COMPRESSION_LEVEL = 3;
        compression_level = compression_level || DEFAULT_COMPRESSION_LEVEL;

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


exports.ZstdCodec = {};
exports.ZstdCodec.Generic = Generic;
exports.ZstdCodec.Simple = Simple;
