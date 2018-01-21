const zstd = require('./zstd-codec-binding.js')();


class ZstdCompressionDict {
    constructor(dict_bytes, compression_level) {
        this.binding = zstd.createCompressionDict(dict_bytes, compression_level);
    }

    get() {
        return this.binding;
    }

    close() {
        if (this.binding) {
            this.binding.delete();
        }
    }

    delete() {
        this.close();
    }
}


class ZstdDecompressionDict {
    constructor(dict_bytes) {
        this.binding = new zstd.createDecompressionDict(dict_bytes);
    }

    get() {
        return this.binding;
    }

    close() {
        if (this.binding) {
            this.binding.delete();
        }
    }

    delete() {
        this.close();
    }
}


exports.ZstdCompressionDict = ZstdCompressionDict;
exports.ZstdDecompressionDict = ZstdDecompressionDict;
