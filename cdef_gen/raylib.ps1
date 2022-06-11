cl /I ../raylib/include /EP .\raylib.c > .\raylib-cdef.lua
("local ffi = require 'ffi'`nffi.cdef[[" + `
	((Get-Content .\raylib-cdef.lua -Raw)`
	-replace '\r', ''`
	-replace '\n\s+', "`n" `
	-replace '\n#pragma [owre].+', "`n"`
	-replace '\n\n+', "`n"`
)+ "]]`nreturn ffi.load('raylib.dll')") | Out-File .\raylib-cdef.lua
Write-Host "Please manually remove all the things before typedef struct Vector2"
