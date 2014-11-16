{
  'targets': [
    {
      'target_name': 'binding',
      'sources': [
        'src/binding.cpp',
        'src/sass_context_wrapper.cpp',
        'src/libsass/ast.cpp',
        'src/libsass/base64vlq.cpp',
        'src/libsass/bind.cpp',
        'src/libsass/cencode.c',
        'src/libsass/constants.cpp',
        'src/libsass/context.cpp',
        'src/libsass/contextualize.cpp',
        'src/libsass/copy_c_str.cpp',
        'src/libsass/error_handling.cpp',
        'src/libsass/eval.cpp',
        'src/libsass/expand.cpp',
        'src/libsass/extend.cpp',
        'src/libsass/file.cpp',
        'src/libsass/functions.cpp',
        'src/libsass/inspect.cpp',
        'src/libsass/json.cpp',
        'src/libsass/node.cpp',
        'src/libsass/output_compressed.cpp',
        'src/libsass/output_nested.cpp',
        'src/libsass/parser.cpp',
        'src/libsass/prelexer.cpp',
        'src/libsass/remove_placeholders.cpp',
        'src/libsass/sass.cpp',
        'src/libsass/sass2scss.cpp',
        'src/libsass/sass_context.cpp',
        'src/libsass/sass_functions.cpp',
        'src/libsass/sass_util.cpp',
        'src/libsass/sass_values.cpp',
        'src/libsass/source_map.cpp',
        'src/libsass/to_c.cpp',
        'src/libsass/to_string.cpp',
        'src/libsass/units.cpp',
        'src/libsass/utf8_string.cpp',
        'src/libsass/util.cpp'
      ],
      'include_dirs': [
        '<!(node -e "require(\'nan\')")'
      ],
      'cflags!': [
        '-fno-exceptions'
      ],
      'cflags_cc!': [
        '-fno-exceptions'
      ],
      'cflags_cc': [
        '-fexceptions',
        '-frtti'
      ],
      'conditions': [
        ['OS=="mac"', {
          'xcode_settings': {
            'OTHER_CPLUSPLUSFLAGS': [
              '-std=c++11',
              '-stdlib=libc++'
            ],
            'OTHER_LDFLAGS': [
              '-stdlib=libc++'
            ],
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
        }],
        ['OS!="win"', {
          'cflags_cc+': [
            '-std=c++0x'
          ]
        }]
      ]
    }
  ]
}
