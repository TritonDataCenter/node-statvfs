// Copyright (c) 2012, Mark Cavage. All rights reserved.

var assert = require('assert');

var statvfs = require('../lib');

assert.ok(statvfs);
assert.equal(typeof (statvfs), 'function');

statvfs('/tmp', function (err, stats) {
        assert.ifError(err);
        assert.ok(stats.bsize);
        assert.ok(stats.frsize);
        assert.ok(stats.blocks);
        assert.ok(stats.bfree);
        assert.ok(stats.bavail);
        assert.ok(stats.files);
        assert.ok(stats.ffree);
        assert.ok(stats.favail);
        assert.ok(stats.fsid);
        assert.ok(stats.flag !== undefined);
        assert.ok(stats.namemax);
});
