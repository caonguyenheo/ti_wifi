var argv = require('minimist')(process.argv.slice(2));
var crypto = require('crypto');
var fs = require('fs');
var events = require('events');

function checksum (file, algorithm, encoding) {
    var self = this;

    var stream = fs.createReadStream(file);
    var hash = crypto.createHash(algorithm || 'md5');

    stream.on('data', function (data) {
        hash.update(data, 'utf8')
    })

    stream.on('end', function () {
        var result = hash.digest(encoding || 'hex');
        self.emit('done', result);
    })

    return self;
}
checksum.prototype.__proto__ = events.EventEmitter.prototype;

// main
var file = argv.file;
if (!file) {
    console.error('Use: %s --file', __filename.slice(__dirname.length + 1));
    return;
}

var cs = new checksum(file);
cs.on('done', function(data) {
    console.log('checksum = %s', data);
})
