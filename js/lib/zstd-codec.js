const ArrayBufferHelper = require('./helpers.js').ArrayBufferHelper;
const zstd = require('./zstd-codec-binding.js')();

class ZstdCodec {
    constructor() {
        this.codec = new zstd.ZstdCodec();
    }

    _withCppVector(callback) {
        const vector = new zstd.VectorU8();
        try {
            return callback(vector);
        }
        finally {
            vector.delete();
        }
    }

    compressBound(content_bytes) {
        const rc = this.codec.compressBound(content_bytes.length);
        return rc >= 0 ? rc : null;
    }

    contentSize(compressed_bytes) {
        return this._withCppVector((src) => {
            zstd.cloneToVector(src, compressed_bytes);

            const rc = this.codec.contentSize(src);
            return rc >= 0 ? rc : null;
        });
    }

    compress(content_bytes, compression_level) {
        // use basic-api `compress`, to embed `frameContentSize`.

        const compressBound = this.compressBound(content_bytes);
        if (!compressBound) return null;

        const DEFAULT_COMPRESSION_LEVEL = 3;
        compression_level = compression_level || DEFAULT_COMPRESSION_LEVEL;

        const self = this;
        const codec = self.codec;
        return self._withCppVector((src) => {
            return self._withCppVector((dest) => {
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

        const self = this;
        const codec = self.codec;
        return self._withCppVector((src) => {
            return self._withCppVector((dest) => {
                zstd.cloneToVector(src, compressed_bytes);

                const contentSize = this.contentSize(compressed_bytes);
                if (!contentSize) return null;

                dest.resize(contentSize, 0);

                var rc = codec.decompress(dest, src);
                if (rc < 0 || rc != contentSize) return null;    // `rc` is compressed size

                return zstd.cloneAsTypedArray(dest);
            });
        });
    }
}

exports.ZstdCodec = ZstdCodec;
