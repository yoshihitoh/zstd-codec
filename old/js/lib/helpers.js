class ArrayBufferHelper {
    static transfer(old_buffer, new_capacity) {
        const bytes = new Uint8Array(new ArrayBuffer(new_capacity));
        bytes.set(new Uint8Array(old_buffer.slice(0, new_capacity)));
        return bytes.buffer;
    }
}


const getClassName = (obj) => {
    if (!obj || typeof obj != 'object') return null;

    // Object.prototype.toString returns '[object ClassName]',
    // remove prefix '[object ' and suffix ']'
    return Object.prototype.toString.call(obj).slice('[object '.length, -1);
};


const isUint8Array = (obj) => {
    return getClassName(obj) == 'Uint8Array';
};


const isString = (obj) => {
    return typeof obj == 'string' || getClassName(obj) == 'String';
};


const toTypedArray = (chunk, encoding, string_decoder) => {
    if (isString(chunk)) {
        chunk = string_decoder(encoding);
    }

    if (isUint8Array(chunk)) {
        // NOTE: Buffer is recognized as Uint8Array object.
        return chunk;
    }
    else if (getClassName(chunk) == 'ArrayBuffer') {
        return new Uint8Array(chunk);
    }
    else if (Array.isArray(chunk)) {
        return new Uint8Array(chunk);
    }

    return null;
};


// NOTE: only available on Node.js environment
const fromTypedArrayToBuffer = (typedArray) => {
    return Buffer.from(typedArray.buffer);
};


exports.ArrayBufferHelper = ArrayBufferHelper;
exports.getClassName = getClassName;
exports.isUint8Array = isUint8Array;
exports.isString = isString;
exports.toTypedArray = toTypedArray;
exports.fromTypedArrayToBuffer = fromTypedArrayToBuffer;
