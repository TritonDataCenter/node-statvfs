{
        "targets": [ {
                "target_name": "statvfs",
                "sources": [ "src/statvfs.cpp" ],
                "include_dirs": [
                    "<!(node -e 'require(\"nan\")')"
                ]
        } ]
}
