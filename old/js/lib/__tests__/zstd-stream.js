const fs = require('fs');
const path = require('path');
const ZstdStream = require('../zstd-stream.js');

const loadBinary = (filePath) => {
    const data = fs.readFileSync(filePath);
    return new Uint8Array(data);
};


const fixturePath = (name) => {
    return path.join(__dirname, 'fixtures', name);
};


const tempPath = (name) => {
    return path.join(__dirname, 'tmp', name);
};


describe('ZstdXXXTransform', () => {

    it('should decompress zst file', (done) => {
        ZstdStream.run((streams) => {
            const ZstdCompressTransform = streams.ZstdCompressTransform;
            const ZstdDecompressTransform = streams.ZstdDecompressTransform;

            const src_zst_file = fixturePath('dance_yorokobi_mai_woman.bmp.zst');
            const src_bmp_file = fixturePath('dance_yorokobi_mai_woman.bmp');
            const dest_file = tempPath('dance_yorokobi_mai_woman-stream.bmp');

            const source = fs.createReadStream(src_zst_file);
            const first_decompress = new ZstdDecompressTransform();
            const second_compress = new ZstdCompressTransform();
            const third_decompress = new ZstdDecompressTransform();
            const sink = fs.createWriteStream(dest_file);
            return new Promise((resolve) =>  {
                sink.on('finish', () => {
                    resolve();
                });
                source
                .pipe(first_decompress)
                .pipe(second_compress)
                .pipe(third_decompress)
                .pipe(sink);
            }).then(() => {
                const result = loadBinary(dest_file);
                const check = loadBinary(src_bmp_file);

                expect(result).toHaveLength(check.length);
                expect(result.slice(      0,  500000).toString()).toEqual(check.slice(      0,  500000).toString());
                expect(result.slice( 500000, 1000000).toString()).toEqual(check.slice( 500000, 1000000).toString());
                expect(result.slice(1000000, 1500000).toString()).toEqual(check.slice(1000000, 1500000).toString());
                expect(result.slice(1500000, 2000000).toString()).toEqual(check.slice(1500000, 2000000).toString());

                done();
            });
        });
    });
});
