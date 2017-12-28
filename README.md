# zstd-codec
Zstandard codec for Node.js and Web, powered by Emscripten

## Installation
- TODO: write installation sptes here

## Usage

- TODO: Add require ZstdCodec

### Basics
#### compress(content_bytes: Uint8Array, compression_level: number) -> Uint8Array
```javascript
// prepare data to compress
const text = `Lorem ipsum dolor sit amet, consectetur adipiscing elit, ...`;
const content_bytes = new Uint8Array(new TextEncoder("utf-8").encode(text));

// compress
const compressed_bytes = codec.compress(content_bytes, 5);  // default level is 3
```

NOTE: (content_bytes.length + compressed_bytes.length) should be less than 10MiB.

#### decompress(compressed_bytes: Uint8Array) -> Uint8Array
```javascript
// prepare data to decompress
const compressed_bytes = ... // Uint8Array data to decompress

// decompress
const content_bytes = codec.decompress(compressed_bytes);
```

NOTE: (content_bytes.length + compressed_bytes.length) should be less than 10MiB.

### Streaming mode
NOTE: streaming api is experimental. Current implementation is not useful.

## TODO
- add CI
- improve APIs
