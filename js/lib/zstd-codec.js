const ArrayBufferHelper = require('./helpers.js').ArrayBufferHelper;

exports.ZstdCodec = class ZstdCodec {
    constructor() {
        this.zstd = require('./zstd-codec-binding.js')();
        this.codec = new this.zstd.ZstdCodec();
    }

    with_cpp_vector(callback) {
        const vector = new this.zstd.VectorU8();
        try {
            return callback(vector);
        }
        finally {
            vector.delete();
        }
    }

    compress_bound(content_bytes) {
        const rc = this.codec.compressBound(content_bytes.length);
        return rc >= 0 ? rc : null;
    }

    content_size(compressed_bytes) {
        const self = this;
        return this.with_cpp_vector((src) => {
            self.zstd.cloneToVector(src, compressed_bytes);

            const rc = this.codec.contentSize(src);
            return rc >= 0 ? rc : null;
        });
    }

    compress(content_bytes, compression_level) {
        // use basic-api `compress`, to embed `frameContentSize`.

        const compress_bound = this.compress_bound(content_bytes);
        if (!compress_bound) return null;

        const DEFAULT_COMPRESSION_LEVEL = 3;
        compression_level = compression_level || DEFAULT_COMPRESSION_LEVEL;

        const self = this;
        const zstd = self.zstd;
        const codec = self.codec;
        return self.with_cpp_vector((src) => {
            return self.with_cpp_vector((dest) => {
                zstd.cloneToVector(src, content_bytes);
                dest.resize(compress_bound, 0);

                var rc = codec.compress(dest, src, compression_level);
                if (rc < 0) return null;    // `rc` is compressed size

                dest.resize(rc, 0);
                return zstd.cloneAsTypedArray(dest);
            });
        });
    }

    decompress(compressed_bytes, content_size_hint) {
        // use streaming-api, to support data without `frameContentSize`.

        const self = this;
        const zstd = self.zstd;
        const codec = self.codec;
        return self.with_cpp_vector((src) => {
            return self.with_cpp_vector((dest) => {
                zstd.cloneToVector(src, compressed_bytes);

                const content_size = this.content_size(compressed_bytes);
                if (!content_size) return null;

                dest.resize(content_size, 0);

                var rc = codec.decompress(dest, src);
                if (rc < 0 || rc != content_size) return null;    // `rc` is compressed size

                return zstd.cloneAsTypedArray(dest);
            });
        });
    }

    estimate_content_size(compressed_bytes) {
        return compressed_bytes.length * 10;
    }
};
