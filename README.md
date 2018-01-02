# zstd-codec
[Zstandard](http://facebook.github.io/zstd/) codec for Node.js and Web, powered by Emscripten.

## Languages
- [English](README.md)

## Description
zstd-codec is a binding of [Zstandard](http://facebook.github.io/zstd/) for Node.js and Browsers,
includes JavaScript port of [Zstandard](http://facebook.github.io/zstd/) compiled with [Emscripten](http://kripken.github.io/emscripten-site/index.html).

## Installation
npm
```bash
npm install zstd-codec
```

yarn
```bash
yarn add zstd-codec
```

## Usage

require module, and instantiate api objects.

```bash
const ZstdCodec = require('zsztd-codec');
const simple = new ZstdCodec.Simple();
const streaming = new ZstdCodec.Streaming();
```

- Use Simple API for small data
- Use Streaming API for large data

### Simple API
- Using Zstandard's Simple API
    - `ZSTD_compress` for compress
    - `ZSTD_decompress` for decompress
- Store whole input/output bytes into Emscripten's heap
    - Available Emscripten's heap size is 16MiB
    - (input.length + output.length) should be less than 12MiB

#### compress(content_bytes, compression_level)
- `content_bytes`:  data to compress, must be `Uint8Array`.
- `compression_level`: (optional) compression level, default value is `3`

```javascript
// prepare data to compress
const data = ...;

// compress
const level = 5;
const compressed = simple.compress(data, level);

// handle compressed data
do_something(compressed);
```

#### decompress(compressed_bytes)
- `compressed_bytes`: data to decompress, must be `Uint8Array`.

```javascript
// prepare compressed data
const compressed = ...;

// decompress
const data = simple.decompress(compressed);

// handle decompressed data
do_something(data);
```

### Streaming APIs
- Using Zstandard's Streaming API
    - `ZSTD_xxxxCStream` APIs for compress
    - `ZSTD_xxxxDStream` APIs for decompress
- Store partial input/output bytes into Emscripten's heap

```javascript
const streaming = new ZstdCodec.Streaming();
```

You can use custom `Iterable` object on `compressChunks` / `decompressChunks`.

#### compress(content_bytes, compression_level)
- `content_bytes`: data to compress, must be 'Uint8Array'
- `compression_level`: (optional) compression level, default value is `3`

```javascript
const compressed = streaming.compress(data); // use default compression_level 3
```

#### compressChunks(chunks, size_hint, compression_level)
- `chunks`: data chunks to compress, must be `Iterable` of `Uint8Array`
- `size_hint`: (optional) size hint to store compressed data (to improve performance)
- `compression_level`: (optional) compression level, default value is `3`

```javascript
const chunks = [dataPart1, dataPart2, dataPart3, ...];
const size_hint = chunks.map((ar) => ar.length).reduce((p, c) => p + c);
const compressed = streaming.compressChunks(chunks, size_hint); // use default compression_level 3
```


#### decompress(compressed_bytes, size_hint)
- `compressed_bytes`: data to decompress, must be `Uint8Array`.
- `size_hint`: (optional) size hint to store decompressed data (to improve performance)

```javascript
const data = streaming.decompress(data); // can omit size_hint
```

#### decompressChunks(chunks, size_hint)
- `chunks`: data chunks to compress, must be `Iterable` of `Uint8Array`
- `size_hint`: (optional) size hint to store compressed data (to improve performance)

```javascript
const chunks = [dataPart1, dataPart2, dataPart3, ...];
const size_hint = 2 * 1024 * 1024; // 2MiB
const data = streaming.decompressChunks(chunks, size_hint);
```

## Example

### Browser
See the [document](https://github.com/yoshihitoh/zstd-codec/blob/develop/js/example/README.md).

### Node.js
TODO: add an example for Node.js.

## TODO
- add CI (Travis CI or Circle CI?)
- improve APIs
- write  this document
- add how to build zstd with Emsxcripten
- add how to test
- performance test
- add more tests
