{
  'targets': [
    {
      'target_name': 'binding',
      'sources': [
        'binding.cpp',
        'sass_context_wrapper.cpp',
        'libsass/ast.cpp',
        'libsass/base64vlq.cpp',
        'libsass/bind.cpp',
        'libsass/constants.cpp',
        'libsass/context.cpp',
        'libsass/contextualize.cpp',
        'libsass/copy_c_str.cpp',
        'libsass/error_handling.cpp',
        'libsass/eval.cpp',
        'libsass/expand.cpp',
        'libsass/extend.cpp',
        'libsass/file.cpp',
        'libsass/functions.cpp',
        'libsass/inspect.cpp',
        'libsass/output_compressed.cpp',
        'libsass/output_nested.cpp',
        'libsass/parser.cpp',
        'libsass/prelexer.cpp',
        'libsass/sass.cpp',
        'libsass/sass_interface.cpp',
        'libsass/source_map.cpp',
        'libsass/to_c.cpp',
        'libsass/to_string.cpp',
        'libsass/units.cpp',
        'libsass/utf8_string.cpp',
        'libsass/util.cpp',
        'libsass/sass2scss/sass2scss.cpp'
      ],
      'include_dirs': [
        '<!(node -e "require(\'nan\')")'
      ],
      'cflags!'   : [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],
      'cflags_cc' : [ '-fexceptions', '-frtti' ],
      'conditions': [
        ['OS=="mac"', {
          'xcode_settings': {
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
            'GCC_ENABLE_CPP_RTTI': 'YES',
            'MACOSX_DEPLOYMENT_TARGET': '10.7'
          }
         }],
        ['OS=="win"', {
          'msvs_settings': {
            'VCCLCompilerTool': {
              'AdditionalOptions': [
                '/GR',
                '/EHsc'
              ]
            }
          },
          'msvs_disabled_warnings': [
            # conversion from `double` to `size_t`, possible loss of data
            4244,
            # decorated name length exceeded
            4503
          ]
        }]
      ]
    }
  ]
}
