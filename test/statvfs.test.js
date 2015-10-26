// Copyright (c) 2012, Mark Cavage. All rights reserved.

var assert = require('assert-plus');

var statvfs = require('../lib');

assert.ok(statvfs);
assert.equal(typeof (statvfs), 'function');

statvfs('/tmp', function (err, stats) {
	assert.ifError(err);
	assert.number(stats.bsize, 'stats.bsize');
	assert.number(stats.frsize, 'stats.frsize');
	assert.number(stats.blocks, 'stats.blocks');
	assert.number(stats.bfree, 'stats.bfree');
	assert.number(stats.bavail, 'stats.bavail');
	assert.number(stats.files, 'stats.files');
	assert.number(stats.ffree, 'stats.files');
	assert.number(stats.favail, 'stats.favail');
	assert.number(stats.fsid, 'stats.fsid');
	assert.number(stats.flag, 'stats.flag');
	assert.number(stats.namemax, 'stats.namemax');
});

statvfs('/nonexistent', function (err, stats) {
	assert.ok(err instanceof Error);
	assert.equal(stats, undefined);
});
