// Copyright (c) 2012, Mark Cavage. All rights reserved.

var _statvfs = require('../build/Release/statvfs').statvfs;

module.exports = function statvfs(path, callback) {
        return (_statvfs(path, callback));
};
