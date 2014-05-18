{
  'targets': [
    {
      'target_name': 'binding',
      'include_dirs': [
        '<!(node -e "require(\'nan\')")',
        'protobuf/src',
        'src',
      ],
      'dependencies': [
        'protobuf/protobuf.gyp:protobuf_full_do_not_use',
      ],
      'sources': [
        'src/descriptor.cc',
        'src/protobuf.cc',
        'src/schema.cc',
      ],
      'conditions': [
        # [
        #   'OS=="linux"', {
        #     'cflags_cc': [
        #       '-std=c++0x'
        #     ]
        #   }
        # ],
        [
          'OS =="mac"',{
            # 'cflags_cc': [
            #   '-std=c++0x'
            # ],
            'xcode_settings':{
              'OTHER_CFLAGS' : [
                '-mmacosx-version-min=10.7'
              ],
            },
          },
        ],
      ],
    },
  ],
}
