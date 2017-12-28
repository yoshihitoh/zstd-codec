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
            content_bytes.set(new Uint8Array(this.zstd.HEAPU8.buffer, dest_heap, content_size));

            return content_bytes;
        });
    }

    compress_stream(read_callback, write_callback, compression_level) {
        compression_level = this.compression_level(compression_level);
        const success = this.zstd.compress_stream(read_callback, write_callback, compression_level);
        return success;
    }

    decompress_stream(read_callback, write_callback) {
        const success = this.zstd.decompress_stream(read_callback, write_callback);
        return success;
    }

    compress_stream_fully(read_callback, bound_size, compression_level) {
        compression_level = this.compression_level(compression_level);
        let buffer = new ArrayBuffer(bound_size);
        let compressed_bytes = new Uint8Array(buffer);
        let dest_offset = 0;
        const success = this.zstd.compress_stream(read_callback, (write_bytes) => {
            compressed_bytes.set(write_bytes, dest_offset);
            dest_offset += write_bytes.length;
        }, compression_level);

        return success
            ? new Uint8Array(this.transfer_array_buffer(buffer, dest_offset)) // shrink to fit
            : null
            ;
    }

    decompress_stream_fully(read_callback, content_size_hint) {
        content_size_hint = content_size_hint || 256 * 1024;
        let buffer = new ArrayBuffer(content_size_hint);
        let content_bytes = new Uint8Array(buffer);
        let dest_offset = 0;
        const success = this.zstd.compress_stream(read_callback, (write_bytes) => {
            // grow ArrayBuffer when capacity is not enought
            buffer = this.ensure_array_buffer(buffer, dest_offset + write_bytes.length);
            content_bytes.set(write_bytes, dest_offset);
            dest_offset += write_bytes.length;
        }, compression_level);

        return success
            ? new Uint8Array(this.transfer_array_buffer(buffer, dest_offset)) // shrink to fit
            : null
            ;
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

    // TODO: separate ArrayBuffer related methods to another class.
    transfer_array_buffer(old_buffer, new_capacity) {
        const bytes = new Uint8Array(new ArrayBuffer(new_capacity));
        bytes.set(new Uint8Array(old_buffer.slice(0, new_capacity)));
        return bytes.buffer;
    }

    ensure_array_buffer(buffer, new_capacity) {
        if (buffer.byteLength >= new_capacity) {
            return buffer;
        }

        new_capacity = Math.max(new_capacity, buffer.byteLength * 2);
        return this.transfer_array_buffer(buffer, new_capacity);
    }
};
