const ArrayBufferHelper = require('./helpers.js').ArrayBufferHelper;

exports.ZstdCodec = class ZstdCodec {
    constructor() {
        this.zstd = require('./zstd-binding.js')();
    }

    compress_bound(content_bytes) {
        const rc = this.zstd.compress_bound(content_bytes.length);
        return rc >= 0 ? rc : null;
    }

    content_size(compressed_bytes) {
        const rc = this.zstd.content_size(compressed_bytes);
        return rc >= 0 ? rc : null;
    }

    compress(content_bytes, compression_level) {
        compression_level = this.compression_level(compression_level);
        const dest_size = this.zstd.compress_bound(content_bytes.length);
        return this.with_heap(dest_size, (dest_heap) => {
            var rc = this.zstd.compress(dest_heap, dest_size, content_bytes, compression_level);
            if (rc < 0) return null;    // `rc` is compressed size

            const compressed_buffer = new ArrayBuffer(rc);
            const compressed_bytes = new Uint8Array(compressed_buffer);
            compressed_bytes.set(new Uint8Array(this.zstd.HEAPU8.buffer, dest_heap, rc));

            return compressed_bytes;
        });
    }

    decompress(compressed_bytes) {
        const content_size = this.zstd.content_size(compressed_bytes);
        return this.with_heap(content_size, (dest_heap) => {
            const rc = this.zstd.decompress(dest_heap, content_size, compressed_bytes);
            if (rc < 0 || rc !== content_size) return null;

            const content_buffer = new ArrayBuffer(content_size);
            const content_bytes = new Uint8Array(content_buffer);
            content_bytes.set(new Uint8Array(this.zstd.HEAPU8.buffer, dest_heap, content_size

                return content_bytes;
            });
        }

    with_heap(size, callback) {
        const heap = this.zstd._malloc(size);
        try {
            return callback(heap);
        }
        finally {
            this.zstd._free(heap);
        }
    }

    compression_level(level) {
        const DEFAULT_COMPRESSION_LEVEL = 3;
        return level || DEFAULT_COMPRESSION_LEVEL;
    }

    estimate_content_size(compressed_bytes) {
        return compressed_bytes.length * 10;
    }
};
