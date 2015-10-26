# statvfs

statvfs is a super simple module that just gives you access to statvfs(3). If
you're not familiar with it, you use it to get information about the filesystem.
This API looks like node's native `fs.stat` API.  Run `man statvfs` for more
information.

# Installation

    npm install statvfs

Note this is a native add-on. You will need gcc, python, blah blah blah.

# Usage

```
var statvfs = require('statvfs');

statvfs('/tmp', function (err, stats) {
    assert.ifError(err); // on errno, will be a node ErrnoException
    console.log(JSON.stringify(stats, null, 2));
    /*
    {
        "bsize": 4096,
        "frsize": 4096,
        "blocks": 262144,
        "bfree": 252508,
        "bavail": 252508,
        "files": 292304,
        "ffree": 289126,
        "favail": 289126,
        "fsid": 140509193,
        "basetype": "tmpfs",
        "flag": 4,
        "namemax": 255,
        "fstr": "/tmp"
    }
    */
});
```

# License

MIT
