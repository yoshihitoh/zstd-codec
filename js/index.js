const zstd_codec = require('./lib/zstd-codec.js');
const zstd_dict = require('./lib/zstd-dict.js');

module.exports = {};
module.exports.ZstdCodec = zstd_codec.ZstdCodec;

module.exports.ZstdCompressionDict = zstd_dict.ZstdCompressionDict;
module.exports.ZstdDecompressionDict = zstd_dict.ZstdDecompressionDict;
