{
  "targets": [
    {
      "target_name": "native",
      "sources": ["src/native.cc", "src/LRUCache.cc"],
      "cflags": ["-O2"],
      "cflags_cc": ["-O2"],
      "include_dirs" : [
        "<!(node -e \"require('nan')\")"
      ]
    }
  ]
}
