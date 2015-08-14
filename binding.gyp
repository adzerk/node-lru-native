{
  "targets": [
    {
      "target_name": "native",
      "sources": ["src/native.cc", "src/LRUCache.cc"],
      "include_dirs": [
        "<!(node -e \"require('nan')\")"
      ],
      "cflags": [ "-std=c++0x", "-O2" ]
    }
  ]
}
