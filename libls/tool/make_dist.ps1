##############################
# 参数定义
# 原位置
$src_dir = "..\libls\src\"
$exclude_headers = "stdafx.h",
                "targetver.h",
                "core\crypto\aes.h",
                "core\crypto\base64.h",
                "core\crypto\cipher_def.h",
                "core\crypto\des.h",
                "core\crypto\md5.h",
                "core\crypto\sha1.h",
                "core\crypto\sha256.h",
                "core\crypto\url_encode.h",
                "core\json\json_batchallocator.h",
                "core\string\string.h"
$lib_dir = "..\libls\bin\"
$lib_files = "libls.dll", "libls.lib", "libls.pdb"

#目标位置
$include_path = "dist\include\libls\"
$lib_path = "dist\lib\"

##############################
# 函数定义
function mkdir([string]$path) {
    if(-not (Test-Path -Path $path)){
        New-Item -Path $path -ItemType Directory
    }
}

function copyfile([string]$src, [string]$target) {
    write-host  "$src ==> $target"
    $dir = $target.substring(0, $target.lastindexof("\"))
    mkdir($dir) | out-null
    copy-item -path $src -destination $dir | out-null
}

##############################
# 脚本
# 拷贝头文件
write-host "=== copy headers ==="
mkdir($include_path) | out-null
get-childitem -recurse $src_dir -filter "*.h" | foreach-object -process {
    $name = $_.fullname.substring($_.fullname.indexof("\libls\src\") + "\libls\src\".length)
    if ($exclude_headers -contains $name) {
        #write-host "... skip $name"
    } else {
        $target = $include_path + $name
        #write-host $target
        copyfile $_.fullname $target
    }
}

# 拷贝lib文件
write-host "=== copy libs ==="
mkdir $lib_path | out-null
get-childitem -recurse $lib_dir | foreach-object -process {
    if ($lib_files -contains $_.name) {
        $name = $_.fullname.replace((resolve-path $lib_dir), "")
        #write-host $name        
        $target = $lib_path + $name
        #write-host $target
        copyfile $_.fullname $target
    }
}