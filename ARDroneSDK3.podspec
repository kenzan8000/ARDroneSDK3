Pod::Spec.new do |s|
  s.name                = 'ARDroneSDK3'
  s.version             = '3.7.5'
  s.vendored_frameworks = 'curl.framework', 'json.framework', 'libARCommands.framework', 'libARController.framework', 'libARDiscovery.framework', 'libARNetwork.framework', 'libARNetworkAL.framework', 'libARSAL.framework', 'libARStream.framework', 'libARUpdater.framework', 'libARUtils.framework', 'libressl.framework', 'mavlink.framework', 'uthash.framework'
  s.frameworks          = 'CoreBlueTooth'
  s.compiler_flags      = '-ObjC'
  s.xcconfig            = { 'FRAMEWORK_SEARCH_PATHS' => '"$(PODS_ROOT)/ARDroneSDK3"' }
end

