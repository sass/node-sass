{
  'targets': [
    {
      'target_name': 'binding',
      'type': '<(library)',
      'sources': [
        'binding.cpp',
        'libsass/constants.cpp',
        'libsass/context.cpp',
        'libsass/document.cpp',
        'libsass/document_parser.cpp',
        'libsass/eval_apply.cpp',
        'libsass/functions.cpp',
        'libsass/node.cpp',
        'libsass/node_emitters.cpp',
        'libsass/node_factory.cpp',
        'libsass/prelexer.cpp',
        'libsass/sass_interface.cpp',
        'libsass/selector.cpp'
      ],
      'conditions': [
        ['OS=="mac"', {
          'xcode_settings': {
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
            'MACOSX_DEPLOYMENT_TARGET': '10.7'
          }
         }],
        ['OS=="linux"', {
          'cflags_cc': [
            '-fexceptions'
          ]
         }]
      ]
    }
  ]
}
