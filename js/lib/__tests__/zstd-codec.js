const fs = require('fs');
const path = require('path');
const textEncoding = require('text-encoding');
const TextEncoder = textEncoding.TextEncoder;
const TextDecoder = textEncoding.TextDecoder;
const ArrayBufferHelper = require('../helpers.js').ArrayBufferHelper;
const ZstdCodec = require('../zstd-codec.js').ZstdCodec;

const fixture_path = (name) => {
    return path.join(__dirname, 'fixtures', name);
}

const fixture_binary = (name) => {
    const data = fs.readFileSync(fixture_path(name));
    return new Uint8Array(data);
};

const fixture_text = (name) => {
    return fs.readFileSync(fixture_path(name), 'utf-8');
};

const tmp_path = (name) => {
    return path.join(__dirname, 'tmp', name);
}

const LOREM_TEXT = 'Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.';

describe('ZstdCodec', () => {
    const codec = new ZstdCodec();

    const with_file = (path, flag, callback) => {
        const fp = fs.openSync(path, flag);
        try {
            callback(fp);
        }
        finally {
            fs.closeSync(fp);
        }
    };

    describe('compress_bound()', () => {
        it('should retrieve compressed size in worst case', () => {
            expect(codec.compress_bound(new Uint8Array())).toBeGreaterThan(0);

            const lorem_bytes = fixture_binary('lorem.txt');
            expect(codec.compress_bound(lorem_bytes)).toBeGreaterThan(0);
        });
    });

    describe('content_size()', () => {
        it('should retrieve content size from zstd compressed files', () => {
            expect(codec.content_size(fixture_binary('lorem.txt.zst'))).toBe(446);
            expect(codec.content_size(fixture_binary('dance_yorokobi_mai_man.bmp.zst'))).toBe(1920054);
            expect(codec.content_size(fixture_binary('dance_yorokobi_mai_woman.bmp.zst'))).toBe(1920054);
        });
    });

    describe('compress()', () => {
        it('should compress data', () => {
            const lorem_bytes = new TextEncoder('utf-8').encode(LOREM_TEXT);
            const compressed_bytes = codec.compress(lorem_bytes);

            expect(compressed_bytes.length).toBeLessThan(lorem_bytes.length);
            expect(codec.decompress(compressed_bytes)).toEqual(lorem_bytes);
        });
    });

    describe('decompress()', () => {
        it('should decompress data', () => {
            const lorem_bytes = codec.decompress(fixture_binary('lorem.txt.zst'));
            expect(lorem_bytes).toEqual(fixture_binary('lorem.txt'));
            // NOTE: use .toString() to avoid RangeError.
            const man_bytes = codec.decompress(fixture_binary('dance_yorokobi_mai_man.bmp.zst'));
            expect(man_bytes.toString()).toEqual(fixture_binary('dance_yorokobi_mai_man.bmp').toString());

            const woman_bytes = codec.decompress(fixture_binary('dance_yorokobi_mai_woman.bmp.zst'));
            expect(woman_bytes.toString()).toEqual(fixture_binary('dance_yorokobi_mai_woman.bmp').toString());
        });
    });

    /*
    describe('compress_stream()', () => {
        it('should compress data', () => {
            const lorem_bytes = new TextEncoder('utf-8').encode(LOREM_TEXT);
            const buffer = new ArrayBuffer(codec.compress_bound(lorem_bytes));
            let src_offset = 0;
            let dest_offset = 0;
            const success = codec.compress_stream((read_size) => {
                const read_bytes = lorem_bytes.slice(src_offset, src_offset + read_size);
                src_offset += read_size;
                return read_bytes;
            }, (write_bytes) => {
                new Uint8Array(buffer, dest_offset, write_bytes.length).set(write_bytes);
                dest_offset += write_bytes.length;
            });
            const compressed_bytes = new Uint8Array(buffer, 0, dest_offset);

            expect(success).toBeTruthy();
            expect(compressed_bytes.length).toBeLessThan(lorem_bytes.length);
        });
    });

    describe('decompress_stream()', () => {
        it('should decompress data', () => {
            let src_bytes = fixture_binary('dance_yorokobi_mai_man.bmp.zst');
            let src_offset = 0;
            let dest_buffer = new ArrayBuffer(512 * 1024);
            let dest_offset = 0;
            const success = codec.decompress_stream((read_size) => {
                const read_bytes = src_bytes.slice(src_offset, src_offset + read_size);
                src_offset += read_bytes.length;
                return read_bytes;
            }, (write_bytes) => {
                dest_buffer = ArrayBufferHelper.ensure_array_buffer(dest_buffer, dest_offset + write_bytes.length);
                const dest_bytes = new Uint8Array(dest_buffer);
                dest_bytes.set(write_bytes, dest_offset);
                dest_offset += write_bytes.length;
            });

            const content_bytes = new Uint8Array(dest_buffer, 0, dest_offset);
            fs.writeFileSync(tmp_path('dance_yorokobi_mai_man.bmp'), content_bytes);

            const man_bytes = fixture_binary('dance_yorokobi_mai_man.bmp');

            expect(success).toBeTruthy();
            expect(content_bytes.length).toEqual(man_bytes.length);
            expect(content_bytes.slice(0, 500000).toString()).toEqual(man_bytes.slice(0, 500000).toString());
        });
    });
    */
});
