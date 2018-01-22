const fs = require('fs');
const path = require('path');
const textEncoding = require('text-encoding');
const TextEncoder = textEncoding.TextEncoder;
const ZstdCodec = require('../zstd-codec.js').ZstdCodec;

const zstd_dict = require('../zstd-dict.js');
const ZstdCompressionDict = zstd_dict.ZstdCompressionDict;
const ZstdDecompressionDict = zstd_dict.ZstdDecompressionDict;

const fixturePath = (name) => {
    return path.join(__dirname, 'fixtures', name);
};

const fixtureBinary = (name) => {
    const data = fs.readFileSync(fixturePath(name));
    return new Uint8Array(data);
};

const tempPath = (name) => {
    return path.join(__dirname, 'tmp', name);
};

const LOREM_TEXT = 'Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.';


class TypedArrayChunks {
    constructor(array, chunk_size) {
        this._array = array;
        this._chunk_size = chunk_size;
        this._offset = 0;
    }

    [Symbol.iterator]() {
        return this;
    }

    next() {
        if (this._offset >= this._array.length) {
            return { done: true, value: undefined };
        }

        const chunk = this._array.slice(this._offset, this._offset + this._chunk_size);
        this._offset += this._chunk_size;
        return { done: false, value: chunk };
    }
}


describe('ZstdCodec.Generic', () => {
    const generic = new ZstdCodec.Generic();

    describe('compressBound()', () => {
        it('should retrieve compressed size in worst case', () => {
            expect(generic.compressBound(new Uint8Array())).toBeGreaterThan(0);

            const lorem_bytes = fixtureBinary('lorem.txt');
            expect(generic.compressBound(lorem_bytes)).toBeGreaterThan(0);
        });
    });

    describe('contentSize()', () => {
        it('should retrieve content size from zstd compressed files', () => {
            expect(generic.contentSize(fixtureBinary('lorem.txt.zst'))).toBe(446);
            expect(generic.contentSize(fixtureBinary('dance_yorokobi_mai_man.bmp.zst'))).toBe(1920054);
            expect(generic.contentSize(fixtureBinary('dance_yorokobi_mai_woman.bmp.zst'))).toBe(1920054);
        });
    });
});


describe('ZstdCodec.Simple', () => {
    const simple = new ZstdCodec.Simple();

    describe('compress()', () => {
        it('should compress data', () => {
            const lorem_bytes = new TextEncoder('utf-8').encode(LOREM_TEXT);
            const compressed_bytes = simple.compress(lorem_bytes);

            expect(compressed_bytes.length).toBeLessThan(lorem_bytes.length);
            expect(simple.decompress(compressed_bytes)).toEqual(lorem_bytes);
        });
    });

    describe('decompress()', () => {
        it('should decompress data', () => {
            const lorem_bytes = simple.decompress(fixtureBinary('lorem.txt.zst'));
            expect(lorem_bytes).toEqual(fixtureBinary('lorem.txt'));
            // NOTE: use .toString() to avoid RangeError.
            const man_bytes = simple.decompress(fixtureBinary('dance_yorokobi_mai_man.bmp.zst'));
            expect(man_bytes.toString()).toEqual(fixtureBinary('dance_yorokobi_mai_man.bmp').toString());

            const woman_bytes = simple.decompress(fixtureBinary('dance_yorokobi_mai_woman.bmp.zst'));
            expect(woman_bytes.toString()).toEqual(fixtureBinary('dance_yorokobi_mai_woman.bmp').toString());
        });
    });

    describe('compressUsingDict', () => {
        it('should compress data', () => {
            const compression_level = 5;
            const dict_bytes = fixtureBinary('sample-dict');
            const cdict = new ZstdCompressionDict(dict_bytes, compression_level);

            const books_bytes = fixtureBinary("sample-books.json");
            const compressed_bytes = simple.compressUsingDict(books_bytes, cdict);
            expect(compressed_bytes.length).toBeLessThan(books_bytes.length);

            const ddict = new ZstdDecompressionDict(dict_bytes);
            expect(simple.decompressUsingDict(compressed_bytes, ddict)).toEqual(books_bytes);

            cdict.delete();
            ddict.delete();
        });
    });
});

describe('ZstdCodec.Streaming', () => {
    const streaming = new ZstdCodec.Streaming();

    describe('compress()', () => {
        it('should compress whole data', () => {
            const lorem_bytes = new TextEncoder('utf-8').encode(LOREM_TEXT);
            const compressed_bytes = streaming.compress(lorem_bytes);

            // compressed?
            expect(compressed_bytes).toEqual(expect.any(Uint8Array));
            expect(compressed_bytes.length).toBeLessThan(lorem_bytes.length);

            // can decompress?
            const check_bytes = streaming.decompress(compressed_bytes);
            expect(check_bytes).toEqual(expect.any(Uint8Array));
            expect(check_bytes).toHaveLength(lorem_bytes.length);
            expect(check_bytes.toString()).toEqual(lorem_bytes.toString());
        });

        it('should compress chunked data', () => {
            let man_bytes = fixtureBinary('dance_yorokobi_mai_man.bmp');
            const chunks = new TypedArrayChunks(man_bytes, 32 * 1024);
            const compressed_bytes = streaming.compressChunks(chunks, 512, 5);

            // compressed?
            expect(compressed_bytes).toEqual(expect.any(Uint8Array));
            expect(compressed_bytes.length).toBeLessThan(man_bytes.length);

            // can decompress?
            const check_bytes = streaming.decompress(compressed_bytes);
            expect(check_bytes).toEqual(expect.any(Uint8Array));
            expect(check_bytes).toHaveLength(man_bytes.length);
            // NOTE: `RangeError: Maximum call stack size exceeded` occurs when compare whole bytes.
            expect(check_bytes.slice(      0,  500000).toString()).toEqual(man_bytes.slice(      0,  500000).toString());
            expect(check_bytes.slice( 500000, 1000000).toString()).toEqual(man_bytes.slice( 500000, 1000000).toString());
            expect(check_bytes.slice(1000000, 1500000).toString()).toEqual(man_bytes.slice(1000000, 1500000).toString());
            expect(check_bytes.slice(1500000, 2000000).toString()).toEqual(man_bytes.slice(1500000, 2000000).toString());
        });
    });

    describe('decompress()', () => {
        it('should decompress whole data', () => {
            let zst_bytes = fixtureBinary('dance_yorokobi_mai_man.bmp.zst');
            const content_bytes = streaming.decompress(zst_bytes);

            // decompressed?
            expect(content_bytes).toEqual(expect.any(Uint8Array));
            expect(content_bytes.length).toBeGreaterThan(zst_bytes.length);

            const man_bytes = fixtureBinary('dance_yorokobi_mai_man.bmp');
            expect(content_bytes).toHaveLength(man_bytes.length);

            // NOTE: `RangeError: Maximum call stack size exceeded` occurs when compare whole bytes.
            expect(content_bytes.slice(      0,  500000).toString()).toEqual(man_bytes.slice(      0,  500000).toString());
            expect(content_bytes.slice( 500000, 1000000).toString()).toEqual(man_bytes.slice( 500000, 1000000).toString());
            expect(content_bytes.slice(1000000, 1500000).toString()).toEqual(man_bytes.slice(1000000, 1500000).toString());
            expect(content_bytes.slice(1500000, 2000000).toString()).toEqual(man_bytes.slice(1500000, 2000000).toString());

            fs.writeFileSync(tempPath('dance_yorokobi_mai_man.bmp'), content_bytes);
        });

        it('should decompress chunked data', () => {
            let zst_bytes = fixtureBinary('dance_yorokobi_mai_woman.bmp.zst');
            const chunks = new TypedArrayChunks(zst_bytes, 1024);
            const content_bytes = streaming.decompressChunks(chunks, 512 * 1024);

            // decompressed?
            expect(content_bytes).toEqual(expect.any(Uint8Array));
            expect(content_bytes.length).toBeGreaterThan(zst_bytes.length);

            const woman_bytes = fixtureBinary('dance_yorokobi_mai_woman.bmp');
            expect(content_bytes).toHaveLength(woman_bytes.length);

            // NOTE: `RangeError: Maximum call stack size exceeded` occurs when compare whole bytes.
            expect(content_bytes.slice(      0,  500000).toString()).toEqual(woman_bytes.slice(      0,  500000).toString());
            expect(content_bytes.slice( 500000, 1000000).toString()).toEqual(woman_bytes.slice( 500000, 1000000).toString());
            expect(content_bytes.slice(1000000, 1500000).toString()).toEqual(woman_bytes.slice(1000000, 1500000).toString());
            expect(content_bytes.slice(1500000, 2000000).toString()).toEqual(woman_bytes.slice(1500000, 2000000).toString());

            fs.writeFileSync(tempPath('dance_yorokobi_mai_woman.bmp'), content_bytes);
        });
    });
});
