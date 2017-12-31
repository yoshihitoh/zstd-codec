class ArrayBufferHelper {
    static transfer(old_buffer, new_capacity) {
        const bytes = new Uint8Array(new ArrayBuffer(new_capacity));
        bytes.set(new Uint8Array(old_buffer.slice(0, new_capacity)));
        return bytes.buffer;
    }
}

exports.ArrayBufferHelper = ArrayBufferHelper;
