cl /I ../spine/2.1.25/spine-c/include /EP .\hdt-raylib-spine.c > .\hdt-raylib-spine.cdef.lua
("local ffi = require 'ffi'`nffi.cdef[[" + ((Get-Content .\hdt-raylib-spine.cdef.lua -Raw) -replace '\r', '' -replace '\n\n+', "`n") + "]]`nreturn ffi.load('raylib.dll')") | Out-File .\hdt-raylib-spine-cdef.lua

