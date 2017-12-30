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
        // use basic-api `compress`, to embed `frameContentSize`.

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

    decompress(compressed_bytes, content_size_hint) {
        // use streaming-api, to support data without `frameContentSize`.
        const initial_buffer_size = this.content_size(compressed_bytes)
            || content_size_hint
            || this.estimate_content_size(compressed_bytes)
            ;

        let dest_buffer = new ArrayBuffer(initial_buffer_size);
        let dest_bytes = new Uint8Array(dest_buffer);
        let dest_offset = 0;
        let write_callback = (write_bytes) => {
            if (dest_offset + write_bytes.length > dest_buffer.byteLength) {
                dest_buffer = ArrayBufferHelper.transfer_array_buffer(dest_buffer, dest_buffer.byteLength * 2);
                dest_bytes = new Uint8Array(dest_buffer);
            }

            dest_bytes.set(write_bytes, dest_offset);
            dest_offset += write_bytes.length;
        };

        var decompress_stream = new this.zstd.ZstdDecompressStream();
        decompress_stream.Begin();
        decompress_stream.Transform(compressed_bytes, write_callback);
        decompress_stream.End(write_callback);

        if (dest_buffer.byteLength - dest_offset > 512 * 1024) {
            dest_buffer = ArrayBufferHelper.transfer_array_buffer(dest_buffer, dest_offset);
            dest_bytes = new Uint8Array(dest_buffer);
        }

        return dest_bytes.slice(0, dest_offset);
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
