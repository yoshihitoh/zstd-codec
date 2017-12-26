exports.ZstdCodec = class ZstdCodec {
    constructor() {
        this.zstd = require('./zstd.js')();
    }

    content_size(compressed_bytes) {
        return this.content_size(compressed_bytes);
    }

    compress(content_bytes) {
        const dest_size = this.zstd.compress_bound(content_bytes.length);
        return this.withHeap(dest_size, (dest_heap) => {
            var rc = this.zstd.compress(dest_heap, dest_size, content_bytes);
            if (rc < 0) return null;    // `rc` is compressed size

            const compressed_buffer = new ArrayBuffer(rc);
            const compressed_bytes = new Uint8Array(compressed_buffer);
            compressed_bytes.set(new Uint8Array(this.zstd.HEAPU8.buffer, dest_heap, rc));

            return compressed_bytes;
        });
    }

    decompress(compressed_bytes) {
        const content_size = this.zstd.content_size(compressed_bytes);
        return this.withHeap(content_size, (dest_heap) => {
            const rc = this.zstd.decompress(dest_heap, content_size, compressed_bytes);
            if (rc < 0 || rc !== content_size) return null;

            const content_buffer = new ArrayBuffer(content_size);
            const content_bytes = new Uint8Array(content_buffer);
            content_bytes.set(new Uint8Array(this.zstd.HEAPU8.buffer, dest_heap, content_size));

            return content_bytes;
        });
    }

    compress_stream(content_bytes, compression_level) {
        compression_level = compression_level || 3;
        const dest_size = this.zstd.compress_bound(content_bytes.length);
        const dest_buffer = new ArrayBuffer(dest_size);
        var src_offset = 0;
        var dest_offset = 0;
        var compressed_bytes = new Uint8Array(dest_buffer);
        const result = this.zstd.compress_stream((read_size) => {
            const readable_size = Math.min(read_size, content_bytes.length - src_offset);
            const read_bytes = new Uint8Array(content_bytes.buffer, src_offset, readable_size);
            src_offset += read_bytes.length;
            return read_bytes;
        }, (write_bytes) => {
            compressed_bytes.set(write_bytes, dest_offset);
            dest_offset += write_bytes.length;
        }, compression_level);

        return result
            ? compressed_bytes.slice(0, dest_offset)
            : null
            ;
    }

    decompress_stream(compressed_bytes) {
        const dest_size = this.zstd.content_size(compressed_bytes);
        const dest_buffer = new ArrayBuffer(dest_size);
        var src_offset = 0;
        var dest_offset = 0;
        var content_bytes = new Uint8Array(dest_buffer);
        const result = this.zstd.decompress_stream((read_size) => {
            const read_bytes = new Uint8Array(compressed_bytes.buffer, src_offset, read_size);
            src_offset += read_size;
            return read_bytes;
        }, (write_bytes) => {
            content_bytes.set(write_bytes, dest_offset);
            dest_offset += write_bytes.length;
        });

        return result ? content_bytes : null;
    }

    withHeap(size, callback) {
        const heap = this.zstd._malloc(size);
        try {
            return callback(heap);
        }
        finally {
            this.zstd._free(heap);
        }
    }
};
