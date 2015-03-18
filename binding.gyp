{
  'targets': [
    {
      'target_name': 'binding',
      'sources': [
        'src/binding.cpp',
        'src/sass_context_wrapper.cpp'
      ],
      'include_dirs': [
        '<!(node -e "require(\'nan\')")',
      ],
      'conditions': [
        ['libsass_ext == ""', {
            'dependencies': [
              'libsass.gyp:libsass',
            ]
        }],
	['libsass_ext == "auto"', {
	  'cflags_cc': [
	    '<!(pkg-config --cflags libsass)',
	  ],
	  'link_settings': {
	    'ldflags': [
	      '<!(pkg-config --libs-only-other --libs-only-L libsass)',
	    ],
	    'libraries': [
	      '<!(pkg-config --libs-only-l libsass)',
	    ],
	  }
	}],
	['libsass_ext == "yes"', {
	  'cflags_cc': [
	    '<(libsass_cflags)',
	  ],
	  'link_settings': {
	    'ldflags': [
	      '<(libsass_ldflags)',
	    ],
	    'libraries': [
	      '<(libsass_library)',
	    ],
	  }
        }],
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
