class ArrayBufferHelper {
    static transfer_array_buffer(old_buffer, new_capacity) {
        const bytes = new Uint8Array(new ArrayBuffer(new_capacity));
        bytes.set(new Uint8Array(old_buffer.slice(0, new_capacity)));
        return bytes.buffer;
    }

    static ensure_array_buffer(buffer, new_capacity) {
        if (buffer.byteLength >= new_capacity) {
            return buffer;
        }

        new_capacity = Math.max(new_capacity, buffer.byteLength * 2);
        return ArrayBufferHelper.transfer_array_buffer(buffer, new_capacity);
    }
}

exports.ArrayBufferHelper = ArrayBufferHelper;
