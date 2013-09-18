require 'mkmf'
dir_config('qrencode', ['/Users/tiffani/Downloads/qrencode-3.1.1'], ['/usr/local/lib'])
have_library('qrencode', 'QRcode_free')
have_header('qrencode.h') or abort "Can't find the qrencode.h header file."
have_func 'QRcode_encodeString'
have_func 'QRcode_free'
have_header 'png.h'
have_library('png', 'png_create_info_struct')
create_makefile("qrencoder")