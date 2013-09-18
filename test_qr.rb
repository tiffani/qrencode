require './qrencoder'

q = QRcode.new
q.data = "Yayyyyy!! We got QR codes going now!"
q.encode
q.write_png "png_stylin_on_em013.png"