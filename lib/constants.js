var Runtimes = exports.Runtimes = {
  NODE: 0,
  NODEJS: 1,
  IOJS: 2,
  ELECTRON: 3
};
exports.RuntimeNames = [
  'Node',
  'Node.js',
  'io.js',
  'Electron'
];
exports.ModuleVersions = {
  11: [Runtimes.NODE, '0.10.x'],
  14: [Runtimes.NODE, '0.12.x'],
  42: [Runtimes.IOJS, '1.x'],
  43: [Runtimes.IOJS, '1.1.x'],
  44: [Runtimes.IOJS, '2.x'],
  45: [Runtimes.IOJS, '3.x'],
  49: [Runtimes.ELECTRON, '1.3.x'],
  50: [Runtimes.ELECTRON, '1.4.x'],
  53: [Runtimes.ELECTRON, '1.6.x'],
  46: [Runtimes.NODEJS, '4.x'],
  47: [Runtimes.NODEJS, '5.x'],
  48: [Runtimes.NODEJS, '6.x'],
  51: [Runtimes.NODEJS, '7.x'],
  57: [Runtimes.NODEJS, '8.x'],
  59: [Runtimes.NODEJS, '9.x'],
  64: [Runtimes.NODEJS, '10.x'],
  67: [Runtimes.NODEJS, '11.x']
};
exports.DefaultOptions = {
  jobs: 1,
  target: process.version,
  force: process.env.SASS_FORCE_BUILD || false,
  arch: process.arch,
  debug: process.config.target_defaults ? process.config.target_defaults.default_configuration === 'Debug' : false,
  modulesVersion: process.versions.modules
};